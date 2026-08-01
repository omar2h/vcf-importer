#pragma once

#include <vector>

#include "../domain/vcf_header.hpp"
#include "../domain/variant.hpp"

namespace vcf
{

    struct ParseResult
    {
        VcfHeader header{};
        std::vector<Variant> variants{};
    };

}