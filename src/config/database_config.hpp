#pragma once

#include <filesystem>

namespace vcf
{
    struct DatabaseConfig
    {
        std::filesystem::path databasePath;
    };
    DatabaseConfig loadDatabaseConfigFromEnvironment();
}