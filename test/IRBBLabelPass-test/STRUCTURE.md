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
│   ├── fibonacci.c                         # Linked in both pipelines
│   ├── capture_passes.sh                   # Script to capture opt/llc passes
│   └── compare_passes.sh                   # Script to compare passes & generate report
│
├── test6_custom_output/                    # Test 6: Custom parameter override
│   ├── CMakeLists.txt
│   └── test6_custom_output.c               # Tests custom output filename
│
├── common/                                 # Shared utilities
│   ├── verify_csv.cmake                    # CSV format validation
│   └── verify_metadata.py                  # IR metadata validation
│
└── build/                                  # Generated (cmake --build build)
    ├── test1_simple/
    │   ├── bb_info.csv
    │   ├── test1_simple_instrumented.ll    # IR with !bb.id metadata
    │   └── test1_simple_instrumented.bc
    ├── test2_dynamic_lib/
    │   ├── bb_info.csv
    │   ├── test2_dynamic_lib_instrumented.ll
    │   └── test2_dynamic_lib_instrumented.bc
    ├── test3_cpp_static/
    │   ├── bb_info.csv
    │   ├── test3_cpp_static_instrumented.ll
    │   └── test3_cpp_static_instrumented.bc
    ├── test4_mixed/
    │   ├── bb_info.csv
    │   ├── test4_mixed_instrumented.ll
    │   └── test4_mixed_instrumented.bc
    ├── test5_optimization/
    │   ├── bb_info.csv
    │   ├── test5_direct              # Direct clang -O2 binary
    │   ├── test5_optimized           # Pipeline-based binary
    │   ├── test5_labeled_instrumented.ll  # Labeled IR with metadata
    │   ├── direct_passes.txt         # clang optimization passes
    │   ├── opt_passes.txt            # opt -O2 passes applied
    │   ├── llc_passes.txt            # llc -O2 passes applied
    │   ├── passes_comparison.txt     # Detailed pass comparison report
    │   └── *.bc
    ├── test6_custom_output/
    │   ├── my_custom_bb_output.csv   # Custom-named CSV output
    │   ├── test6_custom_output_instrumented.ll
    │   └── test6_custom_output_instrumented.bc
    └── CTestTestfile.cmake           # ctest test registry
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
   - Calls `add_subdirectory()` for each test (test1-test6)
   - Each test registers its own build steps and tests

3. **Test Registration Phase**
   - Tests 1-4: Each registers 4 tests (CSV exists, format, content, metadata validation)
   - Test 5: Registers 11 tests (binaries + passes + CSV + metadata validation)
   - Test 6: Registers 4 tests (custom CSV + metadata validation)
   - Total: 16 + 11 = 27 tests

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

# 3. Register 4 ctest tests
add_test(NAME testN_csv_exists ...)
add_test(NAME testN_csv_format ...)
add_test(NAME testN_csv_content ...)
add_test(NAME testN_metadata_validation ...)
```

### Metadata Validation

**verify_metadata.py** performs comprehensive validation:

1. **Parses instrumented IR**: Extracts `!bb.id` metadata from `*_instrumented.ll` files
2. **Parses CSV**: Reads basic block information from `bb_info.csv`
3. **Cross-validates**:
   - All functions in CSV exist in IR
   - All basic blocks have matching IDs
   - BB names match (handles unnamed blocks, numeric labels, complex optimized names)
4. **Handles edge cases**:
   - Mangled C++ function names (e.g., `_ZNK3$_0clEv`)
   - Various linkage types (dso_local, internal, linkonce_odr)
   - Optimized BB labels with hyphens (e.g., `._crit_edge.unr-lcssa`)
   - Unnamed basic blocks (numeric labels like `14:` treated as empty)

**Example validation**:
```python
python3 verify_metadata.py test1_simple_instrumented.ll bb_info.csv
# ✓ Metadata validation PASSED
#   - Functions: 3
#   - Total basic blocks: 6
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
- `test1_simple_instrumented.bc` - Bitcode with `!bb.id` metadata
- `test1_simple_instrumented.ll` - Readable IR with metadata annotations
- `bb_info.csv` with ~6 total basic blocks

**Validation Tests** (4 total):
1. CSV exists (with valid header) ✓
2. CSV format valid (column structure) ✓
3. CSV content valid (data types, row count) ✓
4. Metadata valid (IR annotations match CSV) ✓

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
- `test2_dynamic_lib_instrumented.bc/.ll` with metadata
- `bb_info.csv` with functions and their BBs
- Symbol resolution for `sin`, `cos`, `pow` maintained

**Validation Tests** (4 total):
1. CSV exists ✓
2. CSV format valid ✓
3. CSV content valid (library calls preserved) ✓
4. Metadata valid ✓

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
- `test3_cpp_static_instrumented.bc/.ll` with metadata
- Mangled C++ names preserved in CSV and IR
- Template instantiations visible as separate functions
- `bb_info.csv` with C++ function names

