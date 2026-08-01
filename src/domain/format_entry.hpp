#pragma once

#include <string>
#include <vector>

namespace vcf
{

    struct FormatEntry
    {
        std::string key;
        std::vector<std::string> values;
    };

}