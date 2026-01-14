# LLVM IRBBLabel Pass - Test Suite

## Quick Start

```bash
# Configure
cmake -S . -B build -DLLVM_BIN_DIR=/usr/bin -DPASS_PLUGIN=../pass/build/NuggetPasses.so

# Build
cmake --build build

# Run all tests
cd build && ctest --output-on-failure
```

**Result**: ✅ All 27 tests PASSING

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

### Test 1: Simple C Code ✓
Basic functionality test with 3 simple functions
- **Functions**: `fibonacci()`, `add()`, `multiply()`
- **Output**: 6 labeled basic blocks in `bb_info.csv`
- **Status**: PASSING (4/4)
- **Validations**: CSV format, content, and IR metadata annotations

### Test 2: Dynamic Library Calls ✓
Tests external library dependencies (math functions)
- **Features**: Math library calls (-lm), multiple function types, extern declarations
- **Status**: PASSING (4/4)
- **Validations**: CSV format, content, and IR metadata annotations

### Test 3: C++ Static Code ✓
Tests C++ language features
- **Features**: Function overloading, templates, lambdas, name mangling, STL usage
- **Status**: PASSING (4/4)
- **Validations**: CSV format, content, and IR metadata annotations (handles mangled names)

### Test 4: Mixed C++/Fortran ✓
Tests multi-language interoperability
- **Features**: llvm-link module linking, language interop, symbol resolution, C++/Fortran interface
- **Status**: PASSING (4/4)
- **Validations**: CSV format, content, and IR metadata annotations across languages

### Test 5: Optimization Pipeline Comparison ✓
Verifies optimization equivalence between two compilation pipelines
- **Pipeline 1 (Direct)**: `clang -O2` compilation
- **Pipeline 2 (Pipelined)**: IR → `opt -O2` → pass labeling → `llc -O2` → linking
- **Performance Result**: -3% difference (within 5% tolerance)
- **Status**: PASSING (11/11)
- **Verified**:
  - opt -O2 passes captured (630 lines)
  - llc -O2 passes captured (704 lines)
  - Both binaries execute successfully
  - Comparison report generated with detailed statistics
  - IR metadata validation on optimized code (handles complex BB names)

### Test 6: Custom Output Filename ✓
Tests parameter override for custom CSV output filename
- **Features**: Pass parameter parsing, option override, custom naming
- **Command**: `-passes="ir-bb-label-pass<output_csv=my_custom_bb_output.csv>"`
- **Verification**: CSV file created with custom name instead of default `bb_info.csv`
- **Status**: PASSING (4/4)
- **Validations**: CSV format, content, and IR metadata annotations with custom filename

---

## Test Results

| Test | CSV Generated | Format Valid | Content Valid | Metadata Valid | Pass |
|------|---------------|--------------|---------------|----------------|------|
| test1_simple | ✓ | ✓ | ✓ | ✓ | ✓ (4/4) |
| test2_dynamic_lib | ✓ | ✓ | ✓ | ✓ | ✓ (4/4) |
| test3_cpp_static | ✓ | ✓ | ✓ | ✓ | ✓ (4/4) |
| test4_mixed | ✓ | ✓ | ✓ | ✓ | ✓ (4/4) |
| test5_optimization | ✓ | ✓ | ✓ | ✓ | ✓ (11/11) |
| test6_custom_output | ✓ | ✓ | ✓ | ✓ | ✓ (4/4) |
| **Total** | **6/6** | **6/6** | **6/6** | **6/6** | **27/27** |

---

## Build Outputs

Each test generates:
- `bb_info.csv` - Basic block metadata in CSV format
- `*_instrumented.ll` - Readable LLVM IR with `!bb.id` metadata annotations
- `*_instrumented.bc` - Binary LLVM bitcode with metadata

Test 5 also generates:
- `test5_direct` - Direct compilation binary
- `test5_optimized` - Pipeline compilation binary
- `opt_passes.txt` - Optimizer passes applied (630 lines)
- `llc_passes.txt` - Code generator passes applied (704 lines)
- Performance comparison report

---

## CSV Output Format

```
FunctionName,FunctionID,BasicBlockName,BasicBlockInstCount,BasicBlockID
fibonacci,0,,2,0
fibonacci,0,tailrecurse._crit_edge,5,1
fibonacci,0,tailrecurse,8,2
```

Test 5 Statistics:
- Total basic blocks labeled: 37
- Functions processed: 6
- Metadata nodes created: 37 unique IDs

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

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Pass plugin not found" | Build pass: `cd ../pass && cmake --build build` |
| Tests not found | Reconfigure: `rm -rf build && cmake -S . -B build ...` |
| LLVM tools not found | Set `-DLLVM_BIN_DIR=/usr/bin` or your LLVM path |
| Build fails | Ensure LLVM 18+ with PassBuilder support installed |

---

## Key Features

✓ **Complete Coverage** - Tests across multiple languages  
✓ **Optimization Validation** - Ensures pass doesn't break optimizations  
✓ **Multi-language Support** - C, C++, and Fortran  
✓ **CSV Export** - Structured output for analysis  
✓ **Modular Design** - Each test is independent  
✓ **Easy to Extend** - Simple to add new tests  

---

## Performance Results

**Optimization Pipeline Comparison (Test 5):**

```
Direct compilation (clang -O2):    29 ms average
Pipeline compilation (opt+llc):     29 ms average
Difference:                         0 ms (0%)

✓ Both pipelines have EQUAL performance
✓ opt -O2 passes: 630 lines captured
✓ llc -O2 passes: 704 lines captured
```

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
- NuggetPasses.so built from ../pass/

---

## Adding New Tests

1. Create `testN_description/` directory
2. Copy CMakeLists.txt template from existing test
3. Add source files to the directory
4. Add `add_subdirectory(testN_description)` to master CMakeLists.txt
5. Rebuild: `cmake --build build`

---

See [STRUCTURE.md](STRUCTURE.md) for detailed organization and architecture documentation.
