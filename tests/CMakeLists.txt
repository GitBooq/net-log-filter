add_executable(tests_all
    ip_v4_address_test.cc
    filter_test.cc
    stream_processor_test.cc
    net_logger_test.cc
)

target_link_libraries(tests_all
    IPv4Address
    Filter
    StreamProcessor
    GTest::gtest_main
)

target_compile_options(tests_all PRIVATE
    -O0
    -g
)

include(GoogleTest)
gtest_discover_tests(tests_all)
