cmake_minimum_required(VERSION 3.16)
project(clion VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)

# Include FetchContent module
include(FetchContent)

# Fetch fmt library
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

# Fetch indicators library
FetchContent_Declare(
    indicators
    GIT_REPOSITORY https://github.com/p-ranav/indicators.git
    GIT_TAG v2.3
)
FetchContent_MakeAvailable(indicators)

# Find CLI11
pkg_check_modules(CLI11 REQUIRED CLI11)
if(NOT CLI11_FOUND)
    # Fallback to finding CLI11 via config
    find_package(CLI11 REQUIRED)
endif()

# Find nlohmann/json
pkg_check_modules(JSON REQUIRED nlohmann_json)
if(NOT JSON_FOUND)
    # Fallback to finding nlohmann_json via config
    find_package(nlohmann_json REQUIRED)
endif()

# Find YAML-CPP
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)
if(NOT YAML_CPP_FOUND)
    # Fallback to finding yaml-cpp via config
    find_package(yaml-cpp REQUIRED)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)

# UI Source files
set(UI_SOURCES
    src/ui/ui_manager.cpp
    src/ui/color_manager.cpp
    src/ui/progress_manager.cpp
    src/ui/prompt_manager.cpp
    src/ui/table_formatter.cpp
    src/ui/theme_manager.cpp
    src/ui/terminal_io.cpp
    src/ui/progress_bar.cpp
    src/ui/spinner.cpp
    src/ui/table.cpp
)

# UI Header files
set(UI_HEADERS
    src/ui/ui_manager.h
    src/ui/color_manager.h
    src/ui/progress_manager.h
    src/ui/prompt_manager.h
    src/ui/table_formatter.h
    src/ui/theme_manager.h
    src/ui/terminal_io.h
    src/ui/progress_bar.h
    src/ui/spinner.h
    src/ui/table.h
)

# Source files
set(SOURCES
    src/main.cpp
    src/cli/cli_parser.cpp
    src/cli/interaction.cpp
    src/cli/command_processor.cpp
    src/llm/llm_client.cpp
    src/llm/context_builder.cpp
    src/llm/session.cpp
    src/llm/session_checkpoint.cpp
    src/llm/memory_manager.cpp
    src/indexer/project_scanner.cpp
    src/indexer/code_index.cpp
    src/indexer/prompt_analyzer.cpp
    src/compiler/command_executor.cpp
    src/compiler/enhanced_command_executor.cpp
    src/compiler/error_parser.cpp
    src/compiler/enhanced_error_parser.cpp
    src/utils/file_utils.cpp
    src/utils/diff_utils.cpp
    src/utils/token_counter.cpp
    src/utils/rules_loader.cpp
    src/utils/string_utils.cpp
    src/nlp/text_analyzer.cpp
    src/nlp/command_interpreter.cpp
    src/nlp/code_analyzer.cpp
    src/nlp/error_interpreter.cpp
    ${UI_SOURCES}
)

# Header files
set(HEADERS
    include/clion/common.h
    src/cli/cli_parser.h
    src/cli/interaction.h
    src/llm/llm_client.h
    src/llm/context_builder.h
    src/llm/session.h
    src/indexer/project_scanner.h
    src/indexer/code_index.h
    src/indexer/prompt_analyzer.h
    src/compiler/command_executor.h
    src/compiler/enhanced_command_executor.h
    src/compiler/error_parser.h
    src/compiler/enhanced_error_parser.h
    src/utils/file_utils.h
    src/utils/diff_utils.h
    src/utils/token_counter.h
    src/utils/rules_loader.h
    src/utils/string_utils.h
    src/nlp/text_analyzer.h
    src/nlp/command_interpreter.h
    src/nlp/code_analyzer.h
    src/nlp/error_interpreter.h
    ${UI_HEADERS}
)

# Create executable
add_executable(clion ${SOURCES} ${HEADERS})

# Link libraries
if(CLI11_FOUND)
    target_link_libraries(clion ${CLI11_LIBRARIES})
    target_include_directories(clion PRIVATE ${CLI11_INCLUDE_DIRS})
else()
    target_link_libraries(clion CLI11::CLI11)
endif()

if(JSON_FOUND)
    target_link_libraries(clion ${JSON_LIBRARIES})
    target_include_directories(clion PRIVATE ${JSON_INCLUDE_DIRS})
else()
    target_link_libraries(clion nlohmann_json::nlohmann_json)
endif()

if(YAML_CPP_FOUND)
    target_link_libraries(clion ${YAML_CPP_LIBRARIES})
    target_include_directories(clion PRIVATE ${YAML_CPP_INCLUDE_DIRS})
else()
    target_link_libraries(clion yaml-cpp)
endif()

target_link_libraries(clion ${CURL_LIBRARIES})

# Link new UI libraries
if(fmt_FOUND)
    target_link_libraries(clion fmt::fmt)
else()
    target_link_libraries(clion fmt)
endif()

if(indicators_FOUND)
    target_link_libraries(clion indicators::indicators)
else()
    target_link_libraries(clion indicators)
endif()

# Add Firestore SDK when available
# find_package(Firebase REQUIRED)
# target_link_libraries(clion Firebase::Firestore)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(clion ws2_32)
endif()

# Installation
install(TARGETS clion DESTINATION bin)

# Testing
# enable_testing()
# add_subdirectory(tests)

# Documentation
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in 
                   ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif()

# Package configuration
include(CPack)
set(CPACK_PACKAGE_NAME "clion")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CLion - C++ Agentic CLI Tool")
set(CPACK_PACKAGE_VENDOR "CLion Project")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")