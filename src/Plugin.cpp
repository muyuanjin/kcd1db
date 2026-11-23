#include "LuaDB.h"
#include "db_lua.h"

#include <windows.h>
#include <objbase.h>
#include <MinHook.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <memory>
#include <utility>

namespace
{
constexpr DWORD kImageTimestamp = 0x69ccd815;
constexpr DWORD kImageSize = 0x039eb000;
constexpr std::size_t kCompleteInitSlotRva = 0x02734e78;
constexpr std::size_t kCompleteInitRva = 0x0186ddc4;
constexpr std::size_t kShutdownSlotRva = 0x02734e80;
constexpr std::size_t kShutdownRva = 0x01895bb4;
constexpr std::size_t kScriptSystemVtableRva = 0x027d2488;
constexpr std::size_t kSaveGameRva = 0x00f12d64;
constexpr std::size_t kWriteSaveFileRva = 0x00f0f088;
constexpr std::size_t kSaveHelperVtableRva = 0x026a2dd8;
constexpr std::size_t kLoadHelperVtableRva = 0x026a2fd0;
constexpr std::size_t kLuaToStringRva = 0x002b7ee4;
constexpr char kDatabaseName[] = "kcd1db.db";
constexpr char kSnapshotMetadata[] = "kcd1db_snapshot";

using CompleteInit = bool(*)(void*);
using Shutdown = void(*)(void*);
using SaveGame = bool(*)(void*, int, int, const char*);
using WriteSaveFile = bool(*)(void*, void*, void*, int);
std::byte* g_gameModule;
std::unique_ptr<LuaDB> g_db;
void* g_scriptSystem;
std::filesystem::path g_root;
SaveGame g_saveGame;
WriteSaveFile g_writeSaveFile;
bool g_saveListenerRegistered;

struct PendingSnapshot
{
    std::string filename;
    std::string version;
    LuaDB::Cache cache;
};

thread_local std::optional<PendingSnapshot>* g_pendingSnapshot;

void log_message(const char* operation, const char* detail = "")
{
    FILE* file = nullptr;
    if (!_wfopen_s(&file, (g_root / L"kcd1db.log").c_str(), L"a"))
    {
        std::fprintf(file, "[kcd1db %s] %s%s%s\n", KCD1DB_VERSION, operation,
                     *detail ? ": " : "", detail);
        std::fclose(file);
    }
}

template<class Fn>
Fn method(void* object, std::size_t byteOffset)
{
    const auto vtable = *static_cast<std::byte**>(object);
    return *reinterpret_cast<Fn*>(vtable + byteOffset);
}

template<class Fn>
Fn game_function(std::size_t rva)
{
    return reinterpret_cast<Fn>(g_gameModule + rva);
}

enum class AnyType : int { Any, Nil, Boolean, Handle, Number, String, Table };

struct ScriptAny
{
    AnyType type;
    union
    {
        const void* pointer;
        const char* string;
        bool boolean;
        float number;
        std::byte storage[16];
    };

