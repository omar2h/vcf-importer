#pragma once

#include <string>
#include <vector>

namespace vcf
{

    struct SampleField
    {
        std::string key;
        std::vector<std::string> values;
    };

    struct Sample
    {
        std::vector<SampleField> fields;
    };

}