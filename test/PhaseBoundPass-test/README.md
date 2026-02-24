# PhaseBoundPass Test Suite

This test suite validates the **PhaseBoundPass** LLVM instrumentation pass.

## Overview

PhaseBoundPass instruments specific basic blocks (markers) to define phase boundaries in program execution:
- **Warmup marker**: Executed before measurement begins
- **Start marker**: Begins the measurement region
- **End marker**: Terminates the measurement region

## Test Structure

```
PhaseBoundPass-test/
├── CMakeLists.txt               # Main test configuration
├── README.md                    # This file
├── common/
│   ├── nugget_runtime.c         # Runtime stub functions
│   └── verify_instrumentation.py # IR verification script
├── test1_simple/
│   ├── CMakeLists.txt           # Test configuration
│   └── test1_simple.c           # Test source code
├── test_label_only/
│   ├── CMakeLists.txt           # label_only=true test
│   └── README.md
├── test_warmup_count_zero/
│   ├── CMakeLists.txt           # warmup_marker_count=0 test
│   └── README.md
└── test_invalid_bb_id/
    └── CMakeLists.txt           # Negative: non-existent marker BB IDs
```

## Tests

### Test 1: Simple Marker Instrumentation (`test1_simple`)

**Purpose**: Verify basic marker instrumentation in IR (default mode, `label_only=false`)

**Pipeline**:
1. Compile source to IR (`clang -O0`)
2. Link with runtime
3. Apply optimizations (`opt -O2`)
4. Label basic blocks (`IRBBLabelPass`)
5. Instrument markers (`PhaseBoundPass`)
6. Verify instrumentation in IR

**Checks**:
- `nugget_init` called in `nugget_roi_begin_` with marker counts
- `nugget_warmup_marker_hook` inserted at warmup marker BB
- `nugget_start_marker_hook` inserted at start marker BB
- `nugget_end_marker_hook` inserted at end marker BB

### Test: Label-Only Mode (`test_label_only`)

**Purpose**: Verify that `label_only=true` inserts inline assembly labels (`nugget_warmup_marker`, `nugget_start_marker`, `nugget_end_marker`) into the binary instead of hook function calls.

**Checks**: Disassembly contains exactly 3 marker labels.

### Test: Warmup Count Zero (`test_warmup_count_zero`)

**Purpose**: Verify that when `warmup_marker_count=0`, the warmup marker is skipped and only 2 marker labels (start and end) appear in the binary.

**Checks**: Disassembly contains exactly 2 marker labels.

### Test: Invalid BB ID (`test_invalid_bb_id`)

**Purpose**: Negative test verifying the pass fails gracefully when specified marker BB IDs do not exist in the module.

**Checks**: `opt` exits with non-zero status (WILL_FAIL test).

## Building and Running

### Prerequisites

- LLVM toolchain (clang, opt, llvm-link, llvm-dis)
- NuggetPasses.so plugin built with IRBBLabelPass and PhaseBoundPass
- Python 3 for verification scripts

### Build

```bash
cd test/PhaseBoundPass-test
cmake -B build -S . \
      -DLLVM_BIN_DIR=/path/to/llvm/bin \
      -DPASS_PLUGIN=/path/to/NuggetPasses.so
cmake --build build
```

### Run Tests

```bash
cd build
ctest --output-on-failure
```

### View Instrumented IR

```bash
# View labeled IR with bb.id metadata
cat build/test1_simple/test1_labeled.ll

# View instrumented IR with marker hooks
cat build/test1_simple/test1_instrumented.ll

# View BB information CSV
cat build/test1_simple/bb_info.csv
```

## Pass Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `warmup_marker_bb_id` | *(required)* | Basic block ID for warmup marker |
| `warmup_marker_count` | *(required)* | Execution count before warmup completes (0 to skip) |
| `start_marker_bb_id` | *(required)* | Basic block ID for ROI start marker |
| `start_marker_count` | *(required)* | Execution count before ROI starts |
| `end_marker_bb_id` | *(required)* | Basic block ID for ROI end marker |
| `end_marker_count` | *(required)* | Execution count before ROI ends |
| `label_only` | `false` | If `true`, emit inline assembly labels instead of hook function calls |

## How Markers Work

1. **IRBBLabelPass** assigns unique IDs to each basic block and outputs CSV
2. User selects specific bb_ids for warmup/start/end markers by inspecting CSV
3. **PhaseBoundPass** instruments those specific basic blocks:
   - When `label_only=false` (default):
     - Inserts `nugget_warmup_marker_hook()` at warmup marker BB
     - Inserts `nugget_start_marker_hook()` at start marker BB
     - Inserts `nugget_end_marker_hook()` at end marker BB
   - When `label_only=true`:
     - Inserts inline assembly labels (`nugget_warmup_marker:`, `nugget_start_marker:`, `nugget_end_marker:`) visible in the disassembly
   - Inserts `nugget_init(warmup_count, start_count, end_count)` in `nugget_roi_begin_`

## Choosing Marker Basic Blocks

To choose appropriate marker bb_ids:

1. Build the test to generate `bb_info.csv`
2. Inspect the CSV to find basic blocks in your target function
3. Update marker bb_id values in `test1_simple/CMakeLists.txt`
4. Rebuild

Example CSV entry:
```csv
FunctionName,FunctionID,BasicBlockName,BasicBlockInstCount,BasicBlockID
main,2,,6,4
main,2,.lr.ph,3,5
main,2,._crit_edge,2,6
```

Choose bb_ids that represent meaningful phase boundaries in your program.

## Expected Output

**Successful test run (example for test1_simple):**
```
Test project /path/to/build
    Start 1: test1_simple_csv_exists
1/2 Test #1: test1_simple_csv_exists ...................   Passed    0.01 sec
    Start 2: test1_simple_instrumentation_validation
2/2 Test #2: test1_simple_instrumentation_validation ...   Passed    0.05 sec

100% tests passed, 0 tests failed out of 2
```

Additional tests from `test_label_only`, `test_warmup_count_zero`, and `test_invalid_bb_id` will also appear in the full CTest output.

## Troubleshooting

**Test fails with "missing required option":**
- Ensure all marker bb_id and count parameters are set in CMakeLists.txt

**Test fails with "marker hook not found":**
- Verify the marker bb_ids exist in the CSV
- Check that the bb_ids correspond to non-nugget functions
- Ensure PhaseBoundPass is correctly loading

**CSV file not found:**
- Verify IRBBLabelPass is included in NuggetPasses.so
- Check PASS_PLUGIN path is correct