    explicit ScriptAny(AnyType kind = AnyType::Any) : type(kind), storage{} {}
    ~ScriptAny()
    {
        if (type > AnyType::String)
            game_function<void(*)(ScriptAny*)>(0x003b9e68)(this);
    }
};

static_assert(offsetof(ScriptAny, pointer) == 8);
static_assert(sizeof(ScriptAny) == 24);

using ScriptCallback = int(*)(void*, void*, int);

struct ScriptFunctionDesc
{
    const char* name;
    const char* params;
    const char* global;
    std::byte functor[32]{};
    signed char paramOffset = 0;
    ScriptCallback callback;
    void* data = nullptr;
    int size = 0;
};

static_assert(offsetof(ScriptFunctionDesc, callback) == 0x40);
static_assert(offsetof(ScriptFunctionDesc, size) == 0x50);

int end_function(void* handler, const ScriptAny& value)
{
    return method<int(*)(void*, const ScriptAny&)>(handler, 0x50)(handler, value);
}

int end_boolean(void* handler, bool value)
{
    ScriptAny any(AnyType::Boolean);
    any.boolean = value;
    return end_function(handler, any);
}

void to_script_any(const LuaDB::Value& value, ScriptAny& any)
{
    if (const auto* boolean = std::get_if<bool>(&value))
    {
        any.type = AnyType::Boolean;
        any.boolean = *boolean;
    }
    else if (const auto* number = std::get_if<float>(&value))
    {
        any.type = AnyType::Number;
        any.number = *number;
    }
    else
    {
        any.type = AnyType::String;
        any.string = std::get<std::string>(value).c_str();
    }
}

void* create_table()
{
    return method<void*(*)(void*, bool)>(g_scriptSystem, 0x68)(g_scriptSystem, false);
}

void hold_table(ScriptAny& any, void* table)
{
    method<void(*)(void*)>(table, 0x18)(table);
    any.type = AnyType::Table;
    any.pointer = table;
}

enum class Access { Set, Get, Del, Exi, All };

bool valid_string_param(void* handler, int param, const char* name)
{
    const auto* object = static_cast<const std::byte*>(handler);
    const auto* scriptSystem = *reinterpret_cast<const std::byte* const*>(object + 0x10);
    auto* state = *reinterpret_cast<void* const*>(scriptSystem + 0x10);
    const auto offset = *reinterpret_cast<const int*>(object + 0x20);
    std::size_t length = 0;
    const auto bytes = game_function<const char*(*)(void*, int, std::size_t*)>(kLuaToStringRva)(
        state, offset + param, &length);
    if (bytes && !std::memchr(bytes, '\0', length))
        return true;

    log_message("LuaDB rejected a string argument", name);
    return false;
}

template<Access operation, bool global = false>
int access(void* handler, void*, int)
{
    try
    {
        if constexpr (operation == Access::All)
        {
            const auto cache = g_db->All(global);
            auto* table = create_table();
            ScriptAny result;
            hold_table(result, table);
            for (const auto& [key, value] : cache)
            {
                ScriptAny any;
                to_script_any(value, any);
                method<void(*)(void*, const char*, const ScriptAny&, bool)>(table, 0x38)(
                    table, key.c_str(), any, false);
            }
            return end_function(handler, result);
        }
        else
        {
            const auto get_param = method<bool(*)(void*, int, ScriptAny&)>(handler, 0x38);
            const auto argc = method<int(*)(void*)>(handler, 0x28)(handler);
            if (argc < (operation == Access::Set ? 2 : 1))
                return end_boolean(handler, false);
            ScriptAny key(AnyType::String);
            if (!get_param(handler, 1, key) || !key.string)
                return end_boolean(handler, false);
            if (!valid_string_param(handler, 1, "key contains NUL"))
            {
                if constexpr (operation == Access::Get)
                    return 0;
                return end_boolean(handler, false);
            }

            if constexpr (operation == Access::Set)
            {
                ScriptAny value;
                if (!get_param(handler, 2, value))
                    return end_boolean(handler, false);
                switch (value.type)
                {
                case AnyType::Boolean: g_db->Set(global, key.string, value.boolean); break;
                case AnyType::Number: g_db->Set(global, key.string, value.number); break;
                case AnyType::String:
                    if (!valid_string_param(handler, 2, "value contains NUL"))
                        return end_boolean(handler, false);
                    g_db->Set(global, key.string, std::string(value.string));
                    break;
                default: return end_boolean(handler, false);
                }
                return end_boolean(handler, true);
            }
            if constexpr (operation == Access::Del)
                return end_boolean(handler, g_db->Del(global, key.string));

            const auto value = g_db->Get(global, key.string);
            if constexpr (operation == Access::Exi)
                return end_boolean(handler, value.has_value());
            if (!value)
                return 0;
            ScriptAny result;
            to_script_any(*value, result);
            return end_function(handler, result);
        }
    }
    catch (const std::exception& e)
    {
        log_message("LuaDB call failed", e.what());
        if constexpr (operation == Access::Get)
            return 0;
        return end_boolean(handler, false);
    }
}

int dump(void*, void*, int)
{
    try
    {
        for (const bool global : {false, true})
        {
            log_message(global ? "Global data" : "Save data");
            for (const auto& [key, value] : g_db->All(global))
            {
                const auto text = std::visit([](const auto& item) -> std::string
                {
                    using T = std::decay_t<decltype(item)>;
                    if constexpr (std::is_same_v<T, std::string>)
                        return item.substr(0, 100);
                    else if constexpr (std::is_same_v<T, bool>)
                        return item ? "true" : "false";
                    else
                        return std::to_string(item);
                }, value);
                log_message(key.c_str(), text.c_str());
            }
        }
    }
    catch (const std::exception& e)
    {
        log_message("LuaDB.Dump failed", e.what());
    }
    return 0;
}

std::string new_snapshot_version()
{
    GUID id{};
    if (FAILED(CoCreateGuid(&id)))
        throw std::runtime_error("Could not create a snapshot version");

    return std::format("{:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                       id.Data1, id.Data2, id.Data3, id.Data4[0], id.Data4[1], id.Data4[2],
                       id.Data4[3], id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]);
}

