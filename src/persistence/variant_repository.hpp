#pragma once

namespace vcf
{
    class Database;
    struct Variant;
    struct VcfHeader;
    class VariantRepository
    {
    public:
        explicit VariantRepository(Database &database);

        void initializeSchema();

        void insert(const Variant &variant, const VcfHeader &header);

    private:
        Database &m_database;
    };
}