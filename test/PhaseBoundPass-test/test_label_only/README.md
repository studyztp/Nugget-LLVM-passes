# PhaseBoundPass label_only Test

This test checks that when running PhaseBoundPass with `label_only=true`, the pass inserts unique marker labels (as inline assembly) into the output binary. The test builds the same source as test1_simple, but verifies that the disassembly contains the expected marker labels (e.g., `nugget_start_marker_hook`, `nugget_end_marker_hook`, `nugget_warmup_marker_hook`).

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

The test will pass if the marker labels are present in the disassembly.
