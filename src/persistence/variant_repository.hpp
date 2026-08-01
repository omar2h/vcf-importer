#pragma once

namespace vcf
{
    class Database;
    class VariantRepository
    {
    public:
        explicit VariantRepository(Database &database);

        void initializeSchema();

    private:
        Database &m_database;
    };
}