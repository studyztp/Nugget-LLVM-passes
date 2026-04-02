# Nugget LLVM Passes

[![Daily LLVM Tip-of-Tree Test](https://github.com/studyztp/Nugget-LLVM-passes/actions/workflows/daily-llvm-test.yml/badge.svg)](https://github.com/studyztp/Nugget-LLVM-passes/actions/workflows/daily-llvm-test.yml)

A collection of LLVM analysis and instrumentation passes for basic block labeling, phase analysis, and region-of-interest (ROI) boundary marking. These passes enable fine-grained program behavior analysis, performance phase detection, and targeted instrumentation for profiling and simulation tools.

<!-- LLVM_TEST_RESULTS_START -->
### Latest LLVM Tip-of-Tree Test Results

| | |
|---|---|
| **Status** | ✅ passing |
| **LLVM Version** | `23.0.0` |
| **Tests Passed** | 41 / 41 |
| **Run Date** | 2026-04-02 |
| **Workflow** | [View Run](https://github.com/studyztp/Nugget-LLVM-passes/actions/runs/23887519626) |

<details>
<summary>Per-test results (click to expand)</summary>

| Test | Result |
|------|--------|
| IRBBLabelPass-test1_simple_csv_exists | Passed |
| IRBBLabelPass-test1_simple_csv_has_content | Passed |
| IRBBLabelPass-test1_simple_metadata_validation | Passed |
| test2_dynamic_lib_csv_exists | Passed |
| test2_dynamic_lib_csv_has_content | Passed |
| test2_dynamic_lib_metadata_validation | Passed |
| test3_cpp_static_csv_exists | Passed |
| test3_cpp_static_csv_has_content | Passed |
| test3_cpp_static_metadata_validation | Passed |
| test4_mixed_csv_exists | Passed |
| test4_mixed_csv_has_content | Passed |
| test4_mixed_metadata_validation | Passed |
| test5_optimization_csv_exists | Passed |
| test5_optimization_binaries_exist | Passed |
| test5_direct_binary_runs | Passed |
| test5_optimized_binary_runs | Passed |
| test5_performance_comparison | Passed |
| test5_passes_comparison | Passed |
| test5_metadata_validation | Passed |
| test6_custom_csv_exists | Passed |
| test6_custom_csv_has_content | Passed |
| test6_custom_metadata_validation | Passed |
| IRBBLabelPass-test7_unknown_option | Passed |
| IRBBLabelPass-test7_missing_required_option | Passed |
| PhaseAnalysisPass-test1_simple_csv_exists | Passed |
| PhaseAnalysisPass-test1_simple_instrumentation_validation | Passed |
| PhaseAnalysisPass-test3_no_label_should_fail | Passed |
| PhaseAnalysisPass-test4_missing_runtime_should_fail | Passed |
| PhaseAnalysisPass-test5_pass_chaining_csv_exists | Passed |
| PhaseAnalysisPass-test5_pass_chaining_instrumentation_validation | Passed |
| test2_machine_match_csv_exists | Passed |
| test2_machine_match_executable_exists | Passed |
| test2_machine_match_runs | Passed |
| test2_machine_match_verification | Passed |
| PhaseBoundPass-test1_simple_csv_exists | Passed |
| PhaseBoundPass-test1_simple_instrumentation_validation | Passed |
| test_label_only_disasm_labels | Passed |
| test_warmup_count_zero_disasm_labels | Passed |
| PhaseBoundPass-test_warmup_zero_instr_csv_exists | Passed |
| PhaseBoundPass-test_warmup_zero_instr_validation | Passed |
| test_invalid_bb_id_should_fail | Passed |

</details>
<!-- LLVM_TEST_RESULTS_END -->

## Overview

This repository contains three main LLVM passes:

- **IRBBLabelPass**: Labels IR basic blocks with unique identifiers and exports structural information
- **PhaseAnalysisPass**: Instruments basic blocks for runtime phase detection and analysis
- **PhaseBoundPass**: Marks specific program phases (warmup/start/end) for ROI-based analysis

**Tested with the Ubuntu 24.04 packaged LLVM-18 (x86_64 and aarch64) and the latest GitHub LLVM (daily CI)**

## Features

- ✅ **Stable Basic Block IDs**: Metadata-based labeling survives optimizations
- ✅ **Flexible Instrumentation**: Parameterized passes for different analysis scenarios
- ✅ **CSV Export**: Machine-readable basic block information for external tools
- ✅ **Runtime Hooks**: Integration with custom profiling/simulation frameworks
- ✅ **Comprehensive Tests**: Extensive test suite with validation scripts

---

## Building the Passes

### Prerequisites

- CMake ≥ 3.20
- LLVM installation (with development files)
- C++17-compatible compiler

### Build Instructions

1. **Configure the build** with your LLVM installation:

```bash
cmake -S . -B build -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm
```

**Important**: You must specify `LLVM_DIR` pointing to your LLVM installation's CMake directory. This ensures the passes are built against your intended LLVM version.

2. **Build the plugin**:

```bash
cmake --build build
```

This produces `build/NuggetPasses.so` (Linux), `NuggetPasses.dylib` (macOS), or `NuggetPasses.dll` (Windows).

### Building with Tests Enabled

To build the passes and run tests in one step:

```bash
cmake -S . -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DENABLE_TESTS=ON \
  -DLLVM_BIN_DIR=/path/to/llvm/bin

cmake --build build
ctest --test-dir build --output-on-failure
```

- `ENABLE_TESTS=ON`: Enables test building
- `LLVM_BIN_DIR`: Path to LLVM tools (`clang`, `opt`, etc.) for running tests
  - If not specified, automatically derived from `LLVM_DIR`

---

## Pass Descriptions and Usage

### 1. IRBBLabelPass — Basic Block Labeling

**Purpose**: Assigns globally unique IDs to every basic block in a module via metadata and exports structural information to CSV.

**When to use**:
- Before applying other Nugget passes (PhaseAnalysis/PhaseBound require labeled BBs)
- For basic block-level program analysis
- To maintain stable BB identifiers across optimization levels

#### How it works

1. Iterates through all basic blocks in the module
2. Assigns each BB a unique integer ID (starting from 0)
3. Attaches `!bb.id` metadata to the basic block's terminator instruction
4. Collects BB statistics: function name, instruction count, etc.
5. Exports all information to a CSV file

#### Usage

```bash
# Basic usage with default output (bb_info.csv)
opt -load-pass-plugin=./build/NuggetPasses.so \
    -passes="ir-bb-label-pass" \
    input.ll -o output.bc

# Custom output filename
opt -load-pass-plugin=./build/NuggetPasses.so \
    -passes="ir-bb-label-pass<output_csv=my_results.csv>" \
    input.ll -o output.bc
```

#### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `output_csv` | `bb_info.csv` | Output CSV filename for basic block information |

#### Output Format

The CSV file contains one row per basic block with the following columns:

```csv
FunctionName,FunctionID,BasicBlockName,BasicBlockInstCount,BasicBlockID
main,0,,5,0
main,0,if.then,3,1
main,0,if.end,2,2
helper,1,,8,3
helper,1,loop.body,4,4
```

- **FunctionName**: Name of the parent function
- **FunctionID**: Sequential function ID (0-indexed)
- **BasicBlockName**: LLVM basic block label (empty for entry blocks)
- **BasicBlockInstCount**: Number of IR instructions in the BB
- **BasicBlockID**: Globally unique basic block ID

#### Metadata Format

Each basic block's terminator instruction receives `!bb.id` metadata:

```llvm
br label %if.end, !bb.id !2
!2 = !{!"1"}
```

This metadata persists through optimization passes and can be queried by subsequent analysis tools.

#### Runtime Linking Note

The workload source code references `nugget_roi_begin_()` and `nugget_roi_end_()` to mark the region of interest. The `labeled.bc` output from this pass will contain declarations for these symbols, so a runtime library providing their definitions (e.g., `nugget_roi_runtime.c`) must be linked before producing an executable. However, this runtime must **not** be merged into the bitcode **before** IRBBLabelPass — otherwise the pass would label the runtime's basic blocks, and downstream passes would unnecessarily analyze nugget worker functions.

Each pass uses a **different** runtime library:
- **IRBBLabelPass**: only needs `nugget_roi_begin_` / `nugget_roi_end_` (provided by the workload or a minimal ROI runtime)
- **PhaseAnalysisPass**: needs `nugget_init`, `nugget_bb_hook`, and the ROI functions (phase analysis runtime)
- **PhaseBoundPass**: needs `nugget_init`, `nugget_warmup_marker_hook`, `nugget_start_marker_hook`, `nugget_end_marker_hook`, and the ROI functions (phase bound runtime)

The pass-specific runtime is merged with `labeled.bc` **after** this pass but **before** the corresponding downstream pass. See the PhaseAnalysisPass and PhaseBoundPass sections for details.

---

### 2. PhaseAnalysisPass — Phase Detection Instrumentation

**Purpose**: Instruments every labeled basic block with runtime hooks for phase detection and interval-based analysis.

**When to use**:
- Detecting program execution phases (initialization, computation, I/O, etc.)
- Collecting basic block execution frequency over intervals
- Building phase graphs for architecture simulation

#### How it works

1. Reads `!bb.id` metadata from basic blocks (requires IRBBLabelPass first)
2. Inserts calls to `nugget_bb_hook(bb_size, bb_id, threshold)` before each terminator
3. Inserts initialization call to `nugget_init(total_bb_count)` at ROI begin

The runtime hooks (implemented by your runtime library) can:
- Track basic block execution counts
- Trigger phase transitions after N instructions
- Export phase vectors for analysis

#### Usage

```bash
# Step 1: Label basic blocks first
opt -load-pass-plugin=./build/NuggetPasses.so \
    -passes="ir-bb-label-pass" \
    input.ll -o labeled.bc

# Step 2: Compile the phase analysis runtime to bitcode and merge with
# labeled bitcode. The merge must happen AFTER ir-bb-label-pass (so the
# runtime's basic blocks are not labeled — we don't want to analyze
# worker functions used for runtime data collection) and BEFORE
# phase-analysis-pass (so the pass recognizes runtime functions and
# does not instrument their basic blocks with hooks).
clang -S -emit-llvm nugget_phase_analysis_runtime.c -o nugget_phase_analysis_runtime.ll
llvm-link labeled.bc nugget_phase_analysis_runtime.ll -o merged.bc

# Step 3: Apply phase analysis instrumentation
opt -load-pass-plugin=./build/NuggetPasses.so \
    -passes="phase-analysis-pass<interval_length=10000>" \
    merged.bc -o instrumented.bc

# Step 4: Lower to object file and link (avoid uncontrolled optimizations / LTO)
llc -O2 -filetype=obj -relocation-model=pic instrumented.bc -o program.o
clang program.o -o program
```

#### Parameters

| Parameter | Required | Description |
|-----------|----------|-------------|
| `interval_length` | ✅ Yes | Interval length in IR instructions executed before triggering phase analysis |

#### Runtime Integration

Your runtime library must provide:

```c
// Called once at program ROI start
void nugget_init(uint64_t total_bb_count);

// Called at the end of each basic block (before the terminator)
void nugget_bb_hook(uint64_t bb_size, uint64_t bb_id, uint64_t threshold);

// Marker function for ROI begin (insert in your code)
void nugget_roi_begin_() { /* user code */ }
```

**Example runtime** (see [test/PhaseAnalysisPass-test/common/nugget_runtime.c](test/PhaseAnalysisPass-test/common/nugget_runtime.c)):

```c
uint64_t instruction_counter = 0;
uint64_t interval_length = 0;
uint64_t* bb_vector = NULL;

void nugget_init(uint64_t total_bb_count) {
    bb_vector = calloc(total_bb_count, sizeof(uint64_t));
    // Initialize interval_length from environment or config
}

void nugget_bb_hook(uint64_t bb_size, uint64_t bb_id, uint64_t threshold) {
    instruction_counter += bb_size;
    bb_vector[bb_id]++;
    
    if (instruction_counter >= threshold) {
        // Phase boundary reached - output bb_vector
        print_phase_vector(bb_vector);
        memset(bb_vector, 0, total_bb_count * sizeof(uint64_t));
        instruction_counter = 0;
    }
}
```

#### Expected Workflow

1. **Label BBs** with IRBBLabelPass to produce `labeled.bc`
2. **Merge** the phase analysis runtime bitcode into `labeled.bc` (after labeling so the runtime is not labeled; before the pass so it is not instrumented)
3. **Instrument** with PhaseAnalysisPass
4. **Compile** to executable via `llc` + linker (no uncontrolled optimizations)
5. **Run** program — hooks collect data automatically
6. **Analyze** phase output (vectors, transition graphs, etc.)

---

### 3. PhaseBoundPass — Region-of-Interest Marking

**Purpose**: Instruments specific "marker" basic blocks to define warmup, start, and end boundaries for region-of-interest (ROI) analysis.

**When to use**:
- Skipping initialization/warmup in benchmarks
- Measuring specific code regions (e.g., main loop only)
- Synchronizing simulator warmup with program phases

#### How it works

1. Identifies specific basic blocks by their `!bb.id` metadata
2. Instruments chosen BBs with marker hooks:
   - `nugget_warmup_marker_hook()` — Warmup phase marker
   - `nugget_start_marker_hook()` — ROI start marker
   - `nugget_end_marker_hook()` — ROI end marker
3. Inserts initialization call with marker counts at ROI begin

The runtime tracks how many times each marker executes and triggers actions (start profiling, stop simulation, etc.) when counts are reached.

#### Usage

```bash
# Prerequisites:
#   - You must already have labeled.bc from IRBBLabelPass (the SAME labeled.bc
#     used for phase analysis, so IRBB info is consistent).
#   - You must have already run the phase analysis executable and determined
#     the marker BB IDs and execution counts from its output.

# Step 1: Compile the phase bound runtime to bitcode and merge with
# labeled bitcode. The merge must happen AFTER ir-bb-label-pass (so the
# runtime's basic blocks are not labeled — we don't want to analyze
# worker functions used for runtime marking) and BEFORE phase-bound-pass
# (so the pass recognizes runtime functions and does not instrument their
# basic blocks with hooks).
# NOTE: This uses a different runtime library than phase analysis.
clang -S -emit-llvm nugget_phase_bound_runtime.c -o nugget_phase_bound_runtime.ll
llvm-link labeled.bc nugget_phase_bound_runtime.ll -o merged.bc

# Step 2: Apply phase bound instrumentation using marker info from analysis
opt -load-pass-plugin=./build/NuggetPasses.so \
    -passes="phase-bound-pass<warmup_marker_bb_id=10;warmup_marker_count=1000;start_marker_bb_id=25;start_marker_count=100;end_marker_bb_id=30;end_marker_count=100>" \
    merged.bc -o instrumented.bc

# Step 3: Lower to object file and link (avoid uncontrolled optimizations / LTO)
llc -O2 -filetype=obj -relocation-model=pic instrumented.bc -o program.o
clang program.o -o program
```

#### Parameters

All marker parameters are **required**:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `warmup_marker_bb_id` | *(required)* | Basic block ID for warmup marker |
| `warmup_marker_count` | *(required)* | Number of executions before warmup completes (0 to skip warmup) |
| `start_marker_bb_id` | *(required)* | Basic block ID for ROI start marker |
| `start_marker_count` | *(required)* | Number of executions before ROI starts |
| `end_marker_bb_id` | *(required)* | Basic block ID for ROI end marker |
| `end_marker_count` | *(required)* | Number of executions before ROI ends |
| `label_only` | `false` | If `true`, insert inline assembly labels instead of hook function calls. Useful for binary analysis without runtime library linking. |

**Note**: Use semicolons (`;`) to separate multiple parameters in the pass syntax.

#### Runtime Integration

Your runtime library must provide:

```c
// Called once at program ROI start
void nugget_init(uint64_t warmup_count, uint64_t start_count, uint64_t end_count);

// Called when warmup marker BB executes
void nugget_warmup_marker_hook();

// Called when start marker BB executes
void nugget_start_marker_hook();

// Called when end marker BB executes
void nugget_end_marker_hook();

// Marker function for ROI begin (insert in your code)
void nugget_roi_begin_() { /* user code */ }
```

**Example runtime** (see [test/PhaseBoundPass-test/common/nugget_runtime.c](test/PhaseBoundPass-test/common/nugget_runtime.c)):

```c
uint64_t warmup_count = 0, start_count = 0, end_count = 0;
uint64_t warmup_seen = 0, start_seen = 0, end_seen = 0;
bool warmup_done = false, roi_started = false, roi_ended = false;

void nugget_init(uint64_t w, uint64_t s, uint64_t e) {
    warmup_count = w; start_count = s; end_count = e;
}

void nugget_warmup_marker_hook() {
    if (++warmup_seen >= warmup_count && !warmup_done) {
        printf("Warmup complete\n");
        warmup_done = true;
    }
}

void nugget_start_marker_hook() {
    if (warmup_done && ++start_seen >= start_count && !roi_started) {
        printf("ROI started\n");
        roi_started = true;
        // Start profiling/tracing here
    }
}

void nugget_end_marker_hook() {
    if (roi_started && ++end_seen >= end_count && !roi_ended) {
        printf("ROI ended\n");
        roi_ended = true;
        // Stop profiling/tracing here
        exit(0);  // Optional: terminate program
    }
}
```

#### Typical Use Case

1. **Run IRBBLabelPass** to get `labeled.bc` and BB IDs from CSV
2. **Run PhaseAnalysisPass** on the same `labeled.bc` to collect phase data
3. **Analyze** phase analysis output to determine marker BB IDs and execution counts
4. **Merge** runtime into the same `labeled.bc` and **instrument** with PhaseBoundPass
5. **Compile and run** — ROI automatically captured between markers

---

## Complete Example Workflow

Here's a complete example combining all three passes:

```bash
# 1. Compile source to IR
clang -S -emit-llvm -O2 benchmark.c -o benchmark.ll

# 2. Label basic blocks (produces labeled.bc used by ALL downstream passes)
opt -load-pass-plugin=./build/NuggetPasses.so \
    -passes="ir-bb-label-pass<output_csv=benchmark_bbs.csv>" \
    benchmark.ll -o labeled.bc

# --- Phase Analysis ---
# Each pass uses a DIFFERENT runtime library. The phase analysis runtime
# provides nugget_init, nugget_bb_hook, and the ROI functions.

# 3. Merge the phase analysis runtime into labeled.bc.
#    Must be AFTER ir-bb-label-pass (so runtime BBs are not labeled) and
#    BEFORE phase-analysis-pass (so runtime functions are not instrumented).
clang -S -emit-llvm nugget_phase_analysis_runtime.c -o nugget_phase_analysis_runtime.ll
llvm-link labeled.bc nugget_phase_analysis_runtime.ll -o merged_analysis.bc

# 4. Apply phase analysis instrumentation
opt -load-pass-plugin=./build/NuggetPasses.so \
    -passes="phase-analysis-pass<interval_length=100000>" \
    merged_analysis.bc -o benchmark_analysis.bc

# 5. Compile and run the analysis. Examine its output to determine the
#    marker BB IDs and execution counts for the region you want to sample.
llc -O2 -filetype=obj -relocation-model=pic benchmark_analysis.bc -o benchmark_analysis.o
clang benchmark_analysis.o -o benchmark_analysis
./benchmark_analysis

# --- Phase Bound ---
# The phase bound runtime provides nugget_init, nugget_warmup_marker_hook,
# nugget_start_marker_hook, nugget_end_marker_hook, and the ROI functions.
# We start again from the SAME labeled.bc to keep IRBB info consistent.

# 6. Merge the phase bound runtime into labeled.bc.
clang -S -emit-llvm nugget_phase_bound_runtime.c -o nugget_phase_bound_runtime.ll
llvm-link labeled.bc nugget_phase_bound_runtime.ll -o merged_bound.bc

# 7. Apply phase bound instrumentation (using marker info from step 5)
opt -load-pass-plugin=./build/NuggetPasses.so \
    -passes="phase-bound-pass<warmup_marker_bb_id=42;warmup_marker_count=1000;start_marker_bb_id=42;start_marker_count=100;end_marker_bb_id=42;end_marker_count=1000>" \
    merged_bound.bc -o sample_bound.bc

# 8. Compile to executable
llc -O2 -filetype=obj -relocation-model=pic sample_bound.bc -o benchmark_sample_bound.o
clang benchmark_sample_bound.o -o benchmark_sample_bound
./benchmark_sample_bound

# --- Optional: build original benchmark ---
# The IRBB metadata doesn't affect the executable, but you still need to
# link a placeholder runtime to satisfy the nugget_roi_begin_/nugget_roi_end_
# symbols declared by the workload.
clang -S -emit-llvm nugget_placeholder_runtime.c -o nugget_placeholder_runtime.ll
llvm-link labeled.bc nugget_placeholder_runtime.ll -o merged_placeholder.bc
llc -O2 -filetype=obj -relocation-model=pic merged_placeholder.bc -o original.o
clang original.o -o original
```

---

## CMake Integration with `nugget_util`

The **[`nugget_util`](nugget_util/)** library provides CMake functions that automate the entire Nugget pipeline — compiling sources to LLVM IR, applying passes, merging runtime libraries, lowering to object files, and linking executables — making it easy to integrate Nugget into any CMake-based workload build flow.

See the [`nugget_util` README](nugget_util/README.md) for full documentation, configurable variables, function reference, and usage examples.

---

## Testing

The repository includes a comprehensive test suite for all passes. Tests validate:

- ✅ Metadata correctness (`!bb.id` attached properly)
- ✅ CSV export format and completeness
- ✅ Runtime hook instrumentation (function calls inserted correctly)
- ✅ ROI initialization at `nugget_roi_begin_`
- ✅ Cross-language support (C, C++, Fortran)
- ✅ Compatibility with optimization levels (-O0 and -O2)

### Running Tests

#### Quick Start (From Build Directory)

If you built with `ENABLE_TESTS=ON`:

```bash
cd build
ctest --output-on-failure
```

#### Detailed Test Run

```bash
# Configure with tests enabled
cmake -S . -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DENABLE_TESTS=ON \
  -DLLVM_BIN_DIR=/path/to/llvm/bin

# Build everything (pass plugin + tests)
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test suite
ctest --test-dir build -R IRBBLabelPass
ctest --test-dir build -R PhaseAnalysisPass
ctest --test-dir build -R PhaseBoundPass

# Run specific test
ctest --test-dir build -R IRBBLabelPass-test1_simple
```

### Test Structure

Each pass has its own test suite in the `test/` directory:

- **[test/IRBBLabelPass-test/](test/IRBBLabelPass-test/)**: Tests for basic block labeling
  - Simple C programs
  - Dynamic libraries
  - C++ static analysis
  - Mixed C++/Fortran
  - Optimization levels
  - Custom output paths

- **[test/PhaseAnalysisPass-test/](test/PhaseAnalysisPass-test/)**: Tests for phase analysis instrumentation
  - Simple instrumentation checks
  - Machine code validation (assembly output)

- **[test/PhaseBoundPass-test/](test/PhaseBoundPass-test/)**: Tests for ROI marking
  - Marker placement validation
  - Runtime integration tests

See [test/README.md](test/README.md) for detailed test documentation and standalone test execution instructions.

---

## Project Structure

```
Nugget-LLVM-passes/
├── CMakeLists.txt              # Main build configuration
├── README.md                   # This file
├── LICENSE                     # BSD-3-Clause license
├── src/                        # Pass implementations
│   ├── PluginRegistration.cpp  # Pass registration with LLVM
│   ├── IRBBLabelPass.cpp/hh    # Basic block labeling pass
│   ├── PhaseAnalysisPass.cpp/hh # Phase detection instrumentation
│   ├── PhaseBoundPass.cpp/hh   # ROI marker instrumentation
│   └── common.hh               # Shared utilities and definitions
├── nugget_util/                # CMake utility library for build integration
│   └── nugget-function.cmake   # Pipeline functions (see "CMake Integration" section)
├── build/                      # Build output directory (generated)
│   └── NuggetPasses.so         # Compiled plugin
└── test/                       # Test suites
    ├── README.md               # Test documentation
    ├── IRBBLabelPass-test/     # IRBBLabelPass tests
    ├── PhaseAnalysisPass-test/ # PhaseAnalysisPass tests
    └── PhaseBoundPass-test/    # PhaseBoundPass tests
```

---

## Advanced Usage Tips

### Chaining Passes in a Single Command

```bash
opt -load-pass-plugin=./build/NuggetPasses.so \
    -passes="ir-bb-label-pass<output_csv=bbs.csv>,phase-analysis-pass<interval_length=10000>" \
    input.ll -o output.bc
```

### Using with Clang Directly

```bash
# Load the plugin via clang's -fpass-plugin flag.
# Note: clang registers the plugin but does not expose a way to select
# individual passes or pass parameters on the command line.  Use opt
# for full control over pass ordering and parameterisation.
clang -O2 -fpass-plugin=./build/NuggetPasses.so program.c -o program
```

### Debugging Pass Behavior

```bash
# View instrumented IR in human-readable form
llvm-dis output.bc -o output.ll
less output.ll  # Search for !bb.id metadata, nugget_* calls

# Enable LLVM debug output
opt -debug-pass-manager -load-pass-plugin=./build/NuggetPasses.so \
    -passes="ir-bb-label-pass" input.ll -o output.bc
```

### Integration with Build Systems

**Make:**
```makefile
%.instrumented.bc: %.bc
	opt -load-pass-plugin=$(NUGGET_PLUGIN) \
	    -passes="ir-bb-label-pass,phase-analysis-pass<interval_length=10000>" \
	    $< -o $@
```

**CMake:** For CMake-based projects, use the [`nugget_util`](nugget_util/) library instead of writing custom commands by hand. See the [`nugget_util` README](nugget_util/README.md) for full documentation and examples.

---

## Requirements and Compatibility

- **LLVM Version**: Requires LLVM 15+, tested with LLVM 18
- **C++ Standard**: C++17 or later
- **Platforms**: Linux, macOS (Windows untested but should work)
- **Build System**: CMake 3.20 or later

### Known Limitations

- **Fortran Support**: Requires `flang-new` (LLVM's Fortran frontend)
- **LTO/ThinLTO**: May require additional configuration for whole-program analysis
- **Debug Info**: Passes preserve debug metadata but don't add new debug annotations

---

## Contributing

Contributions are welcome! Please:

1. Follow existing code style (see `add_copyright_headers.sh` for license headers)
2. Add tests for new functionality
3. Update documentation (this README and inline comments)
4. Ensure all tests pass before submitting

---

## License

This project is licensed under the **BSD-3-Clause License**. See [LICENSE](LICENSE) for details.

Copyright (c) 2026 Zhantong Qiu. All rights reserved.

---

## Citation

If you use these passes in academic work, please cite:

TBD

---

## Support and Contact

- **Issues**: [GitHub Issues](https://github.com/studyztp/Nugget-LLVM-passes/issues)
- **Documentation**: [test/README.md](test/README.md) for detailed test information
- **Examples**: See `test/` directory for complete working examples

---

## Acknowledgments

Built with LLVM's new pass manager infrastructure. Inspired by modern program analysis and architecture simulation needs.
