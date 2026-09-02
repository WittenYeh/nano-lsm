cmake_minimum_required(VERSION 3.20)

project(nano_lsm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third-party/emds-toolkit)

add_library(nano_lsm INTERFACE)
add_library(nano-lsm::nano-lsm ALIAS nano_lsm)

target_include_directories(
    nano_lsm
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(nano_lsm INTERFACE cxx_std_23)
target_link_libraries(nano_lsm INTERFACE emds-toolkit::emds-toolkit)

option(NANO_LSM_BUILD_TESTS "Build nano-lsm tests" ON)

if(NANO_LSM_BUILD_TESTS)
    if(NOT TARGET GTest::gtest_main)
        include(FetchContent)

        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
            URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )

        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    enable_testing()
    add_subdirectory(tests)
endif()
