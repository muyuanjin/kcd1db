#pragma once

#include <SQLiteCpp/SQLiteCpp.h>

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

class LuaDB
{
public:
    using Value = std::variant<bool, float, std::string>;
    using Cache = std::unordered_map<std::string, Value>;

    explicit LuaDB(const std::string& path);
    void Set(bool global, std::string key, Value value);
    std::optional<Value> Get(bool global, const std::string& key);
    bool Del(bool global, const std::string& key);
    Cache All(bool global);

    void Save(const std::string& version, const Cache& snapshot);
    void DiscardSnapshot(const std::string& version);
    std::size_t Load(std::string version);
    void FlushGlobals(bool force = false);

private:
    Cache& GetCache(bool global);
    Cache ReadSnapshot(const std::string& file);
    void WriteSnapshot(const std::string& file, const Cache& cache);
    void DeleteSnapshot(const std::string& file);

    SQLite::Database m_db;
    std::mutex m_mutex;
    Cache m_saveCache;
    Cache m_globalCache;
    std::string m_pendingVersion;
    bool m_globalDirty = false;
    std::chrono::steady_clock::time_point m_lastFlush = std::chrono::steady_clock::now();
};
