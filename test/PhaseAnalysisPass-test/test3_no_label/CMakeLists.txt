# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# Test 3: PhaseAnalysisPass run without prior IRBBLabelPass
#
# Purpose: Verify that PhaseAnalysisPass handles IR that has NOT been labeled
# by IRBBLabelPass. Without !bb.id metadata the pass should warn about missing
# metadata and ultimately fail because no basic blocks could be instrumented
# (the assert "There should be at least one basic block instrumented" fires).
#
# This is a negative test -- the pass is expected to fail.

cmake_minimum_required(VERSION 3.20)

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../test1_simple/test1_simple.c)
set(RUNTIME_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../common/nugget_runtime.c)

set(TEST_LL ${OUTPUT_DIR}/test3_no_label.ll)
set(RUNTIME_LL ${OUTPUT_DIR}/nugget_runtime.ll)
set(LINKED_LL ${OUTPUT_DIR}/test3_linked.ll)
set(OPTIMIZED_LL ${OUTPUT_DIR}/test3_optimized.ll)
set(INSTRUMENTED_BC ${OUTPUT_DIR}/test3_instrumented.bc)

# Step 1: Compile sources to IR
add_custom_command(
    OUTPUT ${TEST_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${TEST_SOURCE} -o ${TEST_LL}
    DEPENDS ${TEST_SOURCE}
    COMMENT "Compiling test1_simple.c to LLVM IR (no-label test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_command(
    OUTPUT ${RUNTIME_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${RUNTIME_SOURCE} -o ${RUNTIME_LL}
    DEPENDS ${RUNTIME_SOURCE}
    COMMENT "Compiling nugget_runtime.c to LLVM IR (no-label test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2: Link
add_custom_command(
    OUTPUT ${LINKED_LL}
    COMMAND ${LLVM_LINK_EXECUTABLE} ${TEST_LL} ${RUNTIME_LL} -S -o ${LINKED_LL}
    DEPENDS ${TEST_LL} ${RUNTIME_LL}
    COMMENT "Linking test and runtime IR (no-label test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 3: Optimize (but do NOT run IRBBLabelPass)
add_custom_command(
    OUTPUT ${OPTIMIZED_LL}
    COMMAND ${OPT_EXECUTABLE} -O2 -S ${LINKED_LL} -o ${OPTIMIZED_LL}
    DEPENDS ${LINKED_LL}
    COMMENT "Applying -O2 optimizations (no-label test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

set(_target_prefix "${NUGGET_TARGET_PREFIX}")
set(TEST3_TARGET_NAME "${_target_prefix}test3_no_label_target")
add_custom_target(${TEST3_TARGET_NAME} ALL DEPENDS ${OPTIMIZED_LL})

# Test: Run PhaseAnalysisPass directly on unlabeled IR — should fail.
# We use expect_fail.cmake because the pass aborts via assert(), and CTest
# treats signal-based termination as a hard failure that WILL_FAIL / 
# PASS_REGULAR_EXPRESSION cannot override.
set(_test_prefix "${NUGGET_TEST_PREFIX}")
set(TEST3_NAME "${_test_prefix}test3_no_label_should_fail")
add_test(
    NAME ${TEST3_NAME}
    COMMAND ${CMAKE_COMMAND}
            "-DCMD=${OPT_EXECUTABLE};-load-pass-plugin=${PASS_PLUGIN};-passes=phase-analysis-pass<interval_length=1000>;${OPTIMIZED_LL};-o;${INSTRUMENTED_BC}"
            "-DEXPECTED_ERROR=missing !bb.id metadata"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../../common/expect_fail.cmake
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
