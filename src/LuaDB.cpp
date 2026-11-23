#include "LuaDB.h"

#include <charconv>
#include <stdexcept>

namespace
{
// Keep the Store type values used by LuaDB on disk.
enum StoreType { Boolean = 2, Number = 4, String = 5 };

std::pair<StoreType, std::string> encode(const LuaDB::Value& value)
{
    if (const auto* boolean = std::get_if<bool>(&value))
        return {Boolean, *boolean ? "1" : "0"};
    if (const auto* number = std::get_if<float>(&value))
    {
        char text[48];
        const auto result = std::to_chars(text, text + sizeof(text), *number);
        if (result.ec != std::errc{})
            throw std::runtime_error("Could not encode Lua number");
        return {Number, std::string(text, result.ptr)};
    }
    return {String, std::get<std::string>(value)};
}
}

LuaDB::LuaDB(const std::string& path)
    : m_db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    m_db.exec(R"sql(
        CREATE TABLE IF NOT EXISTS Store (
            key TEXT NOT NULL,
            savefile TEXT NOT NULL,
            type INTEGER,
            value TEXT,
            created_at INTEGER DEFAULT CURRENT_TIMESTAMP,
            updated_at INTEGER DEFAULT CURRENT_TIMESTAMP,
            UNIQUE (key, savefile)
        );
        CREATE INDEX IF NOT EXISTS Store_savefile ON Store (savefile);
    )sql");
    m_globalCache = ReadSnapshot("");
}

void LuaDB::Set(bool global, std::string key, Value value)
{
    std::lock_guard lock(m_mutex);
    auto& cache = global ? m_globalCache : m_saveCache;
    cache.insert_or_assign(std::move(key), std::move(value));
    m_globalDirty |= global;
}

std::optional<LuaDB::Value> LuaDB::Get(bool global, const std::string& key) const
{
    std::lock_guard lock(m_mutex);
    const auto& cache = global ? m_globalCache : m_saveCache;
    const auto found = cache.find(key);
    if (found == cache.end())
        return std::nullopt;
    return found->second;
}

bool LuaDB::Del(bool global, const std::string& key)
{
    std::lock_guard lock(m_mutex);
    auto& cache = global ? m_globalCache : m_saveCache;
    const bool removed = cache.erase(key) != 0;
    m_globalDirty |= global && removed;
    return removed;
}

LuaDB::Cache LuaDB::All(bool global) const
{
    std::lock_guard lock(m_mutex);
    return global ? m_globalCache : m_saveCache;
}

void LuaDB::Save(const std::string& version, const Cache& snapshot)
{
    std::lock_guard lock(m_mutex);
    WriteSnapshot(version, snapshot);
}

std::size_t LuaDB::Load(const std::string& version)
{
    std::lock_guard lock(m_mutex);
    m_saveCache.clear();
    if (!version.empty())
        m_saveCache = ReadSnapshot(version);
    return m_saveCache.size();
}

void LuaDB::FlushGlobals(bool force)
{
    std::lock_guard lock(m_mutex);
    const auto now = std::chrono::steady_clock::now();
    if (!m_globalDirty || (!force && now - m_lastFlush < std::chrono::seconds(1)))
        return;

    m_lastFlush = now;
    WriteSnapshot("", m_globalCache);
    m_globalDirty = false;
}

LuaDB::Cache LuaDB::ReadSnapshot(const std::string& file)
{
    Cache cache;
    SQLite::Statement query(m_db, "SELECT key, type, value FROM Store WHERE savefile = ?");
    query.bind(1, file);
    while (query.executeStep())
    {
        auto key = query.getColumn(0).getString();
        auto value = query.getColumn(2).getString();
        switch (query.getColumn(1).getInt())
        {
        case Boolean:
            cache.emplace(std::move(key), value == "1");
            break;
        case Number:
        {
            float number = 0;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
            if (result.ec == std::errc{} && result.ptr == value.data() + value.size())
                cache.emplace(std::move(key), number);
            break;
        }
        case String:
            cache.emplace(std::move(key), std::move(value));
            break;
        }
    }
    return cache;
}

void LuaDB::WriteSnapshot(const std::string& file, const Cache& cache)
{
    SQLite::Transaction transaction(m_db);
    SQLite::Statement remove(m_db, "DELETE FROM Store WHERE savefile = ?");
    remove.bind(1, file);
    remove.exec();

    SQLite::Statement insert(m_db, "INSERT INTO Store (key, savefile, type, value) VALUES (?, ?, ?, ?)");
    insert.bind(2, file);
    for (const auto& [key, value] : cache)
    {
        const auto [type, encoded] = encode(value);
        insert.bind(1, key);
        insert.bind(3, static_cast<int>(type));
        insert.bind(4, encoded);
        insert.exec();
        insert.reset();
    }
    transaction.commit();
}
