# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# Test 5: Pass chaining in a single opt invocation
#
# Purpose: Verify that IRBBLabelPass and PhaseAnalysisPass can be chained in a
# single opt command using the -passes="pass1,pass2" syntax. The README
# documents this usage pattern, so it must be validated.
#
# Pipeline:
#   1. Compile source + runtime to IR
#   2. Link and optimize with -O2
#   3. Run BOTH passes in a single opt invocation:
#      opt -passes="ir-bb-label-pass,phase-analysis-pass<interval_length=1000>"
#   4. Verify instrumentation is correct
#
# This is a positive test -- the chained invocation should succeed and produce
# the same result as two separate opt invocations.

cmake_minimum_required(VERSION 3.20)

set(PHASE_THRESHOLD 1000)

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../test1_simple/test1_simple.c)
set(RUNTIME_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../common/nugget_runtime.c)

set(TEST_LL ${OUTPUT_DIR}/test5_chain.ll)
set(RUNTIME_LL ${OUTPUT_DIR}/nugget_runtime.ll)
set(LINKED_LL ${OUTPUT_DIR}/test5_linked.ll)
set(OPTIMIZED_LL ${OUTPUT_DIR}/test5_optimized.ll)
set(INSTRUMENTED_BC ${OUTPUT_DIR}/test5_instrumented.bc)
set(INSTRUMENTED_LL ${OUTPUT_DIR}/test5_instrumented.ll)
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)

# Step 1: Compile test source to IR
add_custom_command(
    OUTPUT ${TEST_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${TEST_SOURCE} -o ${TEST_LL}
    DEPENDS ${TEST_SOURCE}
    COMMENT "Compiling test1_simple.c to LLVM IR (pass chaining test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2: Compile runtime to IR
add_custom_command(
    OUTPUT ${RUNTIME_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${RUNTIME_SOURCE} -o ${RUNTIME_LL}
    DEPENDS ${RUNTIME_SOURCE}
    COMMENT "Compiling nugget_runtime.c to LLVM IR (pass chaining test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 3: Link test and runtime IR
add_custom_command(
    OUTPUT ${LINKED_LL}
    COMMAND ${LLVM_LINK_EXECUTABLE} ${TEST_LL} ${RUNTIME_LL} -S -o ${LINKED_LL}
    DEPENDS ${TEST_LL} ${RUNTIME_LL}
    COMMENT "Linking test and runtime IR (pass chaining test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 4: Optimize
add_custom_command(
    OUTPUT ${OPTIMIZED_LL}
    COMMAND ${OPT_EXECUTABLE} -O2 -S ${LINKED_LL} -o ${OPTIMIZED_LL}
    DEPENDS ${LINKED_LL}
    COMMENT "Applying -O2 optimizations (pass chaining test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 5: Run BOTH passes in a SINGLE opt invocation (chained with comma)
# VERBATIM is required so CMake properly quotes the -passes= argument for
# the shell (angle brackets would otherwise be treated as redirection).
add_custom_command(
    OUTPUT ${INSTRUMENTED_BC} ${CSV_FILE}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            "-passes=ir-bb-label-pass,phase-analysis-pass<interval_length=${PHASE_THRESHOLD}>"
            ${OPTIMIZED_LL} -o ${INSTRUMENTED_BC}
    DEPENDS ${OPTIMIZED_LL} ${PASS_PLUGIN}
    COMMENT "Running chained IRBBLabelPass + PhaseAnalysisPass in single opt invocation"
    WORKING_DIRECTORY ${OUTPUT_DIR}
    VERBATIM
)

# Step 6: Convert to readable IR for verification
add_custom_command(
    OUTPUT ${INSTRUMENTED_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${INSTRUMENTED_BC} -o ${INSTRUMENTED_LL}
    DEPENDS ${INSTRUMENTED_BC}
    COMMENT "Converting instrumented bitcode to readable IR (pass chaining test)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

set(_target_prefix "${NUGGET_TARGET_PREFIX}")
set(TEST5_TARGET_NAME "${_target_prefix}test5_pass_chaining_target")
add_custom_target(${TEST5_TARGET_NAME} ALL
    DEPENDS ${INSTRUMENTED_LL} ${CSV_FILE}
)

# Test 5.1: Verify CSV file exists
set(_test_prefix "${NUGGET_TEST_PREFIX}")
set(TEST5_CSV_NAME "${_test_prefix}test5_pass_chaining_csv_exists")
add_test(
    NAME ${TEST5_CSV_NAME}
    COMMAND ${CMAKE_COMMAND} -E cat ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(${TEST5_CSV_NAME} PROPERTIES
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

# Test 5.2: Verify instrumentation is correct (same script as test1)
set(TEST5_INSTRUMENT_NAME "${_test_prefix}test5_pass_chaining_instrumentation_validation")
add_test(
    NAME ${TEST5_INSTRUMENT_NAME}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_instrumentation.py
            ${INSTRUMENTED_LL} ${CSV_FILE} ${PHASE_THRESHOLD}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(${TEST5_INSTRUMENT_NAME} PROPERTIES
    DEPENDS ${TEST5_CSV_NAME}
)
