#include <gtest/gtest.h>
#include <sqlite3.h>

#include <config/database_config.hpp>
#include <persistence/database.hpp>
#include <persistence/variant_repository.hpp>
#include <domain/variant.hpp>

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

std::string_view columnText(sqlite3_stmt *statement, int column)
{
    return std::string_view(reinterpret_cast<const char *>(sqlite3_column_text(statement, column)));
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

TEST(VariantRepositoryTest, InsertsVariant)
{
    const auto databasePath = std::filesystem::temp_directory_path() / "database_test.db";
    std::filesystem::remove(databasePath);

    vcf::DatabaseConfig databaseConfig;
    databaseConfig.databasePath = databasePath;

    vcf::Database database(databaseConfig);

    vcf::VariantRepository variantRepository(database);
    variantRepository.initializeSchema();

    vcf::Variant variant{};
    variant.chromosome = "1";
    variant.position = 100;
    variant.referenceAllele = "A";
    variant.alternateAlleles = {"T"};

    variantRepository.insert(variant);

    constexpr auto sql = R"(
        SELECT chromosome, position, ref, alt
        FROM variants;
    )";

    sqlite3_stmt *statement = nullptr;
    const int rc = sqlite3_prepare_v2(database.connection(), sql, -1, &statement, nullptr);

    ASSERT_EQ(rc, SQLITE_OK);

    const int stepResult = sqlite3_step(statement);

    ASSERT_EQ(stepResult, SQLITE_ROW);

    const auto *chromosome = reinterpret_cast<const char *>(sqlite3_column_text(statement, 0));
    EXPECT_EQ(columnText(statement, 0), "1");

    EXPECT_EQ(sqlite3_column_int64(statement, 1), 100);

    const auto *reference = reinterpret_cast<const char *>(sqlite3_column_text(statement, 2));
    EXPECT_EQ(columnText(statement, 2), "A");

    const auto *alternate = reinterpret_cast<const char *>(sqlite3_column_text(statement, 3));
    EXPECT_EQ(columnText(statement, 3), "T");

    EXPECT_EQ(sqlite3_step(statement), SQLITE_DONE);

    sqlite3_finalize(statement);

    std::filesystem::remove(databasePath);
}