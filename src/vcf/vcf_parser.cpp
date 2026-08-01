#include "vcf_parser.hpp"

#include <stdexcept>
#include <fstream>

#include "../domain/vcf_header.hpp"

namespace vcf
{
    namespace
    {
        constexpr auto malformedHeader = "Malformed VCF header definition";
        constexpr auto malformedFilter = "Malformed FILTER field.";

        std::string_view extractDefinition(std::string_view line)
        {
            auto begin = line.find('<');
            auto end = line.rfind('>');
            if (begin >= end || begin == std::string_view::npos || end == std::string_view::npos)
                throw std::runtime_error(malformedHeader);

            // begin points to '<'
            // end points to '>'
            return line.substr(begin + 1, end - begin - 1);
        }

        std::string_view extractValue(std::string_view text, std::string_view key)
        {
            auto begin = text.find(key);
            if (begin == std::string_view::npos)
                throw std::runtime_error(malformedHeader);
            begin += key.size();

            auto end = text.find(',', begin);
            if (end == std::string_view::npos)
                end = text.size();

            // begin points to first char
            // end points to , or npos
            return text.substr(begin, end - begin);
        }

        FieldType parseFieldType(std::string_view type)
        {
            if (type == "Integer")
                return FieldType::Integer;
            if (type == "Float")
                return FieldType::Float;
            if (type == "String")
                return FieldType::String;
            if (type == "Character")
                return FieldType::Character;
            if (type == "Flag")
                return FieldType::Flag;
            throw std::runtime_error("Unknown field type: " + std::string(type));
        }

        FieldDefinition parseFieldDefinition(std::string_view definition)
        {
            return FieldDefinition{
                .id = std::string(extractValue(definition, "ID=")),
                .number = std::string(extractValue(definition, "Number=")),
                .type = parseFieldType(extractValue(definition, "Type="))};
        }

        void parseMetaLine(std::string_view line, VcfHeader &header)
        {
            if (!line.starts_with("##INFO") && !line.starts_with("##FORMAT"))
                return;

            std::string_view definitionText = extractDefinition(line);
            auto definition = parseFieldDefinition(definitionText);

            if (line.starts_with("##INFO"))
                header.infoDefinitions[definition.id] = std::move(definition);
            else
                header.formatDefinitions[definition.id] = std::move(definition);
        }

        std::vector<std::string_view> split(std::string_view text, char delimiter)
        {
            std::vector<std::string_view> fields;

            std::size_t start = 0;
            while (true)
            {
                auto end = text.find(delimiter, start);
                if (end == std::string_view::npos)
                {
                    fields.push_back(text.substr(start));
                    break;
                }
                std::string_view field = text.substr(start, end - start);
                fields.push_back(field);
                start = end + 1;
            }

            return fields;
        }

        void parseColumnHeader(std::string_view line, VcfHeader &header)
        {
            auto fields = split(line, '\t');

            if (fields.size() < 8)
                throw std::runtime_error(malformedHeader);

            if (fields[0] != "#CHROM")
                throw std::runtime_error(malformedHeader);
            if (fields[1] != "POS")
                throw std::runtime_error(malformedHeader);
            if (fields[2] != "ID")
                throw std::runtime_error(malformedHeader);
            if (fields[3] != "REF")
                throw std::runtime_error(malformedHeader);
            if (fields[4] != "ALT")
                throw std::runtime_error(malformedHeader);
            if (fields[5] != "QUAL")
                throw std::runtime_error(malformedHeader);
            if (fields[6] != "FILTER")
                throw std::runtime_error(malformedHeader);
            if (fields[7] != "INFO")
                throw std::runtime_error(malformedHeader);

            if (fields.size() > 8)
            {
                if (fields[8] != "FORMAT")
                    throw std::runtime_error(malformedHeader);
                for (size_t i = 9; i < fields.size(); i++)
                {
                    header.sampleNames.push_back(std::string(fields[i]));
                }
            }
        }

        Filter parseFilter(std::string_view field)
        {
            if (field == ".")
                return Filter{.status = FilterStatus::NotApplied};

            if (field == "PASS")
                return Filter{.status = FilterStatus::Passed};

            Filter filter;
            filter.status = FilterStatus::Failed;

            const auto ids = split(field, ';');

            for (const auto &id : ids)
            {
                if (id == "PASS" || id == "." || id.empty())
                    throw std::runtime_error(malformedFilter);
                filter.failedFilterIds.push_back(std::string(id));
            }
            return filter;
        }

        Variant parseVariant(std::string_view line, const VcfHeader &header)
        {
            auto fields = split(line, '\t');

            if (fields.size() > 8 && fields.size() < 10)
                throw std::runtime_error(malformedHeader);

            Variant variant;

            variant.chromosome = std::string(fields[0]);
            variant.position = std::stoul(std::string(fields[1]));
            variant.id = fields[2] == "." ? "" : std::string(fields[2]);
            variant.referenceAllele = std::string(fields[3]);

            if (fields[4] != ".")
            {
                const auto alternateAlleles = split(fields[4], ',');
                for (const auto &alt : alternateAlleles)
                    variant.alternateAlleles.push_back(std::string(alt));
            }

            variant.quality = fields[5] == "." ? std::optional<double>{} : std::stod(std::string(fields[5]));
            variant.filter = parseFilter(fields[6]);

            // INFO
            // Samples

            return variant;
        }
    }

    ParseResult VcfParser::parse(const std::filesystem::path &path) const
    {
        ParseResult result;
        std::ifstream file(path);

        if (!file)
        {
            throw std::runtime_error("Failed to open VCF file: " + path.string());
        }

        std::string line;

        while (std::getline(file, line))
        {
            if (line.starts_with("##"))
                parseMetaLine(line, result.header);

            else if (line.starts_with("#CHROM"))
                parseColumnHeader(line, result.header);
            else
            {
                result.variants.push_back(parseVariant(line, result.header));
                // break;
            }
        }

        return result;
    }

}