#include <gtest/gtest.h>
#include <cstdlib>

#include <config/database_config.hpp>

namespace
{
    constexpr auto databasePathEnv = "VCF_DATABASE_PATH";
}

TEST(DatabaseConfigTest, LoadsDatabasePathFromEnvironment)
{
    setenv(databasePathEnv, "test.db", 1);

    auto config = vcf::loadDatabaseConfigFromEnvironment();

    EXPECT_EQ(config.databasePath, "test.db");

    unsetenv(databasePathEnv);
}

TEST(DatabaseConfigTest, RejectsMissingDatabasePathEnvironmentVariable)
{
    unsetenv(databasePathEnv);

    EXPECT_THROW(vcf::loadDatabaseConfigFromEnvironment(), std::runtime_error);
}