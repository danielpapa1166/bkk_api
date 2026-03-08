cmake_minimum_required(VERSION 3.10)
project(bkk_api_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)

# nlohmann_json - try to find it, otherwise fetch
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(nlohmann_json 
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files for the library 
set(LIBRARY_SOURCES
    src/bkk_api.cpp
    src/bkk_client.cpp
    src/bkk_response_parser.cpp
    src/bkk_station_list.cpp
)

# Create shared library
add_library(bkk_api_shared SHARED ${LIBRARY_SOURCES})

# Link libraries to shared library
target_link_libraries(bkk_api_shared PUBLIC CURL::libcurl nlohmann_json::nlohmann_json)

# Set public header files
target_include_directories(bkk_api_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Add executable (optional - uses the shared library)
add_executable(bkk_api 
    src/main.cpp
)
target_link_libraries(bkk_api PRIVATE bkk_api_shared)

# Compiler flags for better warnings
if(MSVC)
    target_compile_options(bkk_api PRIVATE /W4)
else()
    target_compile_options(bkk_api PRIVATE -Wall -Wextra -pedantic)
endif()