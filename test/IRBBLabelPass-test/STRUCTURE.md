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
│   ├── capture_passes.sh                   # Script to capture all passes
│   ├── compare_passes.sh                   # Comparison wrapper
│   ├── compare_passes_detailed.py          # Enhanced Python comparison
│   ├── compare_performance.sh              # Performance testing
│   ├── test5_optimization.c                # Test source code
│   └── (generated at build time: direct_passes.txt, opt_passes.txt, llc_passes.txt, direct_machine_passes.txt, PASSES_COMPARISON_REPORT.md)
│
├── test6_custom_output/                    # Test 6: Custom parameter override
│   ├── CMakeLists.txt
│   └── test6_custom_output.c               # Tests custom output filename
│
└── common/                                 # Shared utilities
    ├── verify_csv.cmake                    # CSV format validation
    └── verify_metadata.py                  # IR metadata validation

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
   - Tests 1-4: Each registers tests for CSV validation and metadata verification
   - Test 5: Registers tests for binary compilation, performance, and pass comparison
   - Test 6: Registers tests for custom parameter handling

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
- `bb_info.csv` - Basic block information

**Validations**:
- CSV exists with valid header
- CSV format valid (column structure)
- CSV content valid (data types, row count)
- Metadata valid (IR annotations match CSV)

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

**Validations**:
- CSV exists
- CSV format valid
- CSV content valid (library calls preserved)
- Metadata valid

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

**Validations**:
- CSV exists
- CSV format valid
- CSV content valid (C++ names handled correctly)
- Metadata valid (handles mangled names like `_ZNK3$_0clEv`)

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

**Validations**:
- CSV exists
- CSV format valid
- CSV content valid (language interop preserved)
- Metadata valid (cross-language validation)

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
- `test5_labeled_instrumented.ll` - IR with metadata from pipelined build

**Expected Output**:
- Both binaries execute with equivalent performance
- `direct_passes.txt` - IR passes from direct compilation
- `opt_passes.txt` - IR passes from opt -O2
- `llc_passes.txt` - Machine passes from llc -O2
- `direct_machine_passes.txt` - Machine passes from direct compilation
- `PASSES_COMPARISON_REPORT.md` - Comprehensive analysis
- `bb_info.csv` with labeled basic blocks

**Validations**:
- CSV exists
- CSV format valid
- Both binaries exist and execute successfully
- Performance comparison within tolerance
- Pass comparison report generated

**Infrastructure**:
- `capture_passes.sh` - Captures optimization passes from clang/opt/llc
- `compare_passes.sh` - Wrapper for comparison with auto-detection
- `compare_passes_detailed.py` - Enhanced Python comparison script
- `compare_performance.sh` - Performance measurements

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

**Validations**:
- Custom CSV file exists
- CSV format valid
- CSV content valid
- Metadata valid

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

---

See [README.md](README.md) for quick start and test overview.
