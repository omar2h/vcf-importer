#pragma once

#include <vector>
#include <filesystem>

#include "parse_result.hpp"

namespace vcf
{

    class VcfParser
    {
    public:
        ParseResult parse(const std::filesystem::path &path) const;
    };
}