# CLion Development Roadmap

## Project Overview
CLion (Command Line Lion) is a high-performance, native C++ CLI tool that acts as an autonomous pair programmer for all agentic coding, focusing on robust context-handling and compiler-feedback loops, with support for OpenRouter and Requesty AI.

## Phase 1: Foundation and Core CLI Structure (The Skeleton)
**Goal**: Establish a robust C++ executable with clear command-line parsing and basic user interaction.

### Phase 1.1: Project Setup ✅
- Initialize the C++ project structure and configure CMake for multi-platform building
- Create modular project directory structure (src/cli, src/llm, src/indexer, src/compiler, src/utils, include, tests)
- Generate CMakeLists.txt with C++20 standard and modular structure
- Create initial main.cpp with basic entry point

### Phase 1.2: Argument Parsing ✅
- Implement command-line argument handling to support the main command and necessary flags (clion review --file <path>)
- Integrate CLI11 library in CMakeLists.txt
- Implement basic CLI structure in src/cli/cli_parser.cpp/h
- Define main 'clion' command and 'review' subcommand
- Add --file <path> option to review subcommand

### Phase 1.3: HTTP Client 🔄
- Implement a LLMClient class for making secure HTTPS POST requests to generic LLM API endpoints (like OpenRouter/Requesty AI)
- Must handle provider-specific API key authentication
- Integrate CURL library in CMakeLists.txt
- Create LLMClient class in src/llm/llm_client.cpp/h
- Implement basic CURL setup and initialization
- Create send_request method stub

### Phase 1.4: Basic I/O & Diff ⏳
- Implement utility functions to read file content and display the generated code patch using a standard unified diff format
- Implement read_file utility in src/utils/file_utils.cpp/h
- Implement generate_diff utility in src/utils/diff_utils.cpp/h

## Phase 2: LLM Communication and Context Handling (The Voice)
**Goal**: Enable the tool to communicate with diverse LLMs and effectively gather project context.

### Phase 2.1: Generic API Integration ⏳
- Complete the LLMClient class to construct diverse JSON payloads (supporting OpenRouter, Requesty AI, etc.)
- Process the structured JSON response, handling different vendor response formats
- Integrate nlohmann/json library
- Complete LLMClient::send_request with generic API payload
- Implement JSON response parsing

### Phase 2.2: Context Injection ⏳
- Implement a file reader function that uses the @file <path> syntax (or similar) to pull file contents and inject them into the main prompt
- Implement file inclusion detection (@file syntax)
- Create context builder in src/llm/context_builder.cpp/h

### Phase 2.3: Conversation Memory ⏳
- Implement a history manager that saves and re-loads the conversation history (prompts and LLM responses) for the current session
- Define session structures in src/llm/session.cpp/h
- Implement local JSON serialization (temporary)
- Integrate Firestore DB (for persistent session storage)
- Implement persistent session storage

### Phase 2.4: Transparency Feature 1 ⏳
- Implement a Real-Time Token Counter that calculates and displays the token usage of the current prompt before sending the request
- Implement token calculation utility in src/utils/token_counter.cpp/h
- Display token usage and cost before API requests

## Phase 3: Differentiating Feature: Native C++ Indexing (The Brain)
**Goal**: Implement the high-performance context feature that sets CLion apart.

### Phase 3.1: Project Scanner Utility ⏳
- Create a fast, recursive function to scan the entire project directory (respecting .gitignore)
- Identify all .cpp, .h, .hpp, and project files
- Create project scanner in src/indexer/project_scanner.cpp/h
- Implement .gitignore parsing

### Phase 3.2: Native Code Indexer ⏳
- Develop a component that reads C++ header files and extracts key metadata: function signatures, class names, and variable definitions
- Store this data in an optimized in-memory structure (e.g., std::map<string, string>)
- Define CodeIndex structure in src/indexer/code_index.cpp/h
- Implement C++ metadata extraction with regex
- Implement generate_index function

### Phase 3.3: Intelligent Context Selection ⏳
- Modify the prompt builder to first check the user's request against the index
- Instead of sending the full file, send only the index data and ask the user to confirm if the full file is necessary
- Create prompt analyzer in src/indexer/prompt_analyzer.cpp/h
- Implement user confirmation for full file inclusion

