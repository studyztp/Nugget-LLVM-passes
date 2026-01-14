# LLVM IRBBLabel Pass - Test Suite

## Quick Start

```bash
# Configure
cmake -S . -B build -DLLVM_BIN_DIR=path/to/LLVM/bin -DPASS_PLUGIN=path/to/NuggetPasses.so

# Build
cmake --build build

# Run all tests
cd build && ctest --output-on-failure
```

---

## Overview

This test suite validates the **IRBBLabelPass** LLVM plugin, which labels all basic blocks with unique metadata IDs and exports them to CSV format.

The suite includes 6 comprehensive test cases covering:
- **Test 1**: Simple C code with basic functions
- **Test 2**: Dynamic library dependencies and external calls
- **Test 3**: C++ language features (templates, overloading, lambdas)
- **Test 4**: Multi-language interoperability (C++ + Fortran)
- **Test 5**: Optimization pipeline comparison (direct vs pipelined)
- **Test 6**: Custom parameter override for output filename

---

## Test Cases

### Test 1: Simple C Code
**Purpose**: Validate basic pass functionality on simple C functions
- **Source**: `fibonacci()`, `add()`, `multiply()` functions
- **Expected**: CSV with labeled basic blocks, IR with `!bb.id` metadata
- **Validates**: Pass correctly processes simple functions and control flow

### Test 2: Dynamic Library Calls
**Purpose**: Validate pass with external library dependencies
- **Source**: Functions calling math library (sin, cos, pow)
- **Expected**: CSV and IR metadata with external symbols preserved
- **Validates**: Pass handles external library calls without labeling in declaration

### Test 3: C++ Static Code
**Purpose**: Test with C++ language features and name mangling
- **Source**: Templates, lambdas, function overloading, STL usage
- **Expected**: CSV with mangled C++ names, IR metadata on all instantiations
- **Validates**: Pass correctly handles C++ name mangling and template instantiations

### Test 4: Mixed C++/Fortran
**Purpose**: Test multi-language interoperability
- **Source**: Fortran module + C++ wrapper + main
- **Expected**: CSV with functions from both languages, cross-language metadata
- **Validates**: Pass works across language boundaries via llvm-link

### Test 5: Optimization Pipeline Comparison
**Purpose**: Verify the pass doesn't break optimization equivalence
- **Pipelines Compared**:
  - Direct: `clang -O2` (single-step compilation)
  - Pipelined: IR → `opt -O2` → pass labeling → `llc -O2` → linking
- **Expected**: Both binaries produce equivalent results, performance within tolerance
- **Validates**: Pass doesn't introduce optimization regressions or break IR semantics

### Test 6: Custom Output Filename
**Purpose**: Validate pass parameter parsing
- **Command**: `-passes="ir-bb-label-pass<output_csv=my_custom_bb_output.csv>"`
- **Expected**: CSV created with custom filename instead of default
- **Validates**: Pass correctly parses and applies custom parameters

---

## Validation Categories

Each test validates these aspects:

| Category | What It Checks |
|----------|----------------|
| **CSV Exists** | Pass generates output file |
| **CSV Format** | Header and data structure correct |
| **Metadata Valid** | IR `!bb.id` annotations match CSV content |

Test 5 additionally validates:
- Both binaries compile and execute successfully
- Performance difference within acceptable tolerance
- Pass comparison report generated correctly

---

## Build Outputs

Each test generates:
- `bb_info.csv` - Basic block metadata in CSV format
- `*_instrumented.ll` - Readable LLVM IR with `!bb.id` metadata annotations
- `*_instrumented.bc` - Binary LLVM bitcode with metadata

Test 5 also generates:
- `test5_direct` - Direct compilation binary
- `test5_optimized` - Pipeline compilation binary
- `direct_passes.txt` - IR passes from direct compilation
- `opt_passes.txt` - IR passes from opt -O2
- `llc_passes.txt` - Machine passes from llc -O2
- `direct_machine_passes.txt` - Machine passes from direct compilation
- `PASSES_COMPARISON_REPORT.md` - Detailed comparison analysis

---

## CSV Output Format

```
FunctionName,FunctionID,BasicBlockName,BasicBlockInstCount,BasicBlockID
fibonacci,0,,2,0
fibonacci,0,tailrecurse._crit_edge,5,1
fibonacci,0,tailrecurse,8,2
```

---

## Directory Structure

```
test/
├── CMakeLists.txt                          # Master configuration
├── README.md                               # This file (overview & quick start)
├── STRUCTURE.md                            # Directory organization guide
├── common/                                 # Shared utilities
│   └── verify_csv.cmake                    # CSV validation helper
│
├── test1_simple/                           # C code test
├── test2_dynamic_lib/                      # Library calls test
├── test3_cpp_static/                       # C++ features test
├── test4_mixed/                            # Multi-language test
├── test5_optimization/                     # Optimization pipeline test
└── build/                                  # Generated outputs (per test)
```

---

## Configuration

Each test has its own `CMakeLists.txt` containing:
- Independent compilation pipeline
- ctest framework registration
- Isolated output directory

Master `CMakeLists.txt`:
- Finds LLVM tools (clang, opt, llc, llvm-dis, flang)
- Validates pass plugin
- Uses `add_subdirectory()` for each test

---

## Key Features

✓ **Complete Coverage** - Tests across multiple languages  
✓ **Optimization Validation** - Ensures pass doesn't break optimizations  
✓ **Multi-language Support** - C, C++, and Fortran  
✓ **CSV Export** - Structured output for analysis  
✓ **Modular Design** - Each test is independent  
✓ **Easy to Extend** - Simple to add new tests  

---

## Debugging

The pass includes conditional debug output controlled via the `DEBUG_PRINT` macro:

**Enable debug output** (compile with `-DDEBUG`):
```bash
cd pass && rm -rf build
cmake -S . -B build -DCMAKE_CXX_FLAGS="-DDEBUG"
cmake --build build
```

Then run tests to see detailed debug information about parameter parsing and pass execution.

**Disable debug output** (default):
```bash
cd pass && rm -rf build
cmake -S . -B build
cmake --build build
```

This produces clean output without debug messages.

---

## Requirements

- LLVM 18+ with PassBuilder support
- CMake 3.20+
- Clang/Clang++
- Flang (for test4, optional)
- NuggetPasses.so

---

## Adding New Tests

1. Create `testN_description/` directory
2. Copy CMakeLists.txt template from existing test
3. Add source files to the directory
4. Add `add_subdirectory(testN_description)` to master CMakeLists.txt
5. Rebuild: `cmake --build build`

---

See [STRUCTURE.md](STRUCTURE.md) for detailed organization and architecture documentation.
