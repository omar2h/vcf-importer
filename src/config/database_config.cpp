#include "database_config.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace vcf
{

    namespace
    {
        constexpr auto databasePathEnv = "VCF_DATABASE_PATH";

        std::string getRequiredEnvironmentVariable(const char *name)
        {
            if (const char *value = std::getenv(name); value != nullptr)
            {
                return value;
            }

            throw std::runtime_error(std::string("Required environment variable '") + name + "' is not set.");
        }
    }

    DatabaseConfig loadDatabaseConfigFromEnvironment()
    {
        DatabaseConfig config;

        config.databasePath = getRequiredEnvironmentVariable(databasePathEnv);

        return config;
    }
}