bool on_save_game(void* manager, int type, int slot, const char* name)
{
    if (!g_saveListenerRegistered)
        return g_saveGame(manager, type, slot, name);

    // The save manager serializes to memory before writing the .whs file.
    std::optional<PendingSnapshot> pending;
    const auto previous = std::exchange(g_pendingSnapshot, &pending);
    const bool saved = g_saveGame(manager, type, slot, name);
    g_pendingSnapshot = previous;
    return saved;
}

bool on_write_save_file(void* manager, void* stream, void* description, int replacementSlot)
{
    if (!g_pendingSnapshot)
        return g_writeSaveFile(manager, stream, description, replacementSlot);
    if (!*g_pendingSnapshot)
    {
        log_message("Save blocked because no snapshot was captured");
        return false;
    }

    const auto& pending = **g_pendingSnapshot;
    try
    {
        // Commit the new version before the writer can touch the old .whs.
        g_db->Save(pending.version, pending.cache);
    }
    catch (const std::exception& e)
    {
        log_message("Save data write failed", e.what());
        return false;
    }

    const bool saved = g_writeSaveFile(manager, stream, description, replacementSlot);
    try
    {
        if (saved)
        {
            const auto detail = pending.filename + " (" + std::to_string(pending.cache.size()) + " entries)";
            log_message("Saved snapshot", detail.c_str());
        }
        else
        {
            g_db->DiscardSnapshot(pending.version);
            log_message("Discarded snapshot after failed save", pending.filename.c_str());
        }
    }
    catch (const std::exception& e)
    {
        log_message(saved ? "Could not log saved snapshot" : "Could not discard unused snapshot", e.what());
    }
    return saved;
}

bool install_save_hooks()
{
    const auto initialized = MH_Initialize();
    if (initialized != MH_OK)
    {
        log_message("Could not initialize save hook", MH_StatusToString(initialized));
        return false;
    }

    auto* saveGame = g_gameModule + kSaveGameRva;
    auto* writeSaveFile = g_gameModule + kWriteSaveFileRva;
    void* originalSaveGame = nullptr;
    void* originalWriteSaveFile = nullptr;
    auto status = MH_CreateHook(saveGame, reinterpret_cast<void*>(on_save_game), &originalSaveGame);
    if (status == MH_OK)
        status = MH_CreateHook(writeSaveFile, reinterpret_cast<void*>(on_write_save_file), &originalWriteSaveFile);
    if (status == MH_OK)
    {
        g_saveGame = reinterpret_cast<SaveGame>(originalSaveGame);
        g_writeSaveFile = reinterpret_cast<WriteSaveFile>(originalWriteSaveFile);
        status = MH_EnableHook(MH_ALL_HOOKS);
    }
    if (status == MH_OK)
        return true;

    log_message("Could not install save hook", MH_StatusToString(status));
    MH_Uninitialize();
    return false;
}

