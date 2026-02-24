# PhaseBoundPass label_only Test

This test checks that when running PhaseBoundPass with `label_only=true`, the pass inserts unique inline assembly labels into the output binary instead of hook function calls. The test builds the same source as test1_simple, but verifies that the disassembly contains the expected inline assembly label names: `nugget_warmup_marker`, `nugget_start_marker`, and `nugget_end_marker`.

## How it works
- Compiles the test and runtime to LLVM IR
- Runs IRBBLabelPass and then PhaseBoundPass with `label_only=true`
- Builds the final binary and disassembles it
- Greps the disassembly for the marker labels

## To run
```sh
cd build-x86  # or your build dir
ctest -R test_label_only
```

The test will pass if the three inline assembly labels (`nugget_warmup_marker`, `nugget_start_marker`, `nugget_end_marker`) are present in the disassembly. Note that these are assembly labels (not function names like `nugget_warmup_marker_hook`).
