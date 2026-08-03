#pragma once

namespace vcf
{
    class Database;

    class DatabaseTransaction
    {
    public:
        explicit DatabaseTransaction(Database &database);
        ~DatabaseTransaction() noexcept;

        DatabaseTransaction(const DatabaseTransaction &) = delete;
        DatabaseTransaction &operator=(const DatabaseTransaction &) = delete;

        DatabaseTransaction(DatabaseTransaction &&) = delete;
        DatabaseTransaction &operator=(DatabaseTransaction &&) = delete;

        void commit();

    private:
        Database &m_database;
        bool m_active{true};
    };
}