# Test Suite Architecture & Organization

## Directory Structure

```
test/
├── README.md                               # Quick start & overview (START HERE)
├── STRUCTURE.md                            # This file: detailed architecture
├── CMakeLists.txt                          # Master build configuration
├── verify_csv.cmake                        # CSV validation helper module
│
├── test1_simple/                           # Test 1: Basic C functionality
│   ├── CMakeLists.txt
│   ├── fibonacci.c
│   ├── add.c
│   └── multiply.c
│
├── test2_dynamic_lib/                      # Test 2: External library calls
│   ├── CMakeLists.txt
│   ├── math_funcs.c
│   ├── math_funcs.h
│   └── main.c
│
├── test3_cpp_static/                       # Test 3: C++ features
│   ├── CMakeLists.txt
│   ├── vector_ops.hpp
│   ├── matrix_ops.cpp
│   └── main.cpp
│
├── test4_mixed/                            # Test 4: Multi-language (C++/Fortran)
│   ├── CMakeLists.txt
│   ├── compute.f90                         # Fortran module
│   ├── compute.h
│   ├── compute_wrapper.cpp                 # C++ wrapper
│   └── main.cpp
│
├── test5_optimization/                     # Test 5: Optimization pipeline
│   ├── CMakeLists.txt
│   ├── fibonacci_source.c                  # Source for both pipelines
│   └── fibonacci.c                         # Linked in both pipelines
│
├── shared_scripts/                         # Shared utility scripts
│   ├── format_check.cmake
│   ├── validate_pass.cmake
│   └── setup_environment.sh
│
└── build/                                  # Generated (cmake --build build)
    ├── test1_simple/
    │   ├── bb_info.csv
    │   ├── fibonacci.ll
    │   ├── add.ll
    │   ├── multiply.ll
    │   ├── fibonacci.bc
    │   ├── add.bc
    │   └── multiply.bc
    ├── test2_dynamic_lib/
    │   ├── bb_info.csv
    │   ├── main.ll
    │   └── main.bc
    ├── test3_cpp_static/
    │   ├── bb_info.csv
    │   ├── vector_ops.ll
    │   ├── matrix_ops.ll
    │   └── *.bc
    ├── test4_mixed/
    │   ├── bb_info.csv
    │   ├── compute.ll
    │   ├── compute_wrapper.ll
    │   ├── main.ll
    │   └── *.bc
    ├── test5_optimization/
    │   ├── bb_info.csv
    │   ├── test5_direct          # Direct clang -O2 binary
    │   ├── test5_optimized       # Pipeline-based binary
    │   ├── opt_passes.txt        # Passes applied by opt -O2
    │   ├── llc_passes.txt        # Passes applied by llc -O2
    │   ├── *.ll
    │   ├── *.bc
    │   └── fibonacci.c           # Source used in both
    └── CTestTestfile.cmake       # ctest test registry
```

---

## Build Workflow

### Master CMakeLists.txt

The master build file orchestrates all tests:

1. **Configuration Phase**
   - Finds required tools: `clang`, `clang++`, `opt`, `llc`, `llvm-dis`, `flang`
   - Validates pass plugin exists: `NuggetPasses.so`
   - Sets up CMake test framework

2. **Subdirectory Phase**
   - Calls `add_subdirectory()` for each test (test1-test5)
   - Each test registers its own build steps and tests

3. **Test Registration Phase**
   - Tests 1-4: Each registers 3 tests (CSV generation, format, content)
   - Test 5: Registers 10 tests (CSV validation + optimization verification)
   - Total: 13 tests + 10 tests = 23 tests + 1 summary = 24 test entries

### Individual Test CMakeLists.txt

Each test directory follows this pattern:

