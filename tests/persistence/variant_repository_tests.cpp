#include <gtest/gtest.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

#include <config/database_config.hpp>
#include <vcf/vcf_parser.hpp>
#include <persistence/database.hpp>
#include <persistence/variant_repository.hpp>
#include <domain/variant.hpp>
#include <domain/vcf_header.hpp>

std::filesystem::path dataPath(std::string_view filename)
{
    return std::filesystem::path(TEST_DATA_DIR) / filename;
}

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

nlohmann::json readVariantData(sqlite3 *connection)
{
    constexpr auto sql = R"(
        SELECT data
        FROM variants;
    )";

    sqlite3_stmt *statement = nullptr;

    const int rc = sqlite3_prepare_v2(connection, sql, -1, &statement, nullptr);

    if (rc != SQLITE_OK)
        throw std::runtime_error("Failed to prepare data query");

    EXPECT_EQ(rc, SQLITE_OK);

    const int stepResult = sqlite3_step(statement);

    if (stepResult != SQLITE_ROW)
    {
        sqlite3_finalize(statement);
        throw std::runtime_error("Variant row not found");
    }

    const auto *text = sqlite3_column_text(statement, 0);

    if (!text)
    {
        sqlite3_finalize(statement);
        throw std::runtime_error("Variant data is null");
    }

    auto data = nlohmann::json::parse(text);

    const int secondStep = sqlite3_step(statement);

    if (secondStep != SQLITE_DONE)
    {
        sqlite3_finalize(statement);
        throw std::runtime_error("Expected exactly one variant row");
    }

    sqlite3_finalize(statement);

    return data;
}

class VariantRepositoryTest : public ::testing::Test
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

TEST_F(VariantRepositoryTest, CreatesVariantsTable)
{

    vcf::Database database(databaseConfig);

    vcf::VariantRepository repository(database);
    repository.initializeSchema();

    EXPECT_TRUE(objectExists(database.connection(), "table", "variants"));
}

TEST_F(VariantRepositoryTest, CreatesVariantsIndex)
{
    vcf::Database database(databaseConfig);
    vcf::VariantRepository repository(database);

    repository.initializeSchema();

    EXPECT_FALSE(objectExists(database.connection(), "index", "idx_variants_chromosome_position"));

    repository.createIndex();

    EXPECT_TRUE(objectExists(database.connection(), "index", "idx_variants_chromosome_position"));
}

TEST_F(VariantRepositoryTest, InsertBeforeInitializationThrows)
{
    vcf::Database database(databaseConfig);
    vcf::VariantRepository repository(database);

    vcf::Variant variant;
    vcf::VcfHeader header;

    EXPECT_THROW(repository.insert(variant, header), std::logic_error);
}

TEST_F(VariantRepositoryTest, InsertsVariant)
{
    vcf::Database database(databaseConfig);

    vcf::VariantRepository repository(database);
    repository.initializeSchema();

    vcf::Variant variant{};
    variant.chromosome = "1";
    variant.position = 100;
    variant.referenceAllele = "A";
    variant.alternateAlleles = {"T"};

    vcf::VcfHeader header{};
    repository.insert(variant, header);

    constexpr auto sql = R"(
        SELECT chromosome, position, ref, alt
        FROM variants;
    )";

    sqlite3_stmt *statement = nullptr;
    const int rc = sqlite3_prepare_v2(database.connection(), sql, -1, &statement, nullptr);

    ASSERT_EQ(rc, SQLITE_OK);

    const int stepResult = sqlite3_step(statement);

    ASSERT_EQ(stepResult, SQLITE_ROW);

    EXPECT_EQ(columnText(statement, 0), "1");

    EXPECT_EQ(sqlite3_column_int64(statement, 1), 100);

    EXPECT_EQ(columnText(statement, 2), "A");

    EXPECT_EQ(columnText(statement, 3), "T");

    EXPECT_EQ(sqlite3_step(statement), SQLITE_DONE);

    sqlite3_finalize(statement);
}

