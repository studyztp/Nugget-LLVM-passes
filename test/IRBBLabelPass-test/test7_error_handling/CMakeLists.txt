# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# Test 7: Error handling and malformed parameter rejection
#
# This test validates that the pass correctly rejects malformed parameters
# and unknown options via the ParseOptions / MatchParamPass logic.
#
# Tests registered:
#   1. test7_unknown_option: pass should reject unknown parameter keys
#   2. test7_malformed_param: pass should reject malformed key=value syntax

cmake_minimum_required(VERSION 3.20)

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

# Reuse test1_simple source — the program doesn't matter, only param parsing
set(SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../test1_simple/test1_simple.c)
set(LL_FILE ${OUTPUT_DIR}/test7_input.ll)

# Step 1: Compile source to LLVM IR (shared by both error-handling tests)
set(_target_prefix "${NUGGET_TARGET_PREFIX}")
set(TEST7_TARGET_NAME "${_target_prefix}test7_error_handling_target")
add_custom_command(
    OUTPUT ${LL_FILE}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${SOURCE_FILE} -o ${LL_FILE}
    DEPENDS ${SOURCE_FILE}
    COMMENT "Compiling input for test7 error handling"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
add_custom_target(${TEST7_TARGET_NAME} ALL DEPENDS ${LL_FILE})

# ============================================================================
# Test 7.1: Unknown option key should be rejected
# ============================================================================
# Passing an option key that is not in IRBBLabelPassOptions should cause an
# error message from the pass.
set(_test_prefix "${NUGGET_TEST_PREFIX}")
set(TEST7_UNKNOWN_NAME "${_test_prefix}test7_unknown_option")
add_test(
    NAME ${TEST7_UNKNOWN_NAME}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            "-passes=ir-bb-label-pass<bogus_key=value>"
            ${LL_FILE} -o /dev/null
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
# opt should exit with non-zero when the pass name matches but param parsing
# fails (unknown option triggers an error).
set_tests_properties(${TEST7_UNKNOWN_NAME} PROPERTIES
    WILL_FAIL TRUE
)

# ============================================================================
# Test 7.2: Missing required option for phase-analysis-pass
# ============================================================================
# phase-analysis-pass requires interval_length. Omitting it should fail.
set(TEST7_MISSING_NAME "${_test_prefix}test7_missing_required_option")
add_test(
    NAME ${TEST7_MISSING_NAME}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            "-passes=phase-analysis-pass"
            ${LL_FILE} -o /dev/null
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(${TEST7_MISSING_NAME} PROPERTIES
    WILL_FAIL TRUE
)