```cmake
# 1. Add custom command to compile source → LLVM IR with pass
add_custom_command(
    OUTPUT *.ll *.bc bb_info.csv
    COMMAND clang/clang++/flang [flags] -emit-llvm -c source.c/cpp/f90
    COMMAND opt -load NuggetPasses.so -IRBBLabel < input.bc > output.bc
    COMMAND llvm-dis output.bc -o output.ll
)

# 2. Add custom target for this build step
add_custom_target(testN_build ALL DEPENDS *.ll *.bc bb_info.csv)

# 3. Register 3 ctest tests
add_test(NAME testN_csv_generated ...)
add_test(NAME testN_csv_format ...)
add_test(NAME testN_csv_content ...)
```

---

## CSV Output Format & Validation

### CSV Structure

Each test generates `bb_info.csv` with columns:

| Column | Type | Description |
|--------|------|-------------|
| `FunctionName` | string | LLVM function name (mangled for C++) |
| `FunctionID` | integer | Unique function identifier (0+) |
| `BasicBlockName` | string | BB name (empty if unnamed entry block) |
| `BasicBlockInstCount` | integer | Number of instructions in BB |
| `BasicBlockID` | integer | Unique BB identifier (0+ per function) |

### Example

```csv
FunctionName,FunctionID,BasicBlockName,BasicBlockInstCount,BasicBlockID
fibonacci,0,,2,0
fibonacci,0,tailrecurse._crit_edge,5,1
fibonacci,0,tailrecurse,8,2
add,1,,3,0
multiply,2,,4,0
multiply,2,if.then,5,1
multiply,2,if.else,6,2
```

### Validation Rules

**verify_csv.cmake** checks:

1. **File Exists**: `bb_info.csv` is generated
2. **Header Valid**: Exact column names in correct order
3. **Data Valid**:
   - Row count > 0 (at least one BB)
   - Each row has 5 fields
   - FunctionID, BasicBlockInstCount, BasicBlockID are integers
   - FunctionName is non-empty
   - No duplicate BB IDs within same function

---

## Test-Specific Workflows

### Test 1: Simple C Code

**Purpose**: Validate basic pass functionality on simple C functions

**Source Files**:
- `fibonacci.c` - Recursive function with tail recursion
- `add.c` - Simple binary operation
- `multiply.c` - Conditional logic

**Compilation**:
```bash
clang -emit-llvm -c fibonacci.c -o fibonacci.bc
opt -load NuggetPasses.so -IRBBLabel fibonacci.bc -o fibonacci.bc
llvm-dis fibonacci.bc -o fibonacci.ll
```

**Expected Output**:
- 3 BC files (one per source)
- 3 LL files (disassembled IR with metadata)
- `bb_info.csv` with ~6 total basic blocks

**Validation Tests** (3 total):
1. CSV generated ✓
2. CSV format valid (header + row structure) ✓
3. CSV content valid (data types, row count) ✓

---

### Test 2: Dynamic Library Calls

**Purpose**: Validate pass with external library dependencies

**Source Files**:
- `math_funcs.c` - Functions calling math library (sin, cos, pow)
- `math_funcs.h` - Header declarations
- `main.c` - Entry point calling math_funcs

**Compilation**:
```bash
clang -emit-llvm -c math_funcs.c -o math_funcs.bc -lm
clang -emit-llvm -c main.c -o main.bc
llvm-link math_funcs.bc main.bc -o combined.bc
opt -load NuggetPasses.so -IRBBLabel combined.bc -o combined.bc
```

**Expected Output**:
- Linked LLVM bitcode with external references preserved
- `bb_info.csv` with functions and their BBs
- Symbol resolution for `sin`, `cos`, `pow` maintained

**Validation Tests** (3 total):
1. CSV generated ✓
2. CSV format valid ✓
3. CSV content valid (library calls preserved) ✓

---

### Test 3: C++ Static Code

**Purpose**: Test with C++ language features and name mangling

**Source Files**:
- `vector_ops.hpp` - Template class with overloaded operators
- `matrix_ops.cpp` - Uses templates and lambdas
- `main.cpp` - Driver

**Compilation**:
```bash
clang++ -emit-llvm -c matrix_ops.cpp -o matrix_ops.bc
clang++ -emit-llvm -c main.cpp -o main.bc
llvm-link matrix_ops.bc main.bc -o combined.bc
opt -load NuggetPasses.so -IRBBLabel combined.bc -o combined.bc
```

