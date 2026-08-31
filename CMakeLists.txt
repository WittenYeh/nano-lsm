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

enable_testing()
add_subdirectory(tests)
