# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# Test 1: Simple PhaseBoundPass instrumentation test
#
# This test validates that PhaseBoundPass correctly:
#   1. Inserts nugget_init call in nugget_roi_begin_ with marker counts
#   2. Inserts warmup/start/end marker hooks at specified basic blocks
#
# Test pipeline:
#   1. Source -> clang -O0 -> IR
#   2. IR -> opt -O2 -> Optimized IR
#   3. Optimized IR -> IRBBLabelPass -> Labeled IR + CSV
#   4. Labeled IR -> PhaseBoundPass -> Instrumented IR
#   5. Verify instrumentation in IR

cmake_minimum_required(VERSION 3.20)

# Configuration - marker basic block IDs and counts
# Based on bb_info.csv: bb_ids 0-4 exist after optimization
# bb_id 0: compute entry, bb_id 1: compute loop, bb_id 2: compute exit
# bb_id 3: helper entry, bb_id 4: main entry
set(WARMUP_MARKER_BB_ID 0)
set(WARMUP_MARKER_COUNT 1)
set(START_MARKER_BB_ID 2)
set(START_MARKER_COUNT 1)
set(END_MARKER_BB_ID 3)
set(END_MARKER_COUNT 1)

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

# Source files
set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/test1_simple.c)
set(RUNTIME_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../common/nugget_runtime.c)

# Intermediate files - IR pipeline
set(TEST_LL ${OUTPUT_DIR}/test1_simple.ll)
set(RUNTIME_LL ${OUTPUT_DIR}/nugget_runtime.ll)
set(LINKED_LL ${OUTPUT_DIR}/test1_linked.ll)
set(OPTIMIZED_LL ${OUTPUT_DIR}/test1_optimized.ll)
set(LABELED_BC ${OUTPUT_DIR}/test1_labeled.bc)
set(LABELED_LL ${OUTPUT_DIR}/test1_labeled.ll)
set(INSTRUMENTED_BC ${OUTPUT_DIR}/test1_instrumented.bc)
set(INSTRUMENTED_LL ${OUTPUT_DIR}/test1_instrumented.ll)
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)

# ============================================================================
# Step 1: Compile test source to LLVM IR
# ============================================================================
add_custom_command(
    OUTPUT ${TEST_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${TEST_SOURCE} -o ${TEST_LL}
    DEPENDS ${TEST_SOURCE}
    COMMENT "Compiling test1_simple.c to LLVM IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Step 2: Compile runtime to LLVM IR
# ============================================================================
add_custom_command(
    OUTPUT ${RUNTIME_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${RUNTIME_SOURCE} -o ${RUNTIME_LL}
    DEPENDS ${RUNTIME_SOURCE}
    COMMENT "Compiling nugget_runtime.c to LLVM IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Step 3: Link test and runtime IR
# ============================================================================
add_custom_command(
    OUTPUT ${LINKED_LL}
    COMMAND ${LLVM_LINK_EXECUTABLE} ${TEST_LL} ${RUNTIME_LL} -S -o ${LINKED_LL}
    DEPENDS ${TEST_LL} ${RUNTIME_LL}
    COMMENT "Linking test and runtime IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Step 4: Apply -O2 optimizations
# ============================================================================
add_custom_command(
    OUTPUT ${OPTIMIZED_LL}
    COMMAND ${OPT_EXECUTABLE} -O2 -S ${LINKED_LL} -o ${OPTIMIZED_LL}
    DEPENDS ${LINKED_LL}
    COMMENT "Applying -O2 optimizations"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Step 5: Run IRBBLabelPass to label all basic blocks
# ============================================================================
add_custom_command(
    OUTPUT ${LABELED_BC} ${CSV_FILE}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label-pass" ${OPTIMIZED_LL} -o ${LABELED_BC}
    DEPENDS ${OPTIMIZED_LL} ${PASS_PLUGIN}
    COMMENT "Running IRBBLabelPass to label basic blocks"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Step 5b: Convert labeled bitcode to readable IR (for debugging)
# ============================================================================
add_custom_command(
    OUTPUT ${LABELED_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${LABELED_BC} -o ${LABELED_LL}
    DEPENDS ${LABELED_BC}
    COMMENT "Converting labeled bitcode to readable IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Step 6: Run PhaseBoundPass to instrument marker basic blocks
# ============================================================================
add_custom_command(
    OUTPUT ${INSTRUMENTED_BC}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            "-passes=phase-bound-pass<warmup_marker_bb_id=${WARMUP_MARKER_BB_ID}$<SEMICOLON>warmup_marker_count=${WARMUP_MARKER_COUNT}$<SEMICOLON>start_marker_bb_id=${START_MARKER_BB_ID}$<SEMICOLON>start_marker_count=${START_MARKER_COUNT}$<SEMICOLON>end_marker_bb_id=${END_MARKER_BB_ID}$<SEMICOLON>end_marker_count=${END_MARKER_COUNT}>"
            ${LABELED_BC} -o ${INSTRUMENTED_BC}
    DEPENDS ${LABELED_BC} ${PASS_PLUGIN}
    COMMENT "Running PhaseBoundPass to instrument marker basic blocks"
    WORKING_DIRECTORY ${OUTPUT_DIR}
    VERBATIM
)

# ============================================================================
# Step 7: Convert instrumented bitcode to readable IR
# ============================================================================
add_custom_command(
    OUTPUT ${INSTRUMENTED_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${INSTRUMENTED_BC} -o ${INSTRUMENTED_LL}
    DEPENDS ${INSTRUMENTED_BC}
    COMMENT "Converting instrumented bitcode to readable IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Target: Build all test1 artifacts
# ============================================================================
set(_target_prefix "${NUGGET_TARGET_PREFIX}")
set(TEST1_TARGET_NAME "${_target_prefix}test1_simple_target")
add_custom_target(${TEST1_TARGET_NAME} ALL 
    DEPENDS ${INSTRUMENTED_LL} ${LABELED_LL} ${CSV_FILE}
)

# ============================================================================
# Test 1.1: Verify CSV file exists and has correct format
# ============================================================================
set(_test_prefix "${NUGGET_TEST_PREFIX}")
set(TEST1_CSV_EXISTS_NAME "${_test_prefix}test1_simple_csv_exists")
add_test(
    NAME ${TEST1_CSV_EXISTS_NAME}
    COMMAND ${CMAKE_COMMAND} -E cat ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(${TEST1_CSV_EXISTS_NAME} PROPERTIES
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

# ============================================================================
# Test 1.2: Verify instrumentation
# ============================================================================
set(TEST1_INSTRUMENT_NAME "${_test_prefix}test1_simple_instrumentation_validation")
add_test(
    NAME ${TEST1_INSTRUMENT_NAME}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_instrumentation.py
            ${INSTRUMENTED_LL} ${CSV_FILE}
            ${WARMUP_MARKER_BB_ID} ${WARMUP_MARKER_COUNT}
            ${START_MARKER_BB_ID} ${START_MARKER_COUNT}
            ${END_MARKER_BB_ID} ${END_MARKER_COUNT}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(${TEST1_INSTRUMENT_NAME} PROPERTIES
    DEPENDS ${TEST1_CSV_EXISTS_NAME}
)