**Expected Output**:
- Mangled C++ names preserved in CSV
- Template instantiations visible as separate functions
- `bb_info.csv` with C++ function names properly decoded

**Validation Tests** (3 total):
1. CSV generated ✓
2. CSV format valid ✓
3. CSV content valid (C++ names handled correctly) ✓

---

### Test 4: Mixed C++/Fortran

**Purpose**: Test multi-language interoperability

**Source Files**:
- `compute.f90` - Fortran module with subroutines
- `compute.h` - C interface to Fortran
- `compute_wrapper.cpp` - C++ wrapper for Fortran routines
- `main.cpp` - Entry point

**Compilation**:
```bash
flang -emit-llvm -c compute.f90 -o compute.bc
clang++ -emit-llvm -c compute_wrapper.cpp -o wrapper.bc
clang++ -emit-llvm -c main.cpp -o main.bc
llvm-link compute.bc wrapper.bc main.bc -o combined.bc
opt -load NuggetPasses.so -IRBBLabel combined.bc -o combined.bc
```

**Expected Output**:
- Fortran subroutines IR representation
- C++/Fortran linkage preserved
- Symbol resolution across languages
- `bb_info.csv` with all functions

**Validation Tests** (3 total):
1. CSV generated ✓
2. CSV format valid ✓
3. CSV content valid (language interop preserved) ✓

---

### Test 5: Optimization Pipeline Comparison

**Purpose**: Validate that pass doesn't break optimization equivalence

**Dual Pipelines**:

**Pipeline 1: Direct Compilation**
```bash
clang -O2 fibonacci.c -o test5_direct
time ./test5_direct  # Measure performance
```

**Pipeline 2: Multi-stage**
```bash
clang -emit-llvm -c fibonacci_source.c -o input.bc
opt -O2 input.bc -o optimized.bc
opt -load NuggetPasses.so -IRBBLabel optimized.bc -o labeled.bc
llc -O2 labeled.bc -o labeled.s
clang labeled.s fibonacci.c -o test5_optimized
time ./test5_optimized  # Measure performance
```

**Expected Output**:
- Both binaries execute in ~29ms (identical performance)
- `opt_passes.txt` - Detailed passes from `opt -O2` (630 lines)
- `llc_passes.txt` - Detailed passes from `llc -O2` (704 lines)
- `bb_info.csv` with 37 labeled BBs across 6 functions
- Performance comparison report

**Validation Tests** (10 total):
1. CSV generated ✓
2. CSV format valid ✓
3. Both binaries exist ✓
4. opt passes file captured ✓
5. llc passes file captured ✓
6. opt passes file non-empty (>10 lines) ✓
7. llc passes file non-empty (>10 lines) ✓
8. Direct binary runs successfully ✓
9. Optimized binary runs successfully ✓
10. Performance comparison (within 10% tolerance) ✓

---

## Build & Test Execution

### Configuration
```bash
cd llvm-nugget-passes/test
cmake -S . -B build \
  -DLLVM_BIN_DIR=/usr/bin \
  -DPASS_PLUGIN=../pass/build/NuggetPasses.so
```

### Build
```bash
cmake --build build
```

### Run Tests
```bash
cd build && ctest --output-on-failure
```

### Expected Result
```
Test project /path/to/build
    Start  1: summary
    1/23   Test  #1: summary ................................ Passed
    2/23   Test  #2: test1_simple ........................... Passed
    ...
    23/23  Test #23: test5_performance_comparison ........... Passed

100% tests passed, 0 tests failed out of 23
```

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| Separate tests per directory | Isolates concerns, enables parallel development |
| Master CMakeLists.txt | Unified build entry point, easier CI/CD integration |
| CSV format | Human-readable, easy to parse, tool-friendly |
| ctest framework | Standard CMake testing, integrates with CI systems |
| Test verification script | Automated validation, no manual inspection needed |
| Multi-language coverage | Proves pass works across different IR characteristics |
| Optimization comparison | Ensures pass doesn't introduce regressions |

