#include <gtest/gtest.h>
#include <filesystem>

#include <vcf/vcf_parser.hpp>
class VcfParserTest : public ::testing::Test
{
protected:
    vcf::VcfParser parser;

    static std::filesystem::path dataPath(std::string_view filename)
    {
        return std::filesystem::path(TEST_DATA_DIR) / filename;
    }
};

TEST_F(VcfParserTest, ParsesSingleVariant)
{
    const auto result = parser.parse(dataPath("basic.vcf"));

    ASSERT_EQ(result.variants.size(), 1u);

    const auto &variant = result.variants.front();

    EXPECT_EQ(variant.chromosome, "1");
    EXPECT_EQ(variant.position, 100);
    EXPECT_EQ(variant.referenceAllele, "A");

    ASSERT_EQ(variant.alternateAlleles.size(), 1u);
    EXPECT_EQ(variant.alternateAlleles.front(), "G");
}

TEST_F(VcfParserTest, ParseMultipleVariants)
{
    const auto result = parser.parse(dataPath("multiple_variants.vcf"));

    ASSERT_EQ(result.variants.size(), 2u);

    EXPECT_EQ(result.variants[0].position, 100);
    EXPECT_EQ(result.variants[1].position, 200);

    EXPECT_EQ(result.variants[0].referenceAllele, "A");
    EXPECT_EQ(result.variants[1].referenceAllele, "C");
}

TEST_F(VcfParserTest, ParsesMultipleAlternateAlleles)
{
    const auto result = parser.parse(dataPath("multiple_alternate_alleles.vcf"));

    ASSERT_EQ(result.variants.size(), 1u);

    const auto &variant = result.variants.front();

    ASSERT_EQ(variant.alternateAlleles.size(), 3u);

    EXPECT_EQ(variant.alternateAlleles[0], "G");
    EXPECT_EQ(variant.alternateAlleles[1], "T");
    EXPECT_EQ(variant.alternateAlleles[2], "C");
}

TEST_F(VcfParserTest, ParsesInfoFields)
{
    const auto result = parser.parse(dataPath("info_fields.vcf"));

    ASSERT_EQ(result.variants.size(), 1u);

    const auto &variant = result.variants.front();

    ASSERT_EQ(variant.info.size(), 3u);

    const auto &dp = variant.info[0];
    EXPECT_EQ(dp.key, "DP");
    ASSERT_EQ(dp.values.size(), 1u);
    EXPECT_EQ(dp.values[0], "10");

    const auto &af = variant.info[1];
    EXPECT_EQ(af.key, "AF");
    ASSERT_EQ(af.values.size(), 1u);
    EXPECT_EQ(af.values[0], "0.5");

    const auto &db = variant.info[2];
    EXPECT_EQ(db.key, "DB");
    EXPECT_TRUE(db.values.empty());
}

TEST_F(VcfParserTest, RejectsUnknownInfoField)
{
    EXPECT_THROW(parser.parse(dataPath("unknown_info.vcf")), std::runtime_error);
}

TEST_F(VcfParserTest, RejectsUnknownFormatField)
{
    EXPECT_THROW(parser.parse(dataPath("unknown_format.vcf")), std::runtime_error);
}

TEST_F(VcfParserTest, RejectsMalformedHeader)
{
    EXPECT_THROW(parser.parse(dataPath("malformed_header.vcf")), std::runtime_error);
}

TEST_F(VcfParserTest, RejectsMalformedVariant)
{
    EXPECT_THROW(parser.parse(dataPath("malformed_variant.vcf")), std::runtime_error);
}

TEST_F(VcfParserTest, MissingFileFormat)
{
    EXPECT_THROW(parser.parse(dataPath("missing_fileformat.vcf")), std::runtime_error);
}