#include "vcf_parser.hpp"

#include <stdexcept>
#include <fstream>

#include "../domain/vcf_header.hpp"

namespace vcf
{
    namespace
    {
        constexpr auto malformedHeaderDefinition = "Malformed VCF header definition.";
        constexpr auto malformedColumnHeader = "Malformed VCF column header.";
        constexpr auto malformedFilter = "Malformed FILTER field.";

        std::string_view extractDefinition(std::string_view line)
        {
            auto begin = line.find('<');
            auto end = line.rfind('>');
            if (begin >= end || begin == std::string_view::npos || end == std::string_view::npos)
                throw std::runtime_error(malformedHeaderDefinition);

            // begin points to '<'
            // end points to '>'
            return line.substr(begin + 1, end - begin - 1);
        }

        std::string_view extractValue(std::string_view text, std::string_view key)
        {
            auto begin = text.find(key);
            if (begin == std::string_view::npos)
                throw std::runtime_error(malformedHeaderDefinition);
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
                throw std::runtime_error(malformedColumnHeader);

            if (fields[0] != "#CHROM")
                throw std::runtime_error(malformedColumnHeader);
            if (fields[1] != "POS")
                throw std::runtime_error(malformedColumnHeader);
            if (fields[2] != "ID")
                throw std::runtime_error(malformedColumnHeader);
            if (fields[3] != "REF")
                throw std::runtime_error(malformedColumnHeader);
            if (fields[4] != "ALT")
                throw std::runtime_error(malformedColumnHeader);
            if (fields[5] != "QUAL")
                throw std::runtime_error(malformedColumnHeader);
            if (fields[6] != "FILTER")
                throw std::runtime_error(malformedColumnHeader);
            if (fields[7] != "INFO")
                throw std::runtime_error(malformedColumnHeader);

            if (fields.size() > 8)
            {
                if (fields[8] != "FORMAT")
                    throw std::runtime_error(malformedColumnHeader);

                if (fields.size() == 9)
                {
                    throw std::runtime_error("FORMAT column requires at least one sample name.");
                }

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

        std::vector<InfoEntry> parseInfo(std::string_view field, const VcfHeader &header)
        {
            if (field == ".")
                return {};

            std::vector<InfoEntry> infoEntries{};
            auto entries = split(field, ';');
            for (const auto &entry : entries)
            {
                InfoEntry infoEntry{};
                auto parts = split(entry, '=');
                if (!header.infoDefinitions.contains(std::string(parts[0])))
                    throw std::runtime_error("Unknown INFO field: " + std::string(parts[0]));

                infoEntry.key = parts[0];

                if (parts.size() != 1 && parts.size() != 2)
                    throw std::runtime_error("Malformed INFO entry: " + std::string(entry));
                if (parts.size() == 2)
                {
                    auto values = split(parts[1], ',');
                    for (const auto &value : values)
                    {
                        infoEntry.values.push_back(std::string(value));
                    }
                }
                infoEntries.push_back(infoEntry);
            }
            return infoEntries;
        }

        std::vector<Sample> parseSamples(const std::vector<std::string_view> &sampleFields,
                                         std::string_view formatField,
                                         const VcfHeader &header)
        {
            std::vector<Sample> samples;
            auto formatKeys = split(formatField, ':');
            for (const auto &sampleField : sampleFields)
            {
                Sample sample{};

                auto sampleParts = split(sampleField, ':');
                if (sampleParts.size() != formatKeys.size())
                    throw std::runtime_error("FORMAT field count does not match sample field count.");

                for (size_t i = 0; i < sampleParts.size(); ++i)
                {
                    if (!header.formatDefinitions.contains(std::string(formatKeys[i])))
                        throw std::runtime_error("Unknown FORMAT field: " + std::string(formatKeys[0]));

                    FormatEntry formatEntry{std::string(formatKeys[i]), {}};
                    auto values = split(sampleParts[i], ',');
                    for (const auto &val : values)
                        formatEntry.values.push_back(std::string(val));
                    sample.formatEntries.push_back(formatEntry);
                }
                samples.push_back(sample);
            }
            return samples;
        }

        Variant parseVariant(std::string_view line, const VcfHeader &header)
        {
            auto fields = split(line, '\t');

            if (fields.size() == 9)
                throw std::runtime_error("FORMAT column requires at least one sample column.");

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
            variant.info = parseInfo(fields[7], header);

            if (fields.size() == 8)
                return variant;

            if (fields.size() >= 10)
            {
                std::vector<std::string_view> sampleFields{};
                for (std::size_t i = 9; i < fields.size(); ++i)
                    sampleFields.push_back(fields[i]);

                variant.samples = parseSamples(sampleFields, fields[8], header);
            }

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

        bool fileFormatSeen = false;
        bool columnHeaderSeen = false;
        std::string line;

        while (std::getline(file, line))
        {
            if (line.starts_with("##fileformat="))
            {
                fileFormatSeen = true;
            }
            else if (line.starts_with("##"))
            {
                if (!fileFormatSeen)
                    throw std::runtime_error("The first line of a VCF file must be a '##fileformat' declaration.");
                parseMetaLine(line, result.header);
            }
            else if (line.starts_with("#CHROM"))
            {
                if (!fileFormatSeen)
                    throw std::runtime_error("The first line of a VCF file must be a '##fileformat' declaration.");
                parseColumnHeader(line, result.header);
                columnHeaderSeen = true;
            }
            else
            {
                if (!columnHeaderSeen)
                    throw std::runtime_error("Variant records cannot appear before the VCF column header.");
                result.variants.push_back(parseVariant(line, result.header));
            }
        }
        if (!fileFormatSeen)
            throw std::runtime_error("The first line of a VCF file must be a '##fileformat' declaration.");

        if (!columnHeaderSeen)
            throw std::runtime_error("Missing required VCF column header.");

        return result;
    }

}