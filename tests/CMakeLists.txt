add_executable(vcf_importer_tests
    parser/vcf_parser_tests.cpp
    config/database_config_tests.cpp
    persistence/database_tests.cpp
    persistence/variant_repository_tests.cpp
    persistence/database_transaction_tests.cpp
)

target_compile_definitions(vcf_importer_tests
    PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

target_link_libraries(vcf_importer_tests
    PRIVATE
        vcf_importer_lib
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(vcf_importer_tests)