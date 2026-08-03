#include <gtest/gtest.h>
#include <filesystem>

#include <vcf/vcf_parser.hpp>
namespace
{
    std::filesystem::path dataPath(std::string_view filename)
    {
        return std::filesystem::path(TEST_DATA_DIR) / filename;
    }
}

TEST(VcfParserTest, ParsesSingleVariant)
{
    vcf::VcfParser parser(dataPath("basic.vcf"));

    vcf::Variant variant;

    ASSERT_TRUE(parser.readNextVariant(variant));

    EXPECT_EQ(variant.chromosome, "1");
    EXPECT_EQ(variant.position, 100);
    EXPECT_EQ(variant.referenceAllele, "A");

    ASSERT_EQ(variant.alternateAlleles.size(), 1u);
    EXPECT_EQ(variant.alternateAlleles.front(), "G");

    EXPECT_FALSE(parser.readNextVariant(variant));
}

TEST(VcfParserTest, ParsesMultipleVariants)
{
    vcf::VcfParser parser(dataPath("multiple_variants.vcf"));

    vcf::Variant variant;

    ASSERT_TRUE(parser.readNextVariant(variant));
    EXPECT_EQ(variant.position, 100);
    EXPECT_EQ(variant.referenceAllele, "A");

    ASSERT_TRUE(parser.readNextVariant(variant));
    EXPECT_EQ(variant.position, 200);
    EXPECT_EQ(variant.referenceAllele, "C");

    EXPECT_FALSE(parser.readNextVariant(variant));
}

TEST(VcfParserTest, ThrowsWhenFileCannotBeOpened)
{
    EXPECT_THROW(vcf::VcfParser(dataPath("missing.vcf")), std::runtime_error);
}

TEST(VcfParserTest, ThrowsForMalformedVariant)
{
    vcf::VcfParser parser(dataPath("malformed_variant.vcf"));
    vcf::Variant variant;

    EXPECT_THROW(parser.readNextVariant(variant), std::runtime_error);
}

TEST(VcfParserTest, ExposesParsedHeader)
{
    vcf::VcfParser parser(dataPath("info_fields.vcf"));

    const auto &header = parser.header();

    EXPECT_FALSE(header.infoDefinitions.empty());
    EXPECT_FALSE(header.formatDefinitions.empty());
}