set(BKK_UDS_SERVER_SOURCES
    bkk_api.cpp
    bkk_client.cpp
    bkk_response_parser.cpp
    bkk_station_list.cpp
    uds_server.cpp
)

add_executable(bkk_uds_server ${BKK_UDS_SERVER_SOURCES})

target_include_directories(bkk_uds_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../bkk_uds
)

target_link_libraries(bkk_uds_server
    PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
)

if(BKK_API_VERBOSE_ON)
    target_compile_definitions(bkk_uds_server PRIVATE BKK_API_VERBOSE_ON)
endif()

if(MSVC)
    target_compile_options(bkk_uds_server PRIVATE /W4)
else()
    target_compile_options(bkk_uds_server PRIVATE -Wall -Wextra -pedantic)
endif()