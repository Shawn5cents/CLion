# CLion (Command Line Lion)

A high-performance, native C++ CLI tool that acts as an autonomous pair programmer for C++ projects. By combining native C++ performance with AI-powered code assistance, CLion revolutionizes how developers interact with their codebase.

## Features

- **Intelligent Code Review**: AI-powered code analysis and improvement suggestions
- **Automated Error Fixing**: Automatically fix compilation errors using AI
- **Native C++ Indexing**: Fast, metadata-based context understanding
- **Transparent Operations**: Full visibility into token usage, costs, and reasoning
- **Project-Aware**: Understands project structure and respects coding conventions

## Quick Start

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 10+, or MSVC 2019+)
- CMake 3.16+
- CURL development libraries
- CLI11 library
- nlohmann/json library
- YAML-CPP library

### Building

```bash
git clone https://github.com/your-org/clion.git
cd clion
mkdir build && cd build
cmake ..
make
```

### Basic Usage

```bash
# Review a file
./clion review --file src/main.cpp

# Fix compilation errors
./clion fix-error "cmake --build ."

# Show detailed reasoning
./clion review --file src/main.cpp --explain
```

## Configuration

Create a `.clionrules.yaml` file in your project root:

```yaml
# Project-specific coding conventions
rules:
  - name: "naming_conventions"
    instruction: "Use snake_case for function names"
    priority: "high"

# API settings
api:
  provider: "gemini"
  model: "gemini-pro"
  max_tokens: 8192
```

## Development Status

CLion is currently in active development. See the [DEVELOPMENT_ROADMAP.md](DEVELOPMENT_ROADMAP.md) for detailed implementation plans.

### Current Phase: Foundation

- [x] Project Setup
- [ ] Argument Parsing with CLI11
- [ ] HTTP Client with CURL
- [ ] Basic I/O & Diff Utilities

## Architecture

See [CLION_ARCHITECTURE.md](CLION_ARCHITECTURE.md) for detailed system architecture and component design.

## Contributing

We welcome contributions! Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file for guidelines.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- CLI11 for command-line parsing
- nlohmann/json for JSON handling
- CURL for HTTP requests
- YAML-CPP for configuration parsing