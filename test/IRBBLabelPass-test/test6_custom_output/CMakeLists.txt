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
# Test 6: Custom CSV output filename
#
# This test validates pass parameter parsing using LLVM's new pass manager syntax:
#   -passes="ir-bb-label-pass<output_csv=custom_name.csv>"
#
# Why this test matters:
#   - Verifies pass option parsing works correctly
#   - Tests parameter passing via new pass manager
#   - Ensures custom CSV filename is used instead of default
#   - Validates pass follows LLVM parameter conventions
#
# Parameter syntax:
#   Pass name: ir-bb-label-pass
#   Option format: <key=value>
#   Default: output_csv=bb_info.csv
#   This test: output_csv=my_custom_bb_output.csv
#
# CMake escaping:
#   Angle brackets must be escaped to prevent shell interpretation:
#   -passes="ir-bb-label-pass\<output_csv=...\>"
#
# Expected behavior:
#   - my_custom_bb_output.csv is created (not bb_info.csv)
#   - File contains standard CSV format
#   - All basic blocks are labeled correctly
#
# Tests registered:
#   1. test6_custom_csv_exists (checks custom filename)
#   2. test6_custom_csv_has_content
#   3. test6_custom_metadata_validation

cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Test 6: Custom output CSV filename
# ============================================================================

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/test6_custom_output.c)
set(LL_FILE ${OUTPUT_DIR}/test6_custom_output.ll)
set(BC_FILE ${OUTPUT_DIR}/test6_custom_output_instrumented.bc)
set(CUSTOM_CSV_FILE ${OUTPUT_DIR}/my_custom_bb_output.csv)
set(READABLE_LL ${OUTPUT_DIR}/test6_custom_output_instrumented.ll)

# Step 1: Compile source to LLVM IR
add_custom_command(
    OUTPUT ${LL_FILE}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm 
            ${SOURCE_FILE} -o ${LL_FILE}
    DEPENDS ${SOURCE_FILE}
    COMMENT "Compiling test6_custom_output.c to LLVM IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2: Run the pass with custom output_csv parameter
# Use escaped angle brackets to prevent shell interpretation
add_custom_command(
    OUTPUT ${BC_FILE}
    COMMAND ${OPT_EXECUTABLE}
            -load-pass-plugin=${PASS_PLUGIN}
            -passes "ir-bb-label-pass\\<output_csv=my_custom_bb_output.csv\\>"
            ${LL_FILE}
            -o ${BC_FILE}
    DEPENDS ${LL_FILE} ${PASS_PLUGIN}
    COMMENT "Running ir-bb-label-pass with custom output CSV on test6"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 3: Convert back to readable format
add_custom_command(
    OUTPUT ${READABLE_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${BC_FILE} -o ${READABLE_LL}
    DEPENDS ${BC_FILE}
    COMMENT "Converting test6_custom_output bitcode to readable IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Create target
add_custom_target(test6_custom_output_target ALL DEPENDS ${READABLE_LL})

# Add tests
add_test(
    NAME test6_custom_csv_exists
    COMMAND ${CMAKE_COMMAND} -E cat ${CUSTOM_CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test6_custom_csv_exists PROPERTIES 
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

add_test(
    NAME test6_custom_csv_has_content
    COMMAND ${CMAKE_COMMAND} 
            -DCSV_FILE=${CUSTOM_CSV_FILE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_csv.cmake
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test6_custom_csv_has_content PROPERTIES DEPENDS test6_custom_csv_exists)
add_test(
    NAME test6_custom_metadata_validation
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_metadata.py 
            ${READABLE_LL} ${CUSTOM_CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test6_custom_metadata_validation PROPERTIES 
    DEPENDS test6_custom_csv_has_content)