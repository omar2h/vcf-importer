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

    vcf::Variant first;
    vcf::Variant second;

    ASSERT_TRUE(parser.readNextVariant(first));
    ASSERT_TRUE(parser.readNextVariant(second));
    EXPECT_FALSE(parser.readNextVariant(second));

    EXPECT_EQ(first.position, 100);
    EXPECT_EQ(second.position, 200);

    EXPECT_EQ(first.referenceAllele, "A");
    EXPECT_EQ(second.referenceAllele, "C");
}

TEST(VcfParserTest, ParsesLargeFile)
{
    vcf::VcfParser parser(dataPath("/home/omar/datasets/assignment.final.vcf"));

    vcf::Variant variant;
    while (parser.readNextVariant(variant))
        ;

    EXPECT_TRUE(true);
}