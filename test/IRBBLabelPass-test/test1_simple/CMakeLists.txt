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
# Test 1: Simple C code with basic control flow
#
# This test validates the IRBBLabel pass on straightforward C code featuring:
#   - Multiple functions (add, subtract, main)
#   - Simple control flow (if/else branches)
#   - Function calls
#   - Basic blocks with clear structure
#
# Compilation pipeline:
#   1. source.c -> clang -O0 -emit-llvm -> unoptimized.ll
#   2. unoptimized.ll -> opt + IRBBLabel pass -> instrumented.bc
#   3. instrumented.bc -> llvm-dis -> instrumented.ll (readable)
#
# Why -O0 -Xclang -disable-O0-optnone:
#   - -O0: No optimization, preserves original structure
#   - -disable-O0-optnone: Removes 'optnone' attribute that blocks opt passes
#   Without -disable-O0-optnone, 'opt' would skip the function entirely
#
# Tests registered:
#   1. test1_simple_csv_exists: Verifies CSV file was generated
#   2. test1_simple_csv_has_content: Validates CSV format and content
#   3. test1_simple_metadata_validation: Cross-checks IR metadata vs CSV

cmake_minimum_required(VERSION 3.20)
# ============================================================================
# Test 1: Simple C code
# ============================================================================

# Set up output directory for generated files
set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

# Define file paths for build pipeline
set(SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/test1_simple.c)
set(LL_FILE ${OUTPUT_DIR}/test1_simple.ll)              # Unoptimized LLVM IR
set(BC_FILE ${OUTPUT_DIR}/test1_simple_instrumented.bc) # Instrumented bitcode
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)                 # Pass output
set(READABLE_LL ${OUTPUT_DIR}/test1_simple_instrumented.ll)  # Readable instrumented IR

# ============================================================================
# Step 1: Compile source to LLVM IR
# ============================================================================
# Command: clang -O0 -Xclang -disable-O0-optnone -S -emit-llvm source.c -o output.ll
#
# Flags explained:
#   -O0: No optimization (preserves original code structure)
#   -Xclang -disable-O0-optnone: Removes 'optnone' attribute
#        Without this, functions are marked 'optnone' which tells opt to skip them
#   -S: Output assembly (LLVM IR in this case)
#   -emit-llvm: Generate LLVM IR instead of native assembly
#
# DEPENDS: Rebuilds if source file changes
add_custom_command(
    OUTPUT ${LL_FILE}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm 
            ${SOURCE_FILE} -o ${LL_FILE}
    DEPENDS ${SOURCE_FILE}
    COMMENT "Compiling test1_simple.c to LLVM IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Step 2: Run the IRBBLabel pass
# ============================================================================
# Command: opt -load-pass-plugin=NuggetPasses.so -passes="ir-bb-label-pass" input.ll -o output.bc
#
# Flags explained:
#   -load-pass-plugin: Loads the pass plugin shared library
#   -passes="ir-bb-label-pass": Runs our custom pass (new pass manager syntax)
#   input.ll: LLVM IR input
#   -o output.bc: Output instrumented bitcode
#
# The pass:
#   - Iterates through all functions and basic blocks
#   - Assigns unique IDs to each basic block
#   - Inserts !bb.id metadata on terminator instructions
#   - Writes CSV file with function and BB information
#
# DEPENDS: Rebuilds if input IR or pass plugin changes
add_custom_command(
    OUTPUT ${BC_FILE}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label-pass" ${LL_FILE} -o ${BC_FILE}
    DEPENDS ${LL_FILE} ${PASS_PLUGIN}
    COMMENT "Running ir-bb-label-pass on test1_simple"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Step 3: Convert bitcode to readable IR
# ============================================================================
# Command: llvm-dis input.bc -o output.ll
#
# llvm-dis (disassembler) converts binary bitcode to human-readable IR.
# This is needed for the metadata validation script to parse !bb.id annotations.
#
# DEPENDS: Rebuilds if instrumented bitcode changes
add_custom_command(
    OUTPUT ${READABLE_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${BC_FILE} -o ${READABLE_LL}
    DEPENDS ${BC_FILE}
    COMMENT "Converting test1_simple bitcode to readable IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Target: Build all test1 artifacts
# ============================================================================
# ALL: Build automatically with 'make' or 'cmake --build'
# DEPENDS: Ensures all steps run in order
add_custom_target(test1_simple_target ALL DEPENDS ${READABLE_LL})

# ============================================================================
# Test 1.1: Verify CSV file exists
# ============================================================================
# Uses CMake's -E cat command to read file (fails if file doesn't exist)
# PASS_REGULAR_EXPRESSION: Checks for correct CSV header
add_test(
    NAME test1_simple_csv_exists
    COMMAND ${CMAKE_COMMAND} -E cat ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test1_simple_csv_exists PROPERTIES 
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

# ============================================================================
# Test 1.2: Verify CSV has valid content
# ============================================================================
# Runs CMake script to validate CSV format and non-empty content
# DEPENDS: Only runs if csv_exists test passes
add_test(
    NAME test1_simple_csv_has_content
    COMMAND ${CMAKE_COMMAND} 
            -DCSV_FILE=${CSV_FILE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_csv.cmake
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test1_simple_csv_has_content PROPERTIES DEPENDS test1_simple_csv_exists)

# ============================================================================
# Test 1.3: Validate IR metadata matches CSV
# ============================================================================
# Runs Python script to cross-validate IR metadata against CSV output
# Ensures !bb.id annotations in IR match BasicBlockID in CSV
# DEPENDS: Only runs if csv_has_content test passes
add_test(
    NAME test1_simple_metadata_validation
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_metadata.py 
            ${READABLE_LL} ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test1_simple_metadata_validation PROPERTIES 
    DEPENDS test1_simple_csv_has_content)