bool register_lua()
{
    auto* table = create_table();
    ScriptAny api;
    hold_table(api, table);
    static constexpr ScriptFunctionDesc functions[] = {
        {"Set", "key, value", "LuaDB", {}, 0, access<Access::Set>},
        {"Get", "key", "LuaDB", {}, 0, access<Access::Get>},
        {"Del", "key", "LuaDB", {}, 0, access<Access::Del>},
        {"Exi", "key", "LuaDB", {}, 0, access<Access::Exi>},
        {"All", "", "LuaDB", {}, 0, access<Access::All>},
        {"SetG", "key, value", "LuaDB", {}, 0, access<Access::Set, true>},
        {"GetG", "key", "LuaDB", {}, 0, access<Access::Get, true>},
        {"DelG", "key", "LuaDB", {}, 0, access<Access::Del, true>},
        {"ExiG", "key", "LuaDB", {}, 0, access<Access::Exi, true>},
        {"AllG", "", "LuaDB", {}, 0, access<Access::All, true>},
        {"Dump", "", "LuaDB", {}, 0, dump},
    };
    const auto add = method<bool(*)(void*, const ScriptFunctionDesc&)>(table, 0xb0);
    for (const auto& function : functions)
    {
        if (!add(table, function))
            return false;
    }
    method<void(*)(void*, const char*, const ScriptAny&)>(g_scriptSystem, 0xf0)(
        g_scriptSystem, "LuaDB", api);
    return method<bool(*)(void*, const char*, std::size_t, const char*, void*)>(g_scriptSystem, 0x30)(
        g_scriptSystem, db_lua, std::strlen(db_lua), "kcd1db/db.lua", nullptr);
}

class FrameworkListener
{
public:
    virtual ~FrameworkListener() = default;
    virtual void OnPostUpdate(float)
    {
        try
        {
            g_db->FlushGlobals();
        }
        catch (const std::exception& e)
        {
            log_message("Global flush failed", e.what());
        }
    }
    virtual void OnSaveGame(void* save)
    {
        if (!g_pendingSnapshot || *static_cast<std::byte**>(save) != g_gameModule + kSaveHelperVtableRva)
            return;

        try
        {
            const auto filename = method<const char*(*)(void*)>(save, 0x40)(save);
            auto pending = PendingSnapshot{filename, new_snapshot_version(), g_db->All(false)};
            method<void(*)(void*, const char*, const char*)>(save, 0x18)(
                save, kSnapshotMetadata, pending.version.c_str());
            *g_pendingSnapshot = std::move(pending);
        }
        catch (const std::exception& e)
        {
            log_message("Save data capture failed", e.what());
        }
    }
    virtual void OnLoadGame(void* save)
    {
        try
        {
            const auto filename = method<const char*(*)(void*)>(save, 0x48)(save);
            const auto version = *static_cast<std::byte**>(save) == g_gameModule + kLoadHelperVtableRva
                ? method<const char*(*)(void*, const char*)>(save, 0x20)(save, kSnapshotMetadata)
                : nullptr;
            const auto count = g_db->Load(version ? version : "");
            const auto detail = std::string(filename ? filename : "") + " (" + std::to_string(count) + " entries)";
            log_message("Loaded save", detail.c_str());
        }
        catch (const std::exception& e)
        {
            log_message("Save data load failed", e.what());
        }
    }
    virtual void OnLevelEnd(const char*) {}
    virtual void OnActionEvent(const void*) {}
    virtual void OnPreRender() {}
    virtual void OnSavegameFileLoadedInMemory(const char*) {}
    virtual void OnForceLoadingWithFlash() {}
};

