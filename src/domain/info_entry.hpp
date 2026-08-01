#pragma once

#include <string>
#include <vector>

namespace vcf
{

    struct InfoEntry
    {
        std::string key;
        std::vector<std::string> values;
    };

}