# AGENTS.md - Agent Guidelines for Assimp

This document provides guidelines for AI agents working on the Assimp codebase.

## Project Overview

Assimp (Open Asset Import Library) is a C++ library that loads various 3D file formats into a shared, in-memory format. It supports 40+ import formats and several export formats.

## Build Commands

### Basic Build (CMake + Ninja recommended)
```bash
# Configure with CMake
cmake -G Ninja -DASSIMP_BUILD_TESTS=ON -DASSIMP_WARNINGS_AS_ERRORS=ON -S . -B build

# Build
cmake --build build
```

### Key CMake Options
- `-DASSIMP_BUILD_TESTS=ON` - Build unit tests (default ON)
- `-DASSIMP_WARNINGS_AS_ERRORS=ON` - Treat warnings as errors (default ON)
- `-DASSIMP_BUILD_ASSIMP_TOOLS=ON` - Build command-line tools
- `-DASSIMP_BUILD_SAMPLES=ON` - Build sample applications
- `-DASSIMP_DOUBLE_PRECISION=ON` - Use double precision for calculations
- `-DASSIMP_NO_EXPORT=ON` - Disable export functionality
- `-DBUILD_SHARED_LIBS=OFF` - Build static library

### Running Tests

#### Run All Tests
```bash
# Using ctest
cd build && ctest

# Or run unit directly
./build/unit
```

#### Run Single Test
```bash
# Using ctest with filter
cd build && ctest -R "TestName"

# Or run unit with filter
./build/unit --gtest_filter="TestSuiteName.TestName"
```

For example:
```bash
./build/unit --gtest_filter="utObjImportExport.*"
./build/unit --gtest_filter="utMaterialSystem.*"
```

#### Test Directory
Tests are located in `test/unit/` and use Google Test. Test files are named `ut<Feature>.cpp`.

## Code Style

### Formatting
- **Use clang-format** before committing. Run: `clang-format -i <file>`
- The project uses a `.clang-format` file at the root with LLVM-based style
- Key settings:
  - Indent width: 4 spaces
  - Tab width: 4, UseTab: Never
  - ColumnLimit: 0 (no line length limit)
  - BreakConstructorInitializers: AfterColon
  - AccessModifierOffset: -4

### Header File Organization
```cpp
// Order of includes (use clang-format to enforce):
// 1. Module's own header
// 2. Other assimp headers (assimp/*)
// 3. External headers (contrib/*)
// 4. Standard library headers
// 5. System headers

#include "Common/Importer.h"
#include <assimp/version.h>
#include <assimp/config.h>
#include "../contrib/some_lib/some.h"
#include <vector>
#include <string>
```

### Naming Conventions
- **Classes/Types**: PascalCase (e.g., `Importer`, `aiScene`)
- **Functions**: PascalCase (e.g., `ReadFile`, `GetExtension`)
- **Variables**: camelCase (e.g., `scene`, `importStep`)
- **Constants**: kCamelCase or UPPER_SNAKE_CASE (e.g., `kMaxVertices`)
- **Member variables**: Often prefixed with `m_` (e.g., `mScene`)
- **Static variables**: Often prefixed with `s_`

### File Naming
- **Header files**: PascalCase (e.g., `Importer.h`, `ScenePrivate.h`)
- **Source files**: PascalCase (e.g., `Importer.cpp`)
- **Test files**: Prefixed with `ut` (e.g., `utObjImportExport.cpp`)

### C++ Guidelines

#### Language Standard
- Minimum: C++17
- Use modern C++ features (smart pointers, constexpr, etc.)

#### Error Handling
- Use exceptions for recoverable errors (derived from `std::exception`)
- Use `ai_assert` for debugging assertions in code
- Return error codes from C API functions

#### Classes
- Use `ai_enable_erasing` pattern for optional features
- Use PImpl idiom for ABI stability where appropriate

#### Memory Management
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Prefer RAII patterns
- Document ownership semantics in function comments

### Importers/Exporters

#### Structure
Each importer typically has:
1. Header in `code/AssetLib/<Format>/`
2. Implementation in `code/AssetLib/<Format>/`
3. Registration in `code/Common/ImporterRegistry.cpp`
4. Unit tests in `test/unit/ImportExport/`

#### Registration
```cpp
void GetImporterInstanceList(std::vector<BaseImporter*>& out);
// Add to registry
out.push_back(new MyFormatImporter());
```

### Post-Processing

- Located in `code/PostProcessing/`
- Each process inherits from `BaseProcess`
- Implement `ExecuteOnScene` method

### Documentation

- Use Doxygen-style comments for public APIs
- Example:
```cpp
/// <summary>
/// Loads a file from disk.
/// </summary>
/// <param name="pFile">Path to the file.</param>
/// <returns>Pointer to the imported scene.</returns>
aiScene* Importer::ReadFile(const char* pFile);
```

### Contributing

1. Create a fork of assimp
2. Create a branch for your feature/fix
3. Run `clang-format` on modified files
4. Ensure tests pass
5. Open a PR against `master` branch

## Directory Structure

```
code/
  AssetLib/     - Importers and exporters
  CApi/         - C API wrapper
  Common/       - Shared utilities
  Geometry/     - Geometry processing
  Material/     - Material handling
  PostProcessing/ - Mesh post-processing
include/        - Public headers (assimp/)
test/
  unit/         - Unit tests
  models/       - Test 3D models
test/           - Test data (non-BSD licensed)
contrib/        - Third-party libraries
```

## CI/CD

The project runs CI on GitHub Actions:
- Builds on Linux and Windows
- Runs tests including memory leak detection
- Checks compiler warnings

## Common Tasks

### Adding a New Importer
1. Create `code/AssetLib/<Format>/<Format>Importer.h`
2. Create `code/AssetLib/<Format>/<Format>Importer.cpp`
3. Register in `code/Common/ImporterRegistry.cpp`
4. Add tests in `test/unit/ImportExport/`
5. Add to CMakeLists.txt if needed

### Running Specific Test Suite
```bash
# Test specific importer
./build/unit --gtest_filter="utglTF2ImportExport.*"

# Test post-processing
./build/unit --gtest_filter="utTriangulate.*"

# Test math operations
./build/unit --gtest_filter="utMatrix4x4.*"
```
