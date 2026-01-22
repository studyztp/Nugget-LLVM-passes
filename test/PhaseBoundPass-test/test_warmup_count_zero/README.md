# PhaseBoundPass label_only Test (warmup_marker_count=0)

This test checks that when running PhaseBoundPass with `label_only=true` and `warmup_marker_count=0`, only 2 marker labels (start and end) are present in the disassembly.

## How it works
- Compiles the test and runtime to LLVM IR
- Runs IRBBLabelPass and then PhaseBoundPass with `label_only=true` and `warmup_marker_count=0`
- Builds the final binary and disassembles it
- Greps the disassembly for the marker labels
- The test passes if there are exactly 2 such labels

## To run
```sh
cd build-x86  # or your build dir
ctest -R test_warmup_count_zero
```
