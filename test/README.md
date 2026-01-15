# Nugget LLVM Passes — Test Suite

This folder contains the test suites for the Nugget LLVM passes. It provides self-contained CMake projects to build and validate:

- IRBBLabelPass-test: Labels IR basic blocks with `!bb.id` and emits CSV.
- PhaseAnalysisPass-test: Instruments labeled BBs with `nugget_bb_hook` and checks `nugget_init` in ROI.
- PhaseBoundPass-test: Instruments warmup/start/end markers and checks `nugget_init` in ROI.

Note: Pipeline-test is a separate benchmark-style suite. Ignore it for now.

## Prerequisites
- CMake ≥ 3.20
- Python 3 (for verification scripts)
- LLVM toolchain built/installed; required tools:
  - clang, clang++, opt, llvm-dis
  - llvm-link (PhaseAnalysis/PhaseBound)
  - llc (optional; IRBBLabel test5 + PhaseAnalysis test2)
  - flang-new (optional; IRBBLabel test4)
- Nugget passes plugin (shared library), e.g. `NuggetPasses.so` built from `llvm-nugget-passes/pass`.

## Quick Start (All Suites)
Configure, build, and run all tests from this folder:

```bash
cmake -S llvm-nugget-passes/test -B llvm-nugget-passes/test/build \
  -DLLVM_BIN_DIR=/path/to/llvm/bin \
  -DPASS_PLUGIN=/path/to/NuggetPasses.so

cmake --build llvm-nugget-passes/test/build

(ctest --test-dir llvm-nugget-passes/test/build --output-on-failure)
```

- `LLVM_BIN_DIR`: path containing `clang`, `opt`, `llvm-dis`, etc.
- `PASS_PLUGIN`: path to `NuggetPasses.so`.

### Suite Toggles
You can enable/disable suites when building from this folder:

```bash
cmake -S llvm-nugget-passes/test -B llvm-nugget-passes/test/build \
  -DLLVM_BIN_DIR=/path/to/llvm/bin \
  -DPASS_PLUGIN=/path/to/NuggetPasses.so \
  -DENABLE_IRBBLABEL_TESTS=ON \
  -DENABLE_PHASE_ANALYSIS_TESTS=ON \
  -DENABLE_PHASE_BOUND_TESTS=ON
```

### Targets and Test Names
- When building all suites together from `test/`, target and test names are prefixed by the suite for uniqueness:
  - IRBBLabel: `IRBBLabelPass-test1_simple_target`, `IRBBLabelPass-test1_simple_csv_exists`, ...
  - PhaseAnalysis: `PhaseAnalysisPass-test1_simple_target`, ...
  - PhaseBound: `PhaseBoundPass-test1_simple_target`, ...
- When building inside a specific suite folder (e.g. `IRBBLabelPass-test` directly), original names are used (e.g. `test1_simple_target`).

## Running a Single Suite Standalone
You can build and run a single suite directly:

```bash
# IRBBLabel
cmake -S llvm-nugget-passes/test/IRBBLabelPass-test -B llvm-nugget-passes/test/IRBBLabelPass-test/build \
  -DLLVM_BIN_DIR=/path/to/llvm/bin -DPASS_PLUGIN=/path/to/NuggetPasses.so
cmake --build llvm-nugget-passes/test/IRBBLabelPass-test/build
ctest --test-dir llvm-nugget-passes/test/IRBBLabelPass-test/build --output-on-failure

# PhaseAnalysis
cmake -S llvm-nugget-passes/test/PhaseAnalysisPass-test -B llvm-nugget-passes/test/PhaseAnalysisPass-test/build \
  -DLLVM_BIN_DIR=/path/to/llvm/bin -DPASS_PLUGIN=/path/to/NuggetPasses.so
cmake --build llvm-nugget-passes/test/PhaseAnalysisPass-test/build
ctest --test-dir llvm-nugget-passes/test/PhaseAnalysisPass-test/build --output-on-failure

# PhaseBound
cmake -S llvm-nugget-passes/test/PhaseBoundPass-test -B llvm-nugget-passes/test/PhaseBoundPass-test/build \
  -DLLVM_BIN_DIR=/path/to/llvm/bin -DPASS_PLUGIN=/path/to/NuggetPasses.so
cmake --build llvm-nugget-passes/test/PhaseBoundPass-test/build
ctest --test-dir llvm-nugget-passes/test/PhaseBoundPass-test/build --output-on-failure
```

