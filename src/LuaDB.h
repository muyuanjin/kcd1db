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
    std::optional<Value> Get(bool global, const std::string& key) const;
    bool Del(bool global, const std::string& key);
    Cache All(bool global) const;

    void Save(const std::string& version, const Cache& snapshot);
    std::size_t Load(const std::string& version);
    void FlushGlobals(bool force = false);

private:
    Cache ReadSnapshot(const std::string& file);
    void WriteSnapshot(const std::string& file, const Cache& cache);

    SQLite::Database m_db;
    mutable std::mutex m_mutex;
    Cache m_saveCache;
    Cache m_globalCache;
    bool m_globalDirty = false;
    std::chrono::steady_clock::time_point m_lastFlush = std::chrono::steady_clock::now();
};
