#pragma once

struct sqlite3_stmt;

namespace vcf
{
    class Database;
    struct Variant;
    struct VcfHeader;
    class VariantRepository
    {
    public:
        explicit VariantRepository(Database &database);
        ~VariantRepository() noexcept;

        VariantRepository(const VariantRepository &) = delete;
        VariantRepository &operator=(const VariantRepository &) = delete;
        VariantRepository(VariantRepository &&) = delete;
        VariantRepository &operator=(VariantRepository &&) = delete;

        void initializeSchema();
        void insert(const Variant &variant, const VcfHeader &header);

    private:
        void prepareInsertStatement();
        void resetInsertStatement() noexcept;

        Database &m_database;
        sqlite3_stmt *m_insertStatement{nullptr};
    };
}