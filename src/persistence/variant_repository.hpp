#pragma once

namespace vcf
{
    class Database;
    struct Variant;
    class VariantRepository
    {
    public:
        explicit VariantRepository(Database &database);

        void initializeSchema();

        void insert(const Variant &variant);

    private:
        Database &m_database;
    };
}