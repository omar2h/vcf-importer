#include <gtest/gtest.h>
#include <sqlite3.h>

#include <config/database_config.hpp>
#include <persistence/database.hpp>
#include <persistence/variant_repository.hpp>

bool objectExists(sqlite3 *connection, std::string_view type, std::string_view name)
{
    constexpr auto sql = R"(
        SELECT name
        FROM sqlite_master
        WHERE type = ?
          AND name = ?;
    )";

    sqlite3_stmt *statement = nullptr;
    const int rc = sqlite3_prepare_v2(connection, sql, -1, &statement, nullptr);

    if (rc != SQLITE_OK)
        return false;

    const int bind1 = sqlite3_bind_text(statement, 1, type.data(), -1, SQLITE_TRANSIENT);
    const int bind2 = sqlite3_bind_text(statement, 2, name.data(), -1, SQLITE_TRANSIENT);

    if (bind1 != SQLITE_OK || bind2 != SQLITE_OK)
    {
        sqlite3_finalize(statement);
        return false;
    }

    const int stepResult = sqlite3_step(statement);

    sqlite3_finalize(statement);

    return stepResult == SQLITE_ROW;
}

TEST(VariantRepositoryTest, CreatesVariantsTable)
{

    const auto databasePath = std::filesystem::temp_directory_path() / "database_test.db";
    std::filesystem::remove(databasePath);

    vcf::DatabaseConfig databaseConfig;
    databaseConfig.databasePath = databasePath;

    vcf::Database database(databaseConfig);

    vcf::VariantRepository variantRepository(database);
    variantRepository.initializeSchema();

    EXPECT_TRUE(objectExists(database.connection(), "table", "variants"));

    std::filesystem::remove(databasePath);
}

TEST(VariantRepositoryTest, CreatesVariantsIndex)
{

    const auto databasePath = std::filesystem::temp_directory_path() / "database_test.db";
    std::filesystem::remove(databasePath);

    vcf::DatabaseConfig databaseConfig;
    databaseConfig.databasePath = databasePath;

    vcf::Database database(databaseConfig);

    vcf::VariantRepository variantRepository(database);
    variantRepository.initializeSchema();

    EXPECT_TRUE(objectExists(database.connection(), "index", "idx_variants_chromosome_position"));

    std::filesystem::remove(databasePath);
}