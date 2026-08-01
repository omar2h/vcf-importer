#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace vcf
{

    enum class FieldType
    {
        Integer,
        Float,
        String,
        Character,
        Flag
    };

    struct FieldDefinition
    {
        std::string id;
        std::string number;
        FieldType type;
    };

    struct VcfHeader
    {
        std::unordered_map<std::string, FieldDefinition> infoDefinitions;
        std::unordered_map<std::string, FieldDefinition> formatDefinitions;
        std::vector<std::string> sampleNames;
    };

}