# LLVM Nugget Passes - Test Suite

This directory contains comprehensive test suites for LLVM custom passes developed in the Nugget project.

## Structure

```
test/
├── README.md                          # This file
└── IRBBLabelPass-test/                # Test suite for IRBBLabel Pass
    ├── README.md                      # Quick start guide
    ├── STRUCTURE.md                   # Architecture documentation
    ├── CMakeLists.txt                 # Build configuration
    ├── common/                        # Shared test utilities
    │   └── verify_csv.cmake           # CSV validation script
    ├── test1_simple/                  # Test case 1: Basic C code
    ├── test2_dynamic_lib/             # Test case 2: Dynamic libraries
    ├── test3_cpp_static/              # Test case 3: C++ features
    ├── test4_mixed/                   # Test case 4: Multi-language (C++/Fortran)
    ├── test5_optimization/            # Test case 5: Optimization pipeline
    ├── build/                         # Generated build outputs
    └── Testing/                       # CTest registry
```

## Quick Start

### For IRBBLabel Pass Tests:

```bash
cd IRBBLabelPass-test

# Configure
cmake -S . -B build \
  -DLLVM_BIN_DIR=/usr/bin \
  -DPASS_PLUGIN=../../pass/build/NuggetPasses.so

# Build
cmake --build build

# Run all tests
cd build && ctest --output-on-failure
```

**Result**: ✅ All 16 tests PASSING

## Available Test Suites

| Pass | Status | Location | Tests |
|------|--------|----------|-------|
| **IRBBLabel** | ✅ Active | `IRBBLabelPass-test/` | 5 cases, 16 tests |

## Test Suite Features

Each test suite typically includes:
- **Multiple test cases** covering different code scenarios
- **CSV output validation** for structured results
- **Build integration** with CMake
- **Automated testing** with ctest framework
- **Documentation** with architecture guides

## Adding New Pass Tests

To add tests for a new pass:

1. Create a new folder: `NewPass-test/`
2. Copy the structure from `IRBBLabelPass-test/`
3. Modify `CMakeLists.txt` for your pass
4. Create test case directories with appropriate source code
5. Update `common/verify_*.cmake` scripts as needed

Example:
```bash
mkdir NewPass-test
cp -r IRBBLabelPass-test/* NewPass-test/
# Edit CMakeLists.txt and test cases for your pass
```

## Documentation

- **README.md** (this file) - Overview of test suite structure
- **IRBBLabelPass-test/README.md** - IRBBLabel pass test quick start
- **IRBBLabelPass-test/STRUCTURE.md** - Detailed architecture documentation

## Requirements

- LLVM 18+ with PassBuilder support
- CMake 3.20+
- Clang/Clang++
- Optional: Flang (for multi-language tests)

## License

See main LLVM project for licensing information.