## Selective Test Runs
Run only some tests:

```bash
# From the aggregated build folder
ctest --test-dir llvm-nugget-passes/test/build -R IRBBLabelPass-test1_simple
ctest --test-dir llvm-nugget-passes/test/build -R PhaseAnalysisPass-test2_machine_match
ctest --test-dir llvm-nugget-passes/test/build -R PhaseBoundPass
```

Build only specific targets (aggregated build):

```bash
cmake --build llvm-nugget-passes/test/build --target IRBBLabelPass-test1_simple_target
cmake --build llvm-nugget-passes/test/build --target PhaseAnalysisPass-test1_simple_target
cmake --build llvm-nugget-passes/test/build --target PhaseBoundPass-test1_simple_target
```

## Suite Details

### IRBBLabelPass-test
- Purpose: Run `ir-bb-label-pass` on various inputs and check metadata/CSV.
- Common pipeline:
  1. `clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm` → IR
  2. `opt -load-pass-plugin=NuggetPasses.so -passes=ir-bb-label-pass` → BC + CSV
  3. `llvm-dis` → human-readable IR
- Tests:
  - `test1_simple`: Basic C; validates CSV presence/content + IR metadata.
  - `test2_dynamic_lib`: Shared libs and edge cases.
  - `test3_cpp_static`: C++ features and mangled names.
  - `test4_mixed`: Mixed C++ + Fortran (requires `flang-new`).
  - `test5_optimization`: Compares pipelines, may use `llc`.
  - `test6_custom_output`: Custom CSV file name.
- Outputs: CSV headers include `FunctionName, FunctionID, BasicBlockName, BasicBlockID, BasicBlockInstCount`.

### PhaseAnalysisPass-test
- Purpose: With labeled IR, run `phase-analysis-pass<interval_length=...>` and verify instrumentation.
- Pipeline:
  1. Compile test + runtime to IR; `llvm-link` → linked IR.
  2. `opt -O2` → optimized IR.
  3. IRBBLabel → labeled BC + CSV; `llvm-dis` → readable IR.
  4. PhaseAnalysis → instrumented BC; `llvm-dis` → readable IR.
- Tests:
  - `test1_simple`: Checks `nugget_init` in `nugget_roi_begin_` and `nugget_bb_hook` calls.
  - `test2_machine_match`: Generates machine code and verifies IR↔machine mapping (requires `llc` and supported arch).
- Arch support for test2: `x86_64` and `AArch64`.

### PhaseBoundPass-test
- Purpose: Insert warmup/start/end hooks at specified BBs and validate them.
- Pipeline mirrors PhaseAnalysis up to labeling; then runs `phase-bound-pass<...>` to place marker hooks.
- Tests:
  - `test1_simple`: Checks `nugget_init` args (marker counts) and presence of all marker hooks.

## Troubleshooting
- "add_custom_target cannot create target ... already exists":
  - When building all suites from `test/`, prefixed target names are used automatically. If you still see collisions, clean the build directory and reconfigure.
- Failing IR metadata tests complaining about `nugget_init_`:
  - The verifier is updated for `nugget_init` (without underscore). Ensure your pass and runtime use the `nugget_init` symbol.
- Missing tools:
  - Ensure `LLVM_BIN_DIR` points to the correct toolchain (`clang`, `opt`, `llvm-dis`, etc.).
- `opt` skipping functions at `-O0`:
  - We compile IR with `-Xclang -disable-O0-optnone` to allow running passes at `-O0`.
- Fortran tests:
  - `test4_mixed` requires `flang-new`; it’s optional and will be skipped if unavailable.
- Architecture-dependent tests:
  - `test2_machine_match` runs only on `x86_64` or `AArch64` when `llc` is found.

## Notes
- Pipeline-test is excluded from the aggregated build by design.
- Keep `PASS_PLUGIN` in sync with the LLVM version used in `LLVM_BIN_DIR` to avoid ABI mismatches.
