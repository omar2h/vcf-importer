#include <cstdlib>
#include <iostream>

#include <config/database_config.hpp>
#include <persistence/database.hpp>
#include <persistence/database_transaction.hpp>
#include <persistence/variant_repository.hpp>
#include <vcf/vcf_parser.hpp>

void printUsage();

int main(int argc, char *argv[])
{
    try
    {
        constexpr std::string_view VcfOption = "--vcf";
        if (argc != 3 || std::string_view(argv[1]) != VcfOption)
        {
            printUsage();
            return EXIT_FAILURE;
        }
        const std::filesystem::path vcfPath{argv[2]};
        vcf::VcfParser parser(vcfPath);
        vcf::Database database(vcf::loadDatabaseConfigFromEnvironment());
        vcf::VariantRepository repository(database);
        repository.initializeSchema();
        vcf::DatabaseTransaction transaction(database);
        vcf::Variant variant;

        std::size_t variantCount = 0;

        while (parser.readNextVariant(variant))
        {
            repository.insert(variant, parser.header());
            ++variantCount;
        }
        repository.createIndex();
        transaction.commit();

        std::cout << "Imported variants: " << variantCount << '\n';

        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}

void printUsage()
{
    std::cerr << "Usage:\n"
              << "    vcf_importer --vcf <path>\n";
}