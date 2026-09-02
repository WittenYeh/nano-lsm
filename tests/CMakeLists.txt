add_executable(nano_lsm_tests nano_lsm_test.cpp)

target_link_libraries(nano_lsm_tests PRIVATE nano-lsm::nano-lsm)

add_test(NAME nano_lsm_tests COMMAND nano_lsm_tests)

add_executable(entry_codec_test entry_codec_test.cpp)
target_link_libraries(entry_codec_test PRIVATE nano-lsm::nano-lsm GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(entry_codec_test)
