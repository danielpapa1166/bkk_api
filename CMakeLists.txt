cmake_minimum_required(VERSION 3.16)
project(bkk_api_uds_service LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BKK_API_VERBOSE_ON "Enable verbose logging in bkk_api" OFF)

# Keep outputs in predictable locations for easier integration/testing.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(CURL REQUIRED)

# Paths to cJSON and rbuflogd sources. Defaults point to the conventional
# submodules/ locations so Yocto builds (which fetch them there via destsuffix)
# and fresh git clones with --recurse-submodules both work out of the box.
# Override with -DCJSON_DIR=... or -DRBUFLOGD_DIR=... for standalone builds
# that reference the top-level project copies.
set(CJSON_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/submodules/cJSON"    CACHE PATH "cJSON source directory")
set(RBUFLOGD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/submodules/rbuflogd" CACHE PATH "rbuflogd source directory")

find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(bkk_stop_utils STATIC bkk_uds/bkk_stop_utils.c)
target_include_directories(bkk_stop_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/bkk_uds)

# Import rbuflogd targets without enabling its daemon/test executables in default ALL builds.
add_subdirectory(${RBUFLOGD_DIR} ${CMAKE_BINARY_DIR}/rbuflogd_build EXCLUDE_FROM_ALL)

add_subdirectory(bkk_uds_server)
add_subdirectory(bkk_uds_client)
add_subdirectory(test)