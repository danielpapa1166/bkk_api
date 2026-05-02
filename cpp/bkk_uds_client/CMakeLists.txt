add_library(bkk_uds_client SHARED bkk_uds_client.c)
target_link_libraries(bkk_uds_client PRIVATE bkk_stop_utils rbuflogd_producer)

target_include_directories(bkk_uds_client
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../bkk_uds
)

set_target_properties(bkk_uds_client PROPERTIES
    OUTPUT_NAME bkk_uds_client
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(bkk_uds_client PRIVATE /W4)
else()
    target_compile_options(bkk_uds_client PRIVATE -Wall -Wextra -pedantic)
endif()