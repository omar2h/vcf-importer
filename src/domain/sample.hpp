#pragma once

#include <vector>

#include "format_entry.hpp"

namespace vcf
{

    struct Sample
    {
        std::vector<FormatEntry> formatEntries;
    };

}