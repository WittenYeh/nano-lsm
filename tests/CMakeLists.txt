add_executable(nano_lsm_tests nano_lsm_test.cpp)

target_link_libraries(nano_lsm_tests PRIVATE nano-lsm::nano-lsm)

add_test(NAME nano_lsm_tests COMMAND nano_lsm_tests)
