#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include "filter.hpp"
#include "info_entry.hpp"
#include "sample.hpp"

namespace vcf
{

    struct Variant
    {
        std::string chromosome{};
        std::uint32_t position{};
        std::string id{};
        std::string referenceAllele{};
        std::vector<std::string> alternateAlleles{};
        std::optional<double> quality{};

        Filter filter{};
        std::vector<InfoEntry> info{};
        Sample sample{};
    };

}