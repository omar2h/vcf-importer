add_library(vcf_importer_lib
    vcf/vcf_parser.cpp
    config/database_config.cpp
    persistence/database.cpp
    persistence/variant_repository.cpp
)

target_include_directories(vcf_importer_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(vcf_importer_lib
    PUBLIC
        SQLite::SQLite3
)

add_executable(vcf_importer
    cli/main.cpp
)

target_link_libraries(vcf_importer
    PRIVATE
        vcf_importer_lib
)