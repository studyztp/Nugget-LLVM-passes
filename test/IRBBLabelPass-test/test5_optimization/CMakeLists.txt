# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Test 5: Optimization Pipeline Comparison
#
# This test compares two compilation pipelines to verify the IRBBLabel pass
# works correctly with optimized code and measures instrumentation impact:
#
# Pipeline 1 (Direct):
#   source.c -> clang -O2 -> binary (baseline)
#   Captures: Pass list from clang -O2
#
# Pipeline 2 (Labeled):
#   source.c -> clang -O0 -> unopt.ll
#           -> opt -O2 -> opt.ll
#           -> opt + IRBBLabel pass -> labeled.bc
#           -> llc -O2 -> object file
#           -> clang (link) -> binary
#   Captures: Pass lists from opt -O2 and llc -O2
#
# Why this test matters:
#   - Optimizations (-O2) drastically change BB structure
#   - Loop unrolling, inlining, vectorization create complex BBs
#   - Optimized BB names include .lr.ph, ._crit_edge, .unr-lcssa, etc.
#   - Pass must handle all optimization artifacts
#   - Real-world usage will instrument optimized production code
#   - Measures performance overhead of instrumentation
#
# Test components:
#   1. Functional correctness: Both binaries produce same output
#   2. Performance comparison: Overhead < 5%
#   3. Pass tracking: Captures all optimization passes applied
#   4. Metadata validation: IR metadata matches CSV after -O2
#
# Scripts used:
#   - capture_passes.sh: Records passes from compiler/opt/llc
#   - compare_passes.sh: Generates pass comparison report
#   - compare_performance.sh: Runs binaries and compares timing
#
# Why llc -O2:
#   - llc is the LLVM code generator (IR -> native code)
#   - -O2 applies backend optimizations (instruction selection, register allocation)
#   - Some basic blocks are created/modified during code generation
#
# Tests registered (7 total):
#   1. test5_optimization_csv_exists (CSV file exists)
#   2. test5_optimization_binaries_exist (both binaries built)
#   3. test5_direct_binary_runs (direct binary executes)
#   4. test5_optimized_binary_runs (optimized binary executes)
#   5. test5_performance_comparison (performance overhead check)
#   6. test5_passes_comparison (pass analysis report with validation)
#   7. test5_metadata_validation (IR metadata vs CSV)

cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Test 5: Optimization Pipeline Comparison
# ============================================================================

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/test5_optimization.c)

