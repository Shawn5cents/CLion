# CLion Project Summary

## Project Overview
CLion (Command Line Lion) is a high-performance, native C++ CLI tool that acts as an autonomous pair programmer for all agentic coding, focusing on robust context-handling and compiler-feedback loops, with support for OpenRouter and Requesty AI.

## Core Technology
- **Language**: C++20 for native speed and efficiency
- **Build System**: CMake for multi-platform building
- **Architecture**: Modular design with separate components for CLI, LLM, indexing, and compilation

## Key Features

### 1. Generic LLM Integration
- Support for multiple LLM providers (OpenRouter, Requesty AI, etc.)
- Provider-specific API key authentication
- Flexible JSON payload construction and response parsing

### 2. Intelligent Context Handling
- File inclusion with @file <path> syntax
- Native C++ code indexing for optimized context selection
- Real-time token counting and cost estimation

### 3. Compiler Feedback Loop
- Automatic error detection and fixing
- Support for GCC/Clang error parsing
- Iterative build and fix workflow

### 4. User Experience
- Interactive approval loop for code changes
- Structured rules engine (.clionrules)
- Transparency features with explain mode

## Project Structure

```
CLion/
├── CMakeLists.txt          # Main build configuration
├── Makefile                # Alternative build system for testing
├── README.md               # Project documentation
├── .gitignore              # Git ignore patterns
├── include/
│   └── clion/
│       └── common.h        # Shared types and utilities
├── src/
│   ├── main.cpp            # Application entry point
│   ├── cli/
│   │   ├── cli_parser.h    # CLI argument parsing
│   │   ├── cli_parser.cpp
│   │   ├── interaction.h   # User interaction utilities
│   │   └── interaction.cpp
│   ├── llm/
│   │   ├── llm_client.h    # LLM API client
│   │   ├── llm_client.cpp
│   │   ├── context_builder.h # Context management
│   │   ├── context_builder.cpp
│   │   ├── session.h       # Conversation memory
│   │   └── session.cpp
│   ├── indexer/
│   │   ├── project_scanner.h # Project scanning utilities
│   │   ├── project_scanner.cpp
│   │   ├── code_index.h    # Code indexing functionality
│   │   ├── code_index.cpp
│   │   ├── prompt_analyzer.h # Intelligent context selection
│   │   └── prompt_analyzer.cpp
│   ├── compiler/
│   │   ├── command_executor.h # Build command execution
│   │   ├── command_executor.cpp
│   │   ├── error_parser.h  # Compiler error parsing
│   │   └── error_parser.cpp
│   └── utils/
│       ├── file_utils.h    # File I/O utilities
│       ├── file_utils.cpp
│       ├── diff_utils.h    # Diff generation and parsing
│       ├── diff_utils.cpp
│       ├── token_counter.h # Token counting and cost estimation
│       ├── token_counter.cpp
│       ├── rules_loader.h  # Configuration file handling
│       └── rules_loader.cpp
└── tests/
    ├── CMakeLists.txt      # Test configuration
    ├── unit/               # Unit tests
    └── integration/        # Integration tests
```

## Development Phases

### Phase 1: Foundation and Core CLI Structure (The Skeleton)
**Goal**: Establish a robust C++ executable with clear command-line parsing and basic user interaction.

#### Phase 1.1: Project Setup ✅
- Initialize the C++ project structure and configure CMake for multi-platform building
- Create modular project directory structure
- Generate CMakeLists.txt with C++20 standard support
- Create initial main.cpp with basic entry point

#### Phase 1.2: Argument Parsing ✅
- Implement command-line argument handling for main command and flags
- Integrate CLI11 library
- Define main 'clion' command with 'review' and 'fix-error' subcommands
- Add --file <path> option to review subcommand

#### Phase 1.3: HTTP Client 🔄
- Implement LLMClient class for secure HTTPS POST requests
- Handle provider-specific API key authentication
- Integrate CURL library
- Create send_request method stub

#### Phase 1.4: Basic I/O & Diff ⏳
- Implement file reading utilities
- Display generated code patches using unified diff format