## Phase 4: Differentiating Feature: Compiler Feedback Loop (The Tamer)
**Goal**: Enable the agent to automatically fix code based on actual compiler errors—a massive win for C++ developers.

### Phase 4.1: Command Execution Tool ⏳
- Implement a C++ function that can execute arbitrary shell commands (e.g., cmake --build . or make)
- Capture the standard output and standard error (stderr)
- Create command executor in src/compiler/command_executor.cpp/h
- Implement stdout/stderr capture

### Phase 4.2: Error Parsing Filter ⏳
- Develop a regular expression or state machine to specifically parse the captured stderr output from common C++ compilers (GCC/Clang) and linters
- Extract the filename, line number, and error message
- Create error parser in src/compiler/error_parser.cpp/h
- Implement GCC/Clang error regex patterns
- Define error structure objects

### Phase 4.3: Iterative Fix Workflow ⏳
- Create a clion fix-error command
- When run, it automatically injects the extracted error details, the problematic code snippet, and the original error message into the LLM prompt
- Ask for a targeted fix and retrying the build command until success
- Implement fix-error command in CLI
- Create iterative fix workflow logic

## Phase 5: User Experience and Final Polish (The Polish)
**Goal**: Ensure a smooth, secure, and transparent user experience.

### Phase 5.1: Interactive Approval Loop ⏳
- Implement a secure loop: 1. LLM proposes changes (diff format). 2. User is prompted [A]pply / [S]kip / [E]dit Prompt?. 3. Only apply file changes on [A]pproval
- Implement approval prompt in src/cli/interaction.cpp/h
- Create diff application utility

### Phase 5.2: Structured Rules Engine (.clionrules) ⏳
- Define the YAML schema for a project configuration file (.clionrules) that holds project-specific coding conventions
- The agent should use this as a high-priority system instruction
- Integrate YAML-CPP library
- Define .clionrules.yaml schema
- Implement rules loader in src/utils/rules_loader.cpp/h

### Phase 5.3: Final Transparency Feature ⏳
- Implement the "Explain Plan Mode" (run with --explain flag) that outputs the agent's full reasoning chain
- Include the exact raw prompt sent to the LLM and the estimated cost
- Add --explain flag to CLI
- Implement explain plan mode output

## Final Phase: Testing and Documentation ⏳
- Final testing and documentation

## Timeline Estimation

Based on current progress:
- **Phase 1.3**: 2-3 days
- **Phase 1.4**: 1-2 days
- **Phase 2.1**: 3-4 days
- **Phase 2.2**: 2-3 days
- **Phase 2.3**: 4-5 days
- **Phase 2.4**: 1-2 days
- **Phase 3.1**: 2-3 days
- **Phase 3.2**: 4-5 days
- **Phase 3.3**: 3-4 days
- **Phase 4.1**: 2-3 days
- **Phase 4.2**: 3-4 days
- **Phase 4.3**: 4-5 days
- **Phase 5.1**: 3-4 days
- **Phase 5.2**: 3-4 days
- **Phase 5.3**: 1-2 days
- **Final Phase**: 2-3 days

**Estimated Total Completion Time**: 40-50 days (8-10 weeks)

## Key Differentiators

1. **Native C++ Performance**: High-speed execution and low memory footprint
2. **Generic API Support**: Works with multiple LLM providers (OpenRouter, Requesty AI, etc.)
3. **Intelligent Context Selection**: Reduces token usage and improves response quality
4. **Compiler Feedback Loop**: Automatic error fixing based on actual compiler output
5. **Transparency Features**: Full visibility into the agent's reasoning process

## Resources Required

1. **Development Environment**: C++20 compatible compiler, CMake, Git
2. **External Libraries**: CLI11, CURL, nlohmann/json, YAML-CPP
3. **Testing Framework**: Google Test (for unit tests)
4. **Documentation**: Markdown processor (for documentation generation)
5. **Cloud Services**: Firestore DB (for persistent session storage)