TEST_F(VariantRepositoryTest, PreservesFormatValuesForMultipleSamples)
{
    vcf::Database database(databaseConfig);
    vcf::VcfParser parser(dataPath("multiple_samples.vcf"));

    vcf::Variant variant;
    ASSERT_TRUE(parser.readNextVariant(variant));

    vcf::VariantRepository repository(database);
    repository.initializeSchema();

    repository.insert(variant, parser.header());

    const auto json = readVariantData(database.connection());

    ASSERT_TRUE(json["FORMAT"].contains("SAMPLE_A"));
    ASSERT_TRUE(json["FORMAT"].contains("SAMPLE_B"));

    EXPECT_EQ(json["FORMAT"]["SAMPLE_A"]["GT"], "0/1");
    EXPECT_EQ(json["FORMAT"]["SAMPLE_A"]["DP"], 10);

    EXPECT_EQ(json["FORMAT"]["SAMPLE_B"]["GT"], "1/1");
    EXPECT_EQ(json["FORMAT"]["SAMPLE_B"]["DP"], 25);
}

TEST_F(VariantRepositoryTest, SerializesPassedFilter)
{
    vcf::Database database(databaseConfig);

    vcf::VariantRepository repository(database);
    repository.initializeSchema();

    vcf::Variant variant{};
    variant.chromosome = "1";
    variant.position = 100;
    variant.referenceAllele = "A";
    variant.alternateAlleles = {"T"};

    variant.filter.status = vcf::FilterStatus::Passed;

    vcf::VcfHeader header;

    repository.insert(variant, header);

    auto data = readVariantData(database.connection());

    EXPECT_EQ(data["FILTER"], "PASS");
}

TEST_F(VariantRepositoryTest, SerializesMissingFilter)
{
    vcf::Database database(databaseConfig);

    vcf::VariantRepository repository(database);
    repository.initializeSchema();

    vcf::Variant variant;
    variant.filter.status = vcf::FilterStatus::NotApplied;

    vcf::VcfHeader header;

    repository.insert(variant, header);

    auto data = readVariantData(database.connection());

    EXPECT_TRUE(data["FILTER"].is_null());
}

TEST_F(VariantRepositoryTest, SerializesMissingQuality)
{
    vcf::Database database(databaseConfig);

    vcf::VariantRepository repository(database);
    repository.initializeSchema();

    vcf::Variant variant;

    variant.quality.reset();

    vcf::VcfHeader header;

    repository.insert(variant, header);

    auto data = readVariantData(database.connection());

    EXPECT_TRUE(data["QUAL"].is_null());
}

TEST_F(VariantRepositoryTest, SerializesQuality)
{
    vcf::Database database(databaseConfig);

    vcf::VariantRepository repository(database);
    repository.initializeSchema();

    vcf::Variant variant;

    variant.quality = 42.5;

    vcf::VcfHeader header;

    repository.insert(variant, header);

    auto data = readVariantData(database.connection());

    EXPECT_EQ(data["QUAL"], 42.5);
}

TEST_F(VariantRepositoryTest, PersistsParsedVariantAsTypedJson)
{
    vcf::VcfParser parser(dataPath("complete_variant.vcf"));
    vcf::Database database(databaseConfig);

    vcf::VariantRepository repository(database);
    repository.initializeSchema();

    vcf::Variant variant;
    ASSERT_TRUE(parser.readNextVariant(variant));

    repository.insert(variant, parser.header());

    const auto json = readVariantData(database.connection());

    ASSERT_TRUE(json["FILTER"].is_string());
    EXPECT_EQ(json["FILTER"], "PASS");

    ASSERT_TRUE(json["QUAL"].is_number());
    EXPECT_DOUBLE_EQ(json["QUAL"].get<double>(), 60.5);

    ASSERT_TRUE(json["INFO"].is_object());

    ASSERT_TRUE(json["INFO"]["DP"].is_number_integer());
    EXPECT_EQ(json["INFO"]["DP"], 20);

    ASSERT_TRUE(json["INFO"]["AF"].is_number_float());
    EXPECT_DOUBLE_EQ(json["INFO"]["AF"].get<double>(), 0.25);

    ASSERT_TRUE(json["INFO"]["DB"].is_boolean());
    EXPECT_TRUE(json["INFO"]["DB"].get<bool>());

    ASSERT_TRUE(json["FORMAT"].is_object());

    EXPECT_EQ(json["FORMAT"]["SAMPLE_A"]["GT"], "0/1");
    EXPECT_EQ(json["FORMAT"]["SAMPLE_A"]["GQ"], 42);
}
