# Tests CMakeLists.txt

# Enable testing
enable_testing()

# Try to find Google Test (optional)
find_package(GTest QUIET)
find_package(GMock QUIET)

if(GTest_FOUND AND GMock_FOUND)
    message(STATUS "Google Test found - building full test suite")
    
    # Test sources
    set(TEST_SOURCES
        unit/test_cli_parser.cpp
        unit/test_command_executor.cpp
        unit/test_error_parser.cpp
        unit/test_file_utils.cpp
        unit/test_string_utils.cpp
        unit/test_token_counter.cpp
        unit/test_rules_loader.cpp
        unit/test_context_builder.cpp
        unit/test_prompt_analyzer.cpp
        unit/test_code_indexer.cpp
        unit/test_project_scanner.cpp
        unit/test_llm_client.cpp
        unit/test_nlp.cpp
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
    
else()
    message(STATUS "Google Test not found - building simple test executables")
    
    # CLI Parser test
    add_executable(clion_cli_parser_test
        unit/test_cli_parser.cpp
        ../src/cli/cli_parser.cpp
        ../src/utils/string_utils.cpp
    )
    target_include_directories(clion_cli_parser_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME CLIParserTest COMMAND clion_cli_parser_test)

    # Command Executor test
    add_executable(clion_command_executor_test
        unit/test_command_executor.cpp
        ../src/compiler/command_executor.cpp
    )
    target_include_directories(clion_command_executor_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME CommandExecutorTest COMMAND clion_command_executor_test)

    # Error Parser test
    add_executable(clion_error_parser_test
        unit/test_error_parser.cpp
        ../src/compiler/error_parser.cpp
    )
    target_include_directories(clion_error_parser_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME ErrorParserTest COMMAND clion_error_parser_test)

    # File Utils test
    add_executable(clion_file_utils_test
        unit/test_file_utils.cpp
        ../src/utils/file_utils.cpp
    )
    target_include_directories(clion_file_utils_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME FileUtilsTest COMMAND clion_file_utils_test)

    # String Utils test
    add_executable(clion_string_utils_test
        unit/test_string_utils.cpp
        ../src/utils/string_utils.cpp
    )
    target_include_directories(clion_string_utils_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME StringUtilsTest COMMAND clion_string_utils_test)

    # Token Counter test
    add_executable(clion_token_counter_test
        unit/test_token_counter.cpp
        ../src/utils/token_counter.cpp
        ../src/utils/file_utils.cpp
    )
    target_include_directories(clion_token_counter_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME TokenCounterTest COMMAND clion_token_counter_test)

    # Rules Loader test
    add_executable(clion_rules_loader_test
        unit/test_rules_loader.cpp
        ../src/utils/rules_loader.cpp
        ../src/utils/file_utils.cpp
    )
    target_include_directories(clion_rules_loader_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME RulesLoaderTest COMMAND clion_rules_loader_test)

    # Context Builder test
    add_executable(clion_context_builder_test
        unit/test_context_builder.cpp
        ../src/llm/context_builder.cpp
        ../src/utils/file_utils.cpp
        ../src/indexer/prompt_analyzer.cpp
        ../src/indexer/code_index.cpp
        ../src/indexer/project_scanner.cpp
        ../src/utils/string_utils.cpp
    )
    target_include_directories(clion_context_builder_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME ContextBuilderTest COMMAND clion_context_builder_test)

    # Prompt Analyzer test
    add_executable(clion_prompt_analyzer_test
        unit/test_prompt_analyzer.cpp
        ../src/indexer/prompt_analyzer.cpp
        ../src/indexer/code_index.cpp
        ../src/utils/file_utils.cpp
        ../src/utils/string_utils.cpp
    )
    target_include_directories(clion_prompt_analyzer_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME PromptAnalyzerTest COMMAND clion_prompt_analyzer_test)

    # Code Indexer test
    add_executable(clion_code_indexer_test
        unit/test_code_indexer.cpp
        ../src/indexer/code_index.cpp
        ../src/utils/file_utils.cpp
    )
    target_include_directories(clion_code_indexer_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME CodeIndexerTest COMMAND clion_code_indexer_test)

    # Project Scanner test
    add_executable(clion_project_scanner_test
        unit/test_project_scanner.cpp
        ../src/indexer/project_scanner.cpp
        ../src/utils/string_utils.cpp
    )
    target_include_directories(clion_project_scanner_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME ProjectScannerTest COMMAND clion_project_scanner_test)

    # LLM Client test
    add_executable(clion_llm_client_test
        unit/test_llm_client.cpp
        ../src/llm/llm_client.cpp
        ../src/llm/session.cpp
        ../src/utils/token_counter.cpp
        ../src/utils/file_utils.cpp
    )
    target_include_directories(clion_llm_client_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME LLMClientTest COMMAND clion_llm_client_test)

    # NLP test
    add_executable(clion_nlp_test
        unit/test_nlp.cpp
        ../src/nlp/text_analyzer.cpp
        ../src/nlp/command_interpreter.cpp
        ../src/utils/file_utils.cpp
    )
    target_include_directories(clion_nlp_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME NLPTest COMMAND clion_nlp_test)
endif()

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

# Integration tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration)
    add_subdirectory(integration)
endif()

# Performance tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance)
    add_subdirectory(performance)
endif()