---

## Adding New Tests

### Step 1: Create Directory
```bash
mkdir test6_description
cd test6_description
```

### Step 2: Create CMakeLists.txt
Copy from existing test, modify:
- Source file names
- Number of source files
- Output names
- Test labels

Example template:
```cmake
set(TEST_NAME test6_description)
set(SOURCES source1.c source2.c)

# Compile → LLVM IR with pass
add_custom_command(
    OUTPUT ${TEST_NAME}_out.bc ${TEST_NAME}.csv
    DEPENDS ${SOURCES}
    COMMAND clang -emit-llvm -c ${SOURCES} -o temp.bc
    COMMAND opt -load ${PASS_PLUGIN} -IRBBLabel temp.bc -o ${TEST_NAME}_out.bc
    COMMAND llvm-dis ${TEST_NAME}_out.bc -o ${TEST_NAME}.ll
)

add_custom_target(${TEST_NAME}_build ALL DEPENDS ${TEST_NAME}_out.bc)

# Tests
add_test(NAME ${TEST_NAME}_csv_generated
    COMMAND [ -f ${TEST_NAME}.csv ]
)
add_test(NAME ${TEST_NAME}_csv_format
    COMMAND cmake -P ${CMAKE_SOURCE_DIR}/verify_csv.cmake ...
)
add_test(NAME ${TEST_NAME}_csv_content
    COMMAND cmake -P ${CMAKE_SOURCE_DIR}/verify_csv.cmake ...
)
```

### Step 3: Add Source Files
Create your `.c`, `.cpp`, or `.f90` files

### Step 4: Update Master CMakeLists.txt
```cmake
add_subdirectory(test6_description)
```

### Step 5: Rebuild
```bash
cd build && cmake --build . && ctest --output-on-failure
```

---

## Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| Pass plugin not found | NuggetPasses.so not built | Build: `cd ../pass && cmake --build build` |
| `clang/opt/llc` not found | LLVM not in PATH | Use `-DLLVM_BIN_DIR=/path/to/llvm/bin` |
| CMake can't find LLVM | Missing LLVM development files | Install: `llvm-dev` (Ubuntu) or `llvm-devel` (Fedora) |
| Linking fails for test4 | Flang not installed | Optional for test4; skip if not needed |
| CSV validation fails | CSV has wrong format | Check source file changes, inspect `bb_info.csv` directly |
| Tests not discovered | Need CMake reconfiguration | Run: `rm -rf build && cmake -S . -B build ...` |

---

## Performance Characteristics

Based on Test 5 measurements:

| Metric | Value |
|--------|-------|
| Direct compilation time (clang -O2) | ~50ms |
| IR generation (clang -emit-llvm) | ~20ms |
| Pass execution (-IRBBLabel) | ~5ms |
| Optimization (opt -O2) | ~15ms |
| Code gen (llc -O2) | ~30ms |
| Total pipeline time | ~100ms |
| **Runtime performance delta** | **0% (no regression)** |

---

## Configuration Files

### verify_csv.cmake
Module that validates CSV files:
- Checks file existence
- Validates header row
- Validates data rows
- Provides detailed error messages

Usage in CMakeLists.txt:
```cmake
add_test(NAME test_csv_format
    COMMAND cmake -P verify_csv.cmake
    -DCSV_FILE=bb_info.csv
    -DMODE=format
)
```

### Master CMakeLists.txt Sections

1. **Header** - Project setup, CMake version
2. **Tools Discovery** - Finds clang, opt, llc, etc.
3. **Validation** - Checks pass plugin availability
4. **Subdirectories** - Imports each test
5. **Global Tests** - Integration tests (if any)

---

## Dependencies

- **Required**: LLVM 18+, CMake 3.20+, Clang
- **Optional**: Flang (for test4), Fortran compiler
- **Internal**: NuggetPasses.so from `../pass/`

---

See [README.md](README.md) for quick start and test results summary.
