# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# Test 4: PhaseAnalysisPass with missing runtime functions
#
# Purpose: Verify that PhaseAnalysisPass fails gracefully when the required
# runtime functions (nugget_bb_hook, nugget_roi_begin_, nugget_init) are not
# present in the module. This tests the error path in
# instrumentAllIRBasicBlocks() when M.getFunction("nugget_bb_hook") returns
# nullptr.
#
# The test compiles test1_simple.c WITHOUT linking nugget_runtime.c, so the
# runtime hook functions are absent from the module.
#
# This is a negative test -- the pass is expected to fail.

cmake_minimum_required(VERSION 3.20)

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../test1_simple/test1_simple.c)

set(TEST_LL ${OUTPUT_DIR}/test4_missing_runtime.ll)
set(OPTIMIZED_LL ${OUTPUT_DIR}/test4_optimized.ll)
set(LABELED_BC ${OUTPUT_DIR}/test4_labeled.bc)
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)
set(INSTRUMENTED_BC ${OUTPUT_DIR}/test4_instrumented.bc)

# Step 1: Compile source to IR (WITHOUT runtime -- no nugget_bb_hook)
add_custom_command(
    OUTPUT ${TEST_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${TEST_SOURCE} -o ${TEST_LL}
    DEPENDS ${TEST_SOURCE}
    COMMENT "Compiling test1_simple.c to LLVM IR (missing runtime test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2: Optimize
add_custom_command(
    OUTPUT ${OPTIMIZED_LL}
    COMMAND ${OPT_EXECUTABLE} -O2 -S ${TEST_LL} -o ${OPTIMIZED_LL}
    DEPENDS ${TEST_LL}
    COMMENT "Applying -O2 optimizations (missing runtime test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 3: Label basic blocks (should succeed -- IRBBLabelPass doesn't need runtime)
add_custom_command(
    OUTPUT ${LABELED_BC} ${CSV_FILE}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label-pass" ${OPTIMIZED_LL} -o ${LABELED_BC}
    DEPENDS ${OPTIMIZED_LL} ${PASS_PLUGIN}
    COMMENT "Running IRBBLabelPass (missing runtime test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

set(_target_prefix "${NUGGET_TARGET_PREFIX}")
set(TEST4_TARGET_NAME "${_target_prefix}test4_missing_runtime_target")
add_custom_target(${TEST4_TARGET_NAME} ALL DEPENDS ${LABELED_BC} ${CSV_FILE})

# Test: PhaseAnalysisPass should fail because nugget_bb_hook is not in the module.
set(_test_prefix "${NUGGET_TEST_PREFIX}")
set(TEST4_NAME "${_test_prefix}test4_missing_runtime_should_fail")
add_test(
    NAME ${TEST4_NAME}
    COMMAND ${CMAKE_COMMAND}
            "-DCMD=${OPT_EXECUTABLE};-load-pass-plugin=${PASS_PLUGIN};-passes=phase-analysis-pass<interval_length=1000>;${LABELED_BC};-o;${INSTRUMENTED_BC}"
            "-DEXPECTED_ERROR=nugget_bb_hook not found"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../../common/expect_fail.cmake
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
