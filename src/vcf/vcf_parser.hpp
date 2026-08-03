#pragma once

#include <filesystem>
#include <fstream>

#include "../domain/variant.hpp"
#include "../domain/vcf_header.hpp"

namespace vcf
{

    class VcfParser
    {
    public:
        explicit VcfParser(const std::filesystem::path &path);

        const VcfHeader &header() const;

        [[nodiscard]]
        bool readNextVariant(Variant &variant);

    private:
        void parseHeader();

    private:
        std::ifstream m_file{};
        VcfHeader m_header{};
    };
}