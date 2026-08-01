#include <gtest/gtest.h>

#include <persistence/database.hpp>
#include <config/database_config.hpp>

TEST(DatabaseTest, OpensDatabaseSuccessfully)
{
    const auto databasePath = std::filesystem::temp_directory_path() / "database_test.db";
    std::filesystem::remove(databasePath);

    vcf::DatabaseConfig databaseConfig;
    databaseConfig.databasePath = databasePath;

    vcf::Database database(databaseConfig);

    EXPECT_NE(database.connection(), nullptr);

    std::filesystem::remove(databasePath);
}

TEST(DatabaseTest, ThrowsWhenDatabaseCannotBeOpened)
{
    const auto databasePath = "/this/directory/does/not/exist/database.db";

    vcf::DatabaseConfig databaseConfig;
    databaseConfig.databasePath = databasePath;

    EXPECT_THROW({ vcf::Database{databaseConfig}; }, std::runtime_error);
}