add_executable(bkk_uds_test test_main.c)

target_include_directories(bkk_uds_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../bkk_uds
)

target_link_libraries(bkk_uds_test
    PRIVATE
        bkk_uds_client
)

if(UNIX AND NOT APPLE)
    set_target_properties(bkk_uds_test PROPERTIES
        BUILD_RPATH "$ORIGIN/../lib"
    )
endif()

if(MSVC)
    target_compile_options(bkk_uds_test PRIVATE /W4)
else()
    target_compile_options(bkk_uds_test PRIVATE -Wall -Wextra -pedantic)
endif()