**Validation Tests** (4 total):
1. CSV exists ✓
2. CSV format valid ✓
3. CSV content valid (C++ names handled correctly) ✓
4. Metadata valid (handles mangled names like `_ZNK3$_0clEv`) ✓

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
- `test4_mixed_instrumented.bc/.ll` with metadata
- Fortran subroutines IR representation
- C++/Fortran linkage preserved
- Symbol resolution across languages
- `bb_info.csv` with all functions

**Validation Tests** (4 total):
1. CSV exists ✓
2. CSV format valid ✓
3. CSV content valid (language interop preserved) ✓
4. Metadata valid (cross-language validation) ✓

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
with similar performance (~29ms)
- `test5_labeled_instrumented.ll` - IR with metadata from pipelined build
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
4. Direct passes file captured ✓
5. opt passes file captured ✓
6. llc passes file captured ✓
7. Pass comparison report generated ✓
8. Direct binary runs successfully ✓
9. Optimized binary runs successfully ✓
10. Performance comparison (within 5% tolerance) ✓

**New Infrastructure**:
- `capture_passes.sh` - Captures optimization passes from clang/opt/llc tools
- `compare_passes.sh` - Generates detailed pass comparison report with statistics
- `direct_passes.txt` - Passes applied by direct clang compilation
- `passes_comparison.txt` - Per-function and per-pass analysis report

---

### Test 6: Custom Parameter Override

**Purpose**: Validate pass parameter parsing with custom output filename

**Source Files**:
- `test6_custom_output.c` - Simple C code with multiply and divide functions

**Compilation & Execution**:
```bash
clang -O0 -emit-llvm -S test6_custom_output.c -o test6_custom_output.ll
opt -load-pass-plugin=NuggetPasses.so \
    -passes="ir-bb-label-pass<output_csv=my_custom_bb_output.csv>" \
    test6_custom_output.ll -o test6_custom_output.bc
```

**Parameter Parsing Features**:
- Pass name: `ir-bb-label-pass`
- Option syntax: `<key=value>`
- Default option: `output_csv` with value `bb_info.csv`
- Override: Custom value passed via `-passes` argument

**Expected Output**:
- CSV file created with **custom name** (`my_custom_bb_output.csv`)
- `test6_custom_output_instrumented.ll` - IR with metadata
- Basic block information properly labeled
- CSV format and content valid

**Validation Tests** (4 total):
1. Custom CSV file exists ✓
2. CSV format valid ✓
3. CSV content valid (with custom filename) ✓
4. Metadata valid ✓

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
    Start  1: test1_simple ................................ Passed
    1/26   Test  #1: test1_simple ........................... Passed
    2/26   Test  #2: test1_csv_exists ....................... Passed
    ...
    25/26  Test #25: test5_passes_comparison ............... Passed
    26/26  Test #26: test6_custom_output ................... Passed

100% tests passed, 0 tests failed out of 26
```

**Note**: Useless echo-only summary test removed in cleanup phase. Tests are organized by case with dependencies.

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

## Implementation Details

### Code Style & Standards

All code follows **Google C++ Style Guide** conventions:

| Element | Convention | Example |
|---------|-----------|---------|
| Class Names | PascalCase | `IRBBLabelPass`, `BasicBlockInfo` |
| Function Names | snake_case | `parse_options()`, `match_param_pass()` |
| Variable Names | snake_case | `function_name`, `basic_block_id` |
| Macro Names | UPPER_CASE | `DEBUG_PRINT`, `kBbIdKey` |
| Constants | kName format | `kBbIdKey = "bb_id"` |

### Conditional Debug Output

The pass uses a `DEBUG_PRINT` macro for conditional debug output:

```cpp
#ifdef DEBUG
#define DEBUG_PRINT(stream, msg) (stream) << (msg)
#else
#define DEBUG_PRINT(stream, msg)
#endif
```

**Usage**:
```cpp
DEBUG_PRINT(errs(), "Processing function: " << func_name << "\n");
```

**Compilation**:
- Default (no debug): `cmake -S . -B build ...`
- With debug output: `cmake -S . -B build -DDEBUG=ON ...`

### Pass Parameter Parsing

The `ParseOptions()` function handles custom parameters:

```cpp
auto opts = ParseOptions("-passes=\"ir-bb-label-pass<output_csv=custom.csv>\"");
// Returns: Expected<std::vector<Options>> with parsed key-value pairs
```

Supported options:
- `output_csv` - Custom CSV output filename (default: `bb_info.csv`)

### Test Infrastructure Cleanup

Recent cleanup removed useless echo-only placeholder tests:
- ✅ Removed no-op echo tests that always passed
- ✅ Replaced with actual validation tests
- ✅ Test count reduced from 27 to 26 entries
- ✅ All tests now provide meaningful validation

---

See [README.md](README.md) for quick start and test results summary.
