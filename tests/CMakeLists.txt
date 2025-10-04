# Tests CMakeLists.txt

# Find Google Test
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

# Enable testing
enable_testing()

# Test sources
set(TEST_SOURCES
    unit/test_file_utils.cpp
    unit/test_diff_utils.cpp
    unit/test_cli_parser.cpp
    unit/test_llm_client.cpp
    unit/test_code_indexer.cpp
    unit/test_command_executor.cpp
    unit/test_error_parser.cpp
    unit/test_token_counter.cpp
    unit/test_rules_loader.cpp
)

# Create test executable
add_executable(clion_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(clion_tests 
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    clion
)

# Add test to CTest
add_test(NAME AllTests COMMAND clion_tests)

# Individual test targets
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_test(NAME ${test_name} COMMAND clion_tests --gtest_filter=${test_name}*)
endforeach()

# Coverage support
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        
        add_custom_target(coverage
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND ${GENHTML_PATH} -o coverage coverage.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        
        message(STATUS "Coverage support enabled")
    endif()
endif()