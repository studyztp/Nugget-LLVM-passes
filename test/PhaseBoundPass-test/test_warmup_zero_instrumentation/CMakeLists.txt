# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# Test: PhaseBoundPass instrumentation mode with warmup_marker_count=0
#
# Purpose: Verify that when warmup_marker_count=0 in instrumentation mode
# (label_only=false), the pass:
#   1. Does NOT look up or require nugget_warmup_marker_hook
#   2. Only instruments start and end markers
#   3. Passes (0, start_count, end_count) to nugget_init
#
# This tests the no_warmup_marker code path in instrumentMarkerBBs() which
# skips the warmup hook function lookup when warmup_marker_count is 0.

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

set(TEST_LL ${OUTPUT_DIR}/test_warmup_zero_instr.ll)
set(RUNTIME_LL ${OUTPUT_DIR}/nugget_runtime.ll)
set(LINKED_LL ${OUTPUT_DIR}/test_warmup_zero_instr_linked.ll)
set(OPTIMIZED_LL ${OUTPUT_DIR}/test_warmup_zero_instr_optimized.ll)
set(LABELED_BC ${OUTPUT_DIR}/test_warmup_zero_instr_labeled.bc)
set(LABELED_LL ${OUTPUT_DIR}/test_warmup_zero_instr_labeled.ll)
set(INSTRUMENTED_BC ${OUTPUT_DIR}/test_warmup_zero_instr_instrumented.bc)
set(INSTRUMENTED_LL ${OUTPUT_DIR}/test_warmup_zero_instr_instrumented.ll)
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)

add_custom_command(
    OUTPUT ${TEST_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${TEST_SOURCE} -o ${TEST_LL}
    DEPENDS ${TEST_SOURCE}
    COMMENT "Compiling test1_simple.c to LLVM IR (zero warmup instrumentation)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${RUNTIME_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${RUNTIME_SOURCE} -o ${RUNTIME_LL}
    DEPENDS ${RUNTIME_SOURCE}
    COMMENT "Compiling nugget_runtime.c to LLVM IR (zero warmup instrumentation)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${LINKED_LL}
    COMMAND ${LLVM_LINK_EXECUTABLE} ${TEST_LL} ${RUNTIME_LL} -S -o ${LINKED_LL}
    DEPENDS ${TEST_LL} ${RUNTIME_LL}
    COMMENT "Linking test and runtime IR (zero warmup instrumentation)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${OPTIMIZED_LL}
    COMMAND ${OPT_EXECUTABLE} -O2 -S ${LINKED_LL} -o ${OPTIMIZED_LL}
    DEPENDS ${LINKED_LL}
    COMMENT "Applying -O2 optimizations (zero warmup instrumentation)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${LABELED_BC} ${CSV_FILE}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label-pass" ${OPTIMIZED_LL} -o ${LABELED_BC}
    DEPENDS ${OPTIMIZED_LL} ${PASS_PLUGIN}
    COMMENT "Running IRBBLabelPass (zero warmup instrumentation)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${LABELED_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${LABELED_BC} -o ${LABELED_LL}
    DEPENDS ${LABELED_BC}
    COMMENT "Converting labeled bitcode to readable IR (zero warmup instrumentation)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Run PhaseBoundPass in instrumentation mode (label_only=false is default)
# with warmup_marker_count=0
add_custom_command(
    OUTPUT ${INSTRUMENTED_BC}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            "-passes=phase-bound-pass<warmup_marker_bb_id=${WARMUP_MARKER_BB_ID}$<SEMICOLON>warmup_marker_count=0$<SEMICOLON>start_marker_bb_id=${START_MARKER_BB_ID}$<SEMICOLON>start_marker_count=${START_MARKER_COUNT}$<SEMICOLON>end_marker_bb_id=${END_MARKER_BB_ID}$<SEMICOLON>end_marker_count=${END_MARKER_COUNT}>"
            ${LABELED_BC} -o ${INSTRUMENTED_BC}
    DEPENDS ${LABELED_BC} ${PASS_PLUGIN}
    COMMENT "Running PhaseBoundPass in instrumentation mode (zero warmup)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
    VERBATIM
)
add_custom_command(
    OUTPUT ${INSTRUMENTED_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${INSTRUMENTED_BC} -o ${INSTRUMENTED_LL}
    DEPENDS ${INSTRUMENTED_BC}
    COMMENT "Converting instrumented bitcode to readable IR (zero warmup instrumentation)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

set(_target_prefix "${NUGGET_TARGET_PREFIX}")
set(TARGET_NAME "${_target_prefix}test_warmup_zero_instr_target")
add_custom_target(${TARGET_NAME} ALL
    DEPENDS ${INSTRUMENTED_LL} ${LABELED_LL} ${CSV_FILE}
)

# Verify CSV exists
set(_test_prefix "${NUGGET_TEST_PREFIX}")
set(CSV_TEST_NAME "${_test_prefix}test_warmup_zero_instr_csv_exists")
add_test(
    NAME ${CSV_TEST_NAME}
    COMMAND ${CMAKE_COMMAND} -E cat ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(${CSV_TEST_NAME} PROPERTIES
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

# Verify instrumentation: warmup hook absent, start/end hooks present
set(INSTRUMENT_TEST_NAME "${_test_prefix}test_warmup_zero_instr_validation")
add_test(
    NAME ${INSTRUMENT_TEST_NAME}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_instrumentation.py
            ${INSTRUMENTED_LL} ${CSV_FILE}
            ${WARMUP_MARKER_BB_ID} ${WARMUP_MARKER_COUNT}
            ${START_MARKER_BB_ID} ${START_MARKER_COUNT}
            ${END_MARKER_BB_ID} ${END_MARKER_COUNT}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(${INSTRUMENT_TEST_NAME} PROPERTIES
    DEPENDS ${CSV_TEST_NAME}
)