# Pipeline 1: Direct compilation with -O2
set(DIRECT_BIN ${OUTPUT_DIR}/test5_direct)
set(DIRECT_PASSES_FILE ${OUTPUT_DIR}/direct_passes.txt)
add_custom_command(
    OUTPUT ${DIRECT_BIN}
    COMMAND ${CLANG_EXECUTABLE} -O2 
            ${SOURCE_FILE} 
            -o ${DIRECT_BIN} -lm
    DEPENDS ${SOURCE_FILE}
    COMMENT "Building direct compilation with -O2"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Capture direct clang -O2 passes applied
add_custom_command(
    OUTPUT ${DIRECT_PASSES_FILE}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/capture_passes.sh ${CLANG_EXECUTABLE} "-O2 -lm" ${SOURCE_FILE} ${DIRECT_PASSES_FILE}
    DEPENDS ${SOURCE_FILE}
    COMMENT "Capturing clang -O2 passes information"
)

# Pipeline 2: Optimization pipeline with pass
set(OPT_LL ${OUTPUT_DIR}/test5_unoptimized.ll)
set(OPT_O2_LL ${OUTPUT_DIR}/test5_optimized.ll)
set(LABELED_BC ${OUTPUT_DIR}/test5_labeled.bc)
set(LABELED_OBJ ${OUTPUT_DIR}/test5_labeled.o)
set(OPTIMIZED_BIN ${OUTPUT_DIR}/test5_optimized)
set(OPTIMIZED_CSV ${OUTPUT_DIR}/bb_info.csv)

# Step 1: Compile to unoptimized LLVM IR
add_custom_command(
    OUTPUT ${OPT_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm 
            ${SOURCE_FILE} 
            -o ${OPT_LL}
    DEPENDS ${SOURCE_FILE}
    COMMENT "Compiling test5 to unoptimized LLVM IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2: Apply -O2 optimizations
add_custom_command(
    OUTPUT ${OPT_O2_LL}
    COMMAND ${OPT_EXECUTABLE} -O2 ${OPT_LL} -S -o ${OPT_O2_LL}
    DEPENDS ${OPT_LL}
    COMMENT "Applying -O2 optimizations to test5"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2b: Capture opt -O2 passes applied (separate step)
set(OPT_PASSES_FILE ${OUTPUT_DIR}/opt_passes.txt)
add_custom_command(
    OUTPUT ${OPT_PASSES_FILE}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/capture_passes.sh ${OPT_EXECUTABLE} "-O2" ${OPT_LL} ${OPT_PASSES_FILE}
    DEPENDS ${OPT_LL}
    COMMENT "Capturing opt -O2 passes information"
)

# Step 3: Run IRBBLabel pass
add_custom_command(
    OUTPUT ${LABELED_BC}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label-pass" ${OPT_O2_LL} -o ${LABELED_BC}
    DEPENDS ${OPT_O2_LL} ${PASS_PLUGIN} ${OPT_PASSES_FILE}
    COMMENT "Labeling basic blocks with ir-bb-label-pass"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 3b: Convert labeled bitcode to readable IR
set(LABELED_LL ${OUTPUT_DIR}/test5_labeled_instrumented.ll)
add_custom_command(
    OUTPUT ${LABELED_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${LABELED_BC} -o ${LABELED_LL}
    DEPENDS ${LABELED_BC}
    COMMENT "Converting labeled bitcode to readable IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 4: Compile bitcode to object with llc and -O2
add_custom_command(
    OUTPUT ${LABELED_OBJ}
    COMMAND ${LLC_EXECUTABLE} -O2 -filetype=obj -relocation-model=pic ${LABELED_BC} -o ${LABELED_OBJ}
    DEPENDS ${LABELED_BC}
    COMMENT "Compiling labeled bitcode to object file"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 4b: Capture llc -O2 passes applied (separate step)
set(LLC_PASSES_FILE ${OUTPUT_DIR}/llc_passes.txt)
add_custom_command(
    OUTPUT ${LLC_PASSES_FILE}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/capture_passes.sh ${LLC_EXECUTABLE} "-O2 -filetype=obj -relocation-model=pic" ${LABELED_BC} ${LLC_PASSES_FILE}
    DEPENDS ${LABELED_BC}
    COMMENT "Capturing llc -O2 passes information"
)

# Step 4c: Capture machine-level passes from direct compilation
# Note: We compile the source to optimized IR first, then run llc to get machine passes
set(DIRECT_OPTIMIZED_LL ${OUTPUT_DIR}/test5_optimized_direct.ll)
set(DIRECT_MACHINE_PASSES_FILE ${OUTPUT_DIR}/direct_machine_passes.txt)

# First compile to optimized IR
add_custom_command(
    OUTPUT ${DIRECT_OPTIMIZED_LL}
    COMMAND ${CLANG_EXECUTABLE} -O2 -Xclang -disable-O0-optnone -S -emit-llvm ${SOURCE_FILE} -o ${DIRECT_OPTIMIZED_LL}
    DEPENDS ${SOURCE_FILE}
    COMMENT "Generating optimized IR from direct clang -O2 compilation"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Then capture machine passes from llc on the optimized IR
add_custom_command(
    OUTPUT ${DIRECT_MACHINE_PASSES_FILE}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/capture_passes.sh ${LLC_EXECUTABLE} "-O2 -filetype=obj -relocation-model=pic" ${DIRECT_OPTIMIZED_LL} ${DIRECT_MACHINE_PASSES_FILE}
    DEPENDS ${DIRECT_OPTIMIZED_LL}
    COMMENT "Capturing machine-level passes from direct clang -O2 compilation"
)

# Step 5: Link to create executable
add_custom_command(
    OUTPUT ${OPTIMIZED_BIN}
    COMMAND ${CLANG_EXECUTABLE} ${LABELED_OBJ} -o ${OPTIMIZED_BIN} -lm
    DEPENDS ${LABELED_OBJ}
    COMMENT "Linking to create optimized executable"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

add_custom_target(test5_optimization_target ALL DEPENDS ${DIRECT_BIN} ${OPTIMIZED_BIN} ${DIRECT_PASSES_FILE} ${OPT_PASSES_FILE} ${LLC_PASSES_FILE} ${DIRECT_MACHINE_PASSES_FILE} ${LABELED_LL})

# Tests
add_test(
    NAME test5_optimization_csv_exists
    COMMAND ${CMAKE_COMMAND} -E cat ${OPTIMIZED_CSV}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_optimization_csv_exists PROPERTIES 
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

add_test(
    NAME test5_optimization_binaries_exist
    COMMAND test -f ${DIRECT_BIN} -a -f ${OPTIMIZED_BIN}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_optimization_binaries_exist PROPERTIES DEPENDS test5_optimization_csv_exists)

# Test: Performance comparison (both binaries run successfully)
add_test(
    NAME test5_direct_binary_runs
    COMMAND ${DIRECT_BIN}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_direct_binary_runs PROPERTIES DEPENDS test5_optimization_binaries_exist)

add_test(
    NAME test5_optimized_binary_runs
    COMMAND ${OPTIMIZED_BIN}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_optimized_binary_runs PROPERTIES DEPENDS test5_direct_binary_runs)

# Test: Performance comparison script (if exists)
add_test(
    NAME test5_performance_comparison
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/compare_performance.sh ${DIRECT_BIN} ${OPTIMIZED_BIN} 5 ${OUTPUT_DIR}/performance_results.log
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_performance_comparison PROPERTIES DEPENDS test5_optimized_binary_runs)

# Test: Compare optimization passes between direct and pipelined
add_test(
    NAME test5_passes_comparison
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/compare_passes.sh ${DIRECT_PASSES_FILE} ${OPT_PASSES_FILE} ${LLC_PASSES_FILE} ${OUTPUT_DIR}/PASSES_COMPARISON_REPORT.md ${DIRECT_MACHINE_PASSES_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_passes_comparison PROPERTIES DEPENDS test5_performance_comparison)
# Test: Validate metadata in instrumented IR matches CSV
add_test(
    NAME test5_metadata_validation
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_metadata.py 
            ${LABELED_LL} ${OPTIMIZED_CSV}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_metadata_validation PROPERTIES 
    DEPENDS test5_passes_comparison)