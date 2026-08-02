#include <iostream>
#include <config/database_config.hpp>
#include <persistence/database.hpp>
#include <persistence/variant_repository.hpp>
#include <vcf/vcf_parser.hpp>

void printUsage();

int main(int argc, char *argv[])
{
    constexpr std::string_view VcfOption = "--vcf";
    if (argc != 3 || std::string_view(argv[1]) != VcfOption)
    {
        printUsage();
        return EXIT_FAILURE;
    }
    std::filesystem::path vcfPath{argv[2]};

    try
    {
        auto databaseConfig = vcf::loadDatabaseConfigFromEnvironment();

        vcf::Database database(databaseConfig);

        vcf::VariantRepository repository(database);

        repository.initializeSchema();

        vcf::VcfParser parser;
        auto result = parser.parse(vcfPath);

        for (const auto &variant : result.variants)
        {
            repository.insert(variant, result.header);
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception &exception)
    {
        std::cerr << exception.what() << '\n';
        return EXIT_FAILURE;
    }
}

void printUsage()
{
    std::cerr << "Usage:\n"
              << "    vcf_importer --vcf <path>\n";
}