FrameworkListener g_listener;

bool on_complete_init(void* framework)
{
    try
    {
        // CCryAction uses this same IScriptSystem member to run scripts/main.lua.
        g_scriptSystem = *reinterpret_cast<void**>(static_cast<std::byte*>(framework) + 0x50);
        if (!g_scriptSystem ||
            *static_cast<std::byte**>(g_scriptSystem) != g_gameModule + kScriptSystemVtableRva)
        {
            log_message("Unsupported script system");
        }
        else if (install_save_hooks())
        {
            const auto dbPath = (g_root / kDatabaseName).u8string();
            g_db = std::make_unique<LuaDB>(std::string(dbPath.begin(), dbPath.end()));
            if (register_lua())
            {
                method<void(*)(void*, FrameworkListener*, const char*, int)>(framework, 0x338)(
                    framework, &g_listener, "kcd1db", 0);
                g_saveListenerRegistered = true;
                log_message("LuaDB and DB registered");
            }
            else
            {
                log_message("Lua registration failed");
            }
        }
    }
    catch (const std::exception& e)
    {
        log_message("Initialization failed", e.what());
    }
    return game_function<CompleteInit>(kCompleteInitRva)(framework);
}

void on_shutdown(void* framework)
{
    game_function<Shutdown>(kShutdownRva)(framework);
    if (g_db)
    {
        try
        {
            g_db->FlushGlobals(true);
        }
        catch (const std::exception& e)
        {
            log_message("Final global flush failed", e.what());
        }
    }
}

bool replace_slot(std::size_t slotRva, std::size_t oldRva, void* hook)
{
    auto* slot = reinterpret_cast<void**>(g_gameModule + slotRva);
    auto* original = g_gameModule + oldRva;
    DWORD protection = 0;
    if (!VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &protection))
        return false;

    const auto previous = InterlockedCompareExchangePointer(slot, hook, original);
    DWORD unused = 0;
    VirtualProtect(slot, sizeof(*slot), protection, &unused);
    return previous == original;
}

void install_hooks(HMODULE game)
{
    g_gameModule = reinterpret_cast<std::byte*>(game);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(game);
    const auto* pe = reinterpret_cast<const IMAGE_NT_HEADERS64*>(g_gameModule + dos->e_lfanew);
    if (pe->FileHeader.TimeDateStamp != kImageTimestamp || pe->OptionalHeader.SizeOfImage != kImageSize)
    {
        log_message("Unsupported WHGame.dll");
        return;
    }
    if (!replace_slot(kCompleteInitSlotRva, kCompleteInitRva, reinterpret_cast<void*>(on_complete_init)))
    {
        log_message("Could not install framework initialization hook");
        return;
    }
    if (!replace_slot(kShutdownSlotRva, kShutdownRva, reinterpret_cast<void*>(on_shutdown)))
        log_message("Could not install framework shutdown hook");
    log_message("Framework initialization hook installed");
}

DWORD WINAPI wait_for_game(void*)
{
    for (int i = 0; i < 600; ++i)
    {
        if (const auto game = GetModuleHandleW(L"WHGame.dll"))
        {
            install_hooks(game);
            return 0;
        }
        Sleep(10);
    }
    log_message("WHGame.dll did not load");
    return 0;
}
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        wchar_t exe[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(nullptr, exe, MAX_PATH);
        if (!length || length == MAX_PATH)
            return TRUE;
        const std::filesystem::path exePath(exe);
        if (_wcsicmp(exePath.filename().c_str(), L"KingdomCome.exe") != 0)
            return TRUE;
        g_root = exePath.parent_path().parent_path().parent_path();

        if (const auto game = GetModuleHandleW(L"WHGame.dll"))
        {
            install_hooks(game);
        }
        else if (const auto thread = CreateThread(nullptr, 0, wait_for_game, nullptr, 0, nullptr))
        {
            CloseHandle(thread);
        }
    }
    return TRUE;
}
