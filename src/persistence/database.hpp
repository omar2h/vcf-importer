#pragma once
struct sqlite3;
namespace vcf
{
    struct DatabaseConfig;

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