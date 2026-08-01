#pragma once

#include <string>
#include <vector>

namespace vcf
{

    enum class FilterStatus
    {
        NotApplied,
        Passed,
        Failed
    };

    struct Filter
    {
        FilterStatus status{};
        std::vector<std::string> failedFilterIds;
    };

}