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

    EXPECT_THROW(static_cast<void>(parser.readNextVariant(variant)), std::runtime_error);
}

TEST(VcfParserTest, ExposesParsedHeader)
{
    vcf::VcfParser parser(dataPath("info_fields.vcf"));

    const auto &header = parser.header();

    EXPECT_FALSE(header.infoDefinitions.empty());
    EXPECT_FALSE(header.formatDefinitions.empty());
}

TEST(VcfParserTest, ParsesMultipleAlternateAlleles)
{
    vcf::VcfParser parser(dataPath("multiple_alternate_alleles.vcf"));

    vcf::Variant variant;

    ASSERT_TRUE(parser.readNextVariant(variant));
    ASSERT_EQ(variant.alternateAlleles.size(), 3u);

    EXPECT_EQ(variant.alternateAlleles[0], "G");
    EXPECT_EQ(variant.alternateAlleles[1], "T");
    EXPECT_EQ(variant.alternateAlleles[2], "C");
}

TEST(VcfParserTest, ParsesInfoFields)
{
    vcf::VcfParser parser(dataPath("info_fields.vcf"));

    vcf::Variant variant;

    ASSERT_TRUE(parser.readNextVariant(variant));
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

TEST(VcfParserTest, ParsesMultipleSamples)
{
    vcf::VcfParser parser(dataPath("multiple_samples.vcf"));

    vcf::Variant variant;
    ASSERT_TRUE(parser.readNextVariant(variant));

    ASSERT_EQ(parser.header().sampleNames.size(), 2u);
    EXPECT_EQ(parser.header().sampleNames[0], "SAMPLE_A");
    EXPECT_EQ(parser.header().sampleNames[1], "SAMPLE_B");

    ASSERT_EQ(variant.samples.size(), 2u);

    ASSERT_EQ(variant.samples[0].formatEntries.size(), 2u);
    EXPECT_EQ(variant.samples[0].formatEntries[0].key, "GT");
    EXPECT_EQ(variant.samples[0].formatEntries[0].values[0], "0/1");
    EXPECT_EQ(variant.samples[0].formatEntries[1].key, "DP");
    EXPECT_EQ(variant.samples[0].formatEntries[1].values[0], "10");

    ASSERT_EQ(variant.samples[1].formatEntries.size(), 2u);
    EXPECT_EQ(variant.samples[1].formatEntries[0].values[0], "1/1");
    EXPECT_EQ(variant.samples[1].formatEntries[1].values[0], "25");
}

TEST(VcfParserTest, RejectsMissingSampleColumn)
{
    vcf::VcfParser parser{dataPath("missing_sample_column.vcf")};

    vcf::Variant variant;

    EXPECT_THROW(static_cast<void>(parser.readNextVariant(variant)), std::runtime_error);
}

TEST(VcfParserTest, RejectsUnknownInfoField)
{
    vcf::VcfParser parser(dataPath("unknown_info.vcf"));
    vcf::Variant variant;
    EXPECT_THROW(static_cast<void>(parser.readNextVariant(variant)), std::runtime_error);
}

TEST(VcfParserTest, RejectsUnknownFormatField)
{
    vcf::VcfParser parser(dataPath("unknown_format.vcf"));
    vcf::Variant variant;
    EXPECT_THROW(static_cast<void>(parser.readNextVariant(variant)), std::runtime_error);
}

TEST(VcfParserTest, RejectsMalformedHeader)
{
    EXPECT_THROW(vcf::VcfParser parser(dataPath("malformed_header.vcf")), std::runtime_error);
}

TEST(VcfParserTest, RejectsMalformedVariant)
{
    vcf::VcfParser parser(dataPath("malformed_variant.vcf"));
    vcf::Variant variant;
    EXPECT_THROW(static_cast<void>(parser.readNextVariant(variant)), std::runtime_error);
}

TEST(VcfParserTest, MissingFileFormat)
{
    EXPECT_THROW(vcf::VcfParser parser(dataPath("missing_fileformat.vcf")), std::runtime_error);
}