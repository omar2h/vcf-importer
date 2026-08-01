add_library(vcf_importer_lib
    vcf/vcf_parser.cpp
    config/database_config.cpp
)

target_include_directories(vcf_importer_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(vcf_importer
    cli/main.cpp
)

target_link_libraries(vcf_importer
    PRIVATE
        vcf_importer_lib
        SQLite::SQLite3
)