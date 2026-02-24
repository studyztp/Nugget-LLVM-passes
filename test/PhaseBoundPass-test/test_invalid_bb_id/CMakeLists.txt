# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# Test: PhaseBoundPass with non-existent marker BB IDs
#
# Purpose: Verify that PhaseBoundPass fails gracefully when a specified marker
# bb_id does not exist in the module. The pass should call report_fatal_error
# because instrumentMarkerBBs returns false when not all markers are found.
#
# This is a negative test -- the pass is expected to fail.

cmake_minimum_required(VERSION 3.20)

# Use bb_ids that are guaranteed to be invalid (very large numbers)
set(WARMUP_MARKER_BB_ID 99999)
set(WARMUP_MARKER_COUNT 1)
set(START_MARKER_BB_ID 99998)
set(START_MARKER_COUNT 1)
set(END_MARKER_BB_ID 99997)
set(END_MARKER_COUNT 1)

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../test1_simple/test1_simple.c)
set(RUNTIME_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../common/nugget_runtime.c)

set(TEST_LL ${OUTPUT_DIR}/test_invalid.ll)
set(RUNTIME_LL ${OUTPUT_DIR}/nugget_runtime.ll)
set(LINKED_LL ${OUTPUT_DIR}/test_invalid_linked.ll)
set(OPTIMIZED_LL ${OUTPUT_DIR}/test_invalid_optimized.ll)
set(LABELED_BC ${OUTPUT_DIR}/test_invalid_labeled.bc)
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)
set(INSTRUMENTED_BC ${OUTPUT_DIR}/test_invalid_instrumented.bc)

add_custom_command(
    OUTPUT ${TEST_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${TEST_SOURCE} -o ${TEST_LL}
    DEPENDS ${TEST_SOURCE}
    COMMENT "Compiling test1_simple.c to LLVM IR (invalid bb_id test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${RUNTIME_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${RUNTIME_SOURCE} -o ${RUNTIME_LL}
    DEPENDS ${RUNTIME_SOURCE}
    COMMENT "Compiling nugget_runtime.c to LLVM IR (invalid bb_id test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${LINKED_LL}
    COMMAND ${LLVM_LINK_EXECUTABLE} ${TEST_LL} ${RUNTIME_LL} -S -o ${LINKED_LL}
    DEPENDS ${TEST_LL} ${RUNTIME_LL}
    COMMENT "Linking test and runtime IR (invalid bb_id test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${OPTIMIZED_LL}
    COMMAND ${OPT_EXECUTABLE} -O2 -S ${LINKED_LL} -o ${OPTIMIZED_LL}
    DEPENDS ${LINKED_LL}
    COMMENT "Applying -O2 optimizations (invalid bb_id test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${LABELED_BC} ${CSV_FILE}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label-pass" ${OPTIMIZED_LL} -o ${LABELED_BC}
    DEPENDS ${OPTIMIZED_LL} ${PASS_PLUGIN}
    COMMENT "Running IRBBLabelPass (invalid bb_id test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

add_custom_target(test_invalid_bb_id_target ALL DEPENDS ${LABELED_BC} ${CSV_FILE})

# The pass should fail because the marker bb_ids don't exist.
# We use expect_fail.cmake because the pass calls report_fatal_error which
# sends SIGABRT, and CTest treats signal-based termination as a hard failure.
# The -passes argument is passed via PASS_ARG (quoted) to protect its internal
# semicolons from CMake list splitting.
add_test(
    NAME test_invalid_bb_id_should_fail
    COMMAND ${CMAKE_COMMAND}
            "-DCMD=${OPT_EXECUTABLE};-load-pass-plugin=${PASS_PLUGIN}"
            "-DPASS_ARG=-passes=phase-bound-pass<warmup_marker_bb_id=${WARMUP_MARKER_BB_ID}$<SEMICOLON>warmup_marker_count=${WARMUP_MARKER_COUNT}$<SEMICOLON>start_marker_bb_id=${START_MARKER_BB_ID}$<SEMICOLON>start_marker_count=${START_MARKER_COUNT}$<SEMICOLON>end_marker_bb_id=${END_MARKER_BB_ID}$<SEMICOLON>end_marker_count=${END_MARKER_COUNT}>"
            "-DEXTRA_ARGS=${LABELED_BC};-o;${INSTRUMENTED_BC}"
            "-DEXPECTED_ERROR=Error instrumenting marker basic blocks"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../../common/expect_fail.cmake
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