### Phase 2: LLM Communication and Context Handling (The Voice)
**Goal**: Enable communication with diverse LLMs and effective project context gathering.

#### Phase 2.1: Generic API Integration ⏳
- Complete LLMClient for diverse JSON payloads
- Process structured JSON responses from different vendors
- Integrate nlohmann/json library

#### Phase 2.2: Context Injection ⏳
- Implement file reader with @file <path> syntax
- Create context builder for prompt management

#### Phase 2.3: Conversation Memory ⏳
- Implement history manager for conversation persistence
- Define session structures
- Integrate Firestore DB for persistent storage

#### Phase 2.4: Transparency Feature 1 ⏳
- Implement real-time token counter
- Display token usage and cost before API requests

### Phase 3: Differentiating Feature: Native C++ Indexing (The Brain)
**Goal**: Implement high-performance context feature that sets CLion apart.

#### Phase 3.1: Project Scanner Utility ⏳
- Create fast recursive project directory scanner
- Respect .gitignore patterns
- Identify all C++ source and header files

#### Phase 3.2: Native Code Indexer ⏳
- Extract C++ metadata: function signatures, class names, variables
- Store in optimized in-memory structure
- Implement regex-based parsing

#### Phase 3.3: Intelligent Context Selection ⏳
- Check user request against code index
- Send only index data initially, with user confirmation for full files
- Create prompt analyzer for context optimization

### Phase 4: Differentiating Feature: Compiler Feedback Loop (The Tamer)
**Goal**: Enable automatic code fixing based on actual compiler errors.

#### Phase 4.1: Command Execution Tool ⏳
- Implement shell command execution
- Capture stdout and stderr
- Support build commands (cmake, make)

#### Phase 4.2: Error Parsing Filter ⏳
- Parse compiler error output with regex
- Extract filename, line number, and error message
- Support GCC/Clang error formats

#### Phase 4.3: Iterative Fix Workflow ⏳
- Create clion fix-error command
- Inject error details into LLM prompt
- Retry build until success

### Phase 5: User Experience and Final Polish (The Polish)
**Goal**: Ensure smooth, secure, and transparent user experience.

#### Phase 5.1: Interactive Approval Loop ⏳
- Implement secure approval process for code changes
- Provide [A]pply / [S]kip / [E]dit Prompt options
- Apply changes only on user approval

#### Phase 5.2: Structured Rules Engine (.clionrules) ⏳
- Define YAML schema for project configuration
- Store coding conventions as high-priority instructions
- Integrate YAML-CPP library

#### Phase 5.3: Final Transparency Feature ⏳
- Implement "Explain Plan Mode" with --explain flag
- Display full reasoning chain and raw prompts
- Show estimated costs

## Key Differentiators

1. **Native C++ Performance**: High-speed execution and low memory footprint
2. **Generic API Support**: Works with multiple LLM providers
3. **Intelligent Context Selection**: Reduces token usage and improves response quality
4. **Compiler Feedback Loop**: Automatic error fixing based on actual compiler output
5. **Transparency Features**: Full visibility into the agent's reasoning process

## Current Status

- **Phase 1.1**: Completed ✅
- **Phase 1.2**: Completed ✅
- **Phase 1.3**: In Progress 🔄
- **Phase 1.4**: Completed ✅
- **Phase 2.1**: Completed ✅
- **Phase 2.2**: Pending ⏳
- **Phase 3**: Pending ⏳
- **Phase 4**: Pending ⏳
- **Phase 5**: Pending ⏳

## Repository

The project is hosted at: https://github.com/Shawn5cents/CLion

## Timeline Estimation

**Estimated Total Completion Time**: 40-50 days (8-10 weeks)

## Resources Required

1. **Development Environment**: C++20 compatible compiler, CMake, Git
2. **External Libraries**: CLI11, CURL, nlohmann/json, YAML-CPP
3. **Testing Framework**: Google Test (for unit tests)
4. **Documentation**: Markdown processor (for documentation generation)
5. **Cloud Services**: Firestore DB (for persistent session storage)