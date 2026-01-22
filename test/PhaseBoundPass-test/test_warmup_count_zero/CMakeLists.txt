# SPDX-License-Identifier: BSD-3-Clause
# Test: PhaseBoundPass label_only mode with warmup_marker_count=0
# This test checks that label_only=true and warmup_marker_count=0 results in only 2 marker labels in the disassembly

cmake_minimum_required(VERSION 3.20)

set(WARMUP_MARKER_BB_ID 0)
set(WARMUP_MARKER_COUNT 0)
set(START_MARKER_BB_ID 2)
set(START_MARKER_COUNT 1)
set(END_MARKER_BB_ID 3)
set(END_MARKER_COUNT 1)

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../test1_simple/test1_simple.c)
set(RUNTIME_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../common/nugget_runtime.c)

set(TEST_LL ${OUTPUT_DIR}/test_warmup_count_zero.ll)
set(RUNTIME_LL ${OUTPUT_DIR}/nugget_runtime.ll)
set(LINKED_LL ${OUTPUT_DIR}/test_warmup_count_zero_linked.ll)
set(OPTIMIZED_LL ${OUTPUT_DIR}/test_warmup_count_zero_optimized.ll)
set(LABELED_BC ${OUTPUT_DIR}/test_warmup_count_zero_labeled.bc)
set(LABELED_LL ${OUTPUT_DIR}/test_warmup_count_zero_labeled.ll)
set(INSTRUMENTED_BC ${OUTPUT_DIR}/test_warmup_count_zero_instrumented.bc)
set(INSTRUMENTED_LL ${OUTPUT_DIR}/test_warmup_count_zero_instrumented.ll)
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)
set(DISASM_FILE ${OUTPUT_DIR}/test_warmup_count_zero_disasm.txt)

add_custom_command(
    OUTPUT ${TEST_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${TEST_SOURCE} -o ${TEST_LL}
    DEPENDS ${TEST_SOURCE}
    COMMENT "Compiling test1_simple.c to LLVM IR (zero warmup)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${RUNTIME_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${RUNTIME_SOURCE} -o ${RUNTIME_LL}
    DEPENDS ${RUNTIME_SOURCE}
    COMMENT "Compiling nugget_runtime.c to LLVM IR (zero warmup)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${LINKED_LL}
    COMMAND ${LLVM_LINK_EXECUTABLE} ${TEST_LL} ${RUNTIME_LL} -S -o ${LINKED_LL}
    DEPENDS ${TEST_LL} ${RUNTIME_LL}
    COMMENT "Linking test and runtime IR (zero warmup)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${OPTIMIZED_LL}
    COMMAND ${OPT_EXECUTABLE} -O2 -S ${LINKED_LL} -o ${OPTIMIZED_LL}
    DEPENDS ${LINKED_LL}
    COMMENT "Applying -O2 optimizations (zero warmup)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${LABELED_BC} ${CSV_FILE}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label-pass" ${OPTIMIZED_LL} -o ${LABELED_BC}
    DEPENDS ${OPTIMIZED_LL} ${PASS_PLUGIN}
    COMMENT "Running IRBBLabelPass to label basic blocks (zero warmup)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${LABELED_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${LABELED_BC} -o ${LABELED_LL}
    DEPENDS ${LABELED_BC}
    COMMENT "Converting labeled bitcode to readable IR (zero warmup)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${INSTRUMENTED_BC}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            "-passes=phase-bound-pass<warmup_marker_bb_id=${WARMUP_MARKER_BB_ID}$<SEMICOLON>warmup_marker_count=0$<SEMICOLON>start_marker_bb_id=${START_MARKER_BB_ID}$<SEMICOLON>start_marker_count=${START_MARKER_COUNT}$<SEMICOLON>end_marker_bb_id=${END_MARKER_BB_ID}$<SEMICOLON>end_marker_count=${END_MARKER_COUNT}$<SEMICOLON>label_only=true>"
            ${LABELED_BC} -o ${INSTRUMENTED_BC}
    DEPENDS ${LABELED_BC} ${PASS_PLUGIN}
    COMMENT "Running PhaseBoundPass in label_only mode (zero warmup)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
    VERBATIM
)
add_custom_command(
    OUTPUT ${INSTRUMENTED_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${INSTRUMENTED_BC} -o ${INSTRUMENTED_LL}
    DEPENDS ${INSTRUMENTED_BC}
    COMMENT "Converting instrumented bitcode to readable IR (zero warmup)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${DISASM_FILE}
    COMMAND ${CLANG_EXECUTABLE} ${INSTRUMENTED_LL} -o ${OUTPUT_DIR}/test_warmup_count_zero_bin
    COMMAND ${LLVM_OBJDUMP_EXECUTABLE} -d ${OUTPUT_DIR}/test_warmup_count_zero_bin > ${DISASM_FILE}
    DEPENDS ${INSTRUMENTED_LL}
    COMMENT "Disassembling final binary to check for marker labels (zero warmup)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_target(test_warmup_count_zero_target ALL
    DEPENDS ${INSTRUMENTED_LL} ${LABELED_LL} ${CSV_FILE} ${DISASM_FILE}
)
add_test(
    NAME test_warmup_count_zero_disasm_labels
    COMMAND bash -c "grep -E '<nugget_(start|end|warmup)_marker>' ${DISASM_FILE} | tee /dev/stderr | wc -l | grep -q '^2$'"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
