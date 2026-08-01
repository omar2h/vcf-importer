#pragma once

namespace vcf
{
    struct DatabaseConfig;
    struct sqlite3;
    class Database
    {
    public:
        explicit Database(const DatabaseConfig &config);
        ~Database();

        Database(const Database &) = delete;
        Database &operator=(const Database &) = delete;

        Database(Database &&) = delete;
        Database &operator=(Database &&) = delete;

        sqlite3 *connection() const;

    private:
        sqlite3 *m_connection{};
    };
}