#include <gtest/gtest.h>
#include <sqlite3.h>
#include <filesystem>

#include <config/database_config.hpp>
#include <persistence/database.hpp>
#include <domain/variant.hpp>
#include <domain/vcf_header.hpp>
#include <persistence/variant_repository.hpp>
#include <persistence/database_transaction.hpp>

namespace
{
    vcf::Variant makeVariant()
    {
        vcf::Variant variant;
        variant.chromosome = "1";
        variant.position = 100;
        variant.referenceAllele = "A";
        variant.alternateAlleles = {"T"};
        return variant;
    }

    std::int64_t variantCount(sqlite3 *connection)
    {
        constexpr auto sql = "SELECT COUNT(*) FROM variants;";

        sqlite3_stmt *statement = nullptr;

        const int rc = sqlite3_prepare_v2(connection, sql, -1, &statement, nullptr);

        if (rc != SQLITE_OK)
        {
            throw std::runtime_error(
                std::string("Failed to prepare count query: ") +
                sqlite3_errmsg(connection));
        }

        const int stepResult = sqlite3_step(statement);

        if (stepResult != SQLITE_ROW)
        {
            const std::string error = sqlite3_errmsg(connection);
            sqlite3_finalize(statement);

            throw std::runtime_error(
                "Failed to read variant count: " + error);
        }

        const auto count = sqlite3_column_int64(statement, 0);

        sqlite3_finalize(statement);

        return count;
    }
}

class DatabaseTransactionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        databasePath = std::filesystem::temp_directory_path() / "database_test.db";
        std::filesystem::remove(databasePath);

        databaseConfig.databasePath = databasePath;
    }

    void TearDown() override
    {
        std::filesystem::remove(databasePath);
    }

    vcf::DatabaseConfig databaseConfig;
    std::filesystem::path databasePath;
};

TEST_F(DatabaseTransactionTest, CommitPersistsInsertedVariant)
{
    vcf::Database database(databaseConfig);
    vcf::VariantRepository repository(database);
    repository.initializeSchema();
    const auto variant = makeVariant();
    const vcf::VcfHeader header{};

    {

        vcf::DatabaseTransaction transaction(database);

        repository.insert(variant, header);

        transaction.commit();
    }

    EXPECT_EQ(variantCount(database.connection()), 1);
}

TEST_F(DatabaseTransactionTest, RollsBackWhenNotCommitted)
{
    vcf::Database database(databaseConfig);
    vcf::VariantRepository repository(database);
    repository.initializeSchema();
    const auto variant = makeVariant();
    const vcf::VcfHeader header{};

    {
        vcf::DatabaseTransaction transaction(database);

        repository.insert(variant, header);
    }

    EXPECT_EQ(variantCount(database.connection()), 0);
}