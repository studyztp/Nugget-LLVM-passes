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
# Test 2: Machine Code Matching Test
#
# This test validates that IR basic blocks correctly map to machine basic blocks
# after the full compilation pipeline:
#   1. Source -> clang -O0 -> IR
#   2. IR -> opt -O2 -> Optimized IR
#   3. Optimized IR -> IRBBLabelPass -> Labeled IR + CSV
#   4. Labeled IR -> PhaseAnalysisPass -> Instrumented IR
#   5. Instrumented IR -> llc -O2 -> Assembly/Object
#   6. Object -> clang (link) -> Executable
#   7. Executable -> objdump -> Disassembly
#   8. Verify: IR hooks match machine code hooks
#
# This test ensures that the instrumentation survives the backend
# compilation and appears correctly in the final binary.

cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Test 2: Machine Code Matching
# ============================================================================

# Configuration
set(PHASE_THRESHOLD 1000)

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

# Source files
set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/test2_machine_match.c)
set(RUNTIME_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../common/nugget_runtime.c)

# Intermediate files - IR pipeline
set(TEST_LL ${OUTPUT_DIR}/test2_machine_match.ll)
set(RUNTIME_LL ${OUTPUT_DIR}/nugget_runtime.ll)
set(LINKED_LL ${OUTPUT_DIR}/test2_linked.ll)
set(OPTIMIZED_LL ${OUTPUT_DIR}/test2_optimized.ll)
set(LABELED_BC ${OUTPUT_DIR}/test2_labeled.bc)
set(LABELED_LL ${OUTPUT_DIR}/test2_labeled.ll)
set(INSTRUMENTED_BC ${OUTPUT_DIR}/test2_instrumented.bc)
set(INSTRUMENTED_LL ${OUTPUT_DIR}/test2_instrumented.ll)
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)

# Intermediate files - Backend pipeline
set(OBJECT_FILE ${OUTPUT_DIR}/test2_machine_match.o)
set(EXECUTABLE ${OUTPUT_DIR}/test2_machine_match)
set(DISASM_FILE ${OUTPUT_DIR}/test2_disasm.txt)

# ============================================================================
# Step 1: Compile test source to LLVM IR
# ============================================================================
add_custom_command(
    OUTPUT ${TEST_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm
            ${TEST_SOURCE} -o ${TEST_LL}
    DEPENDS ${TEST_SOURCE}
    COMMENT "Compiling test2_machine_match.c to LLVM IR"
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
# Step 6: Run PhaseAnalysisPass to instrument basic blocks
# ============================================================================
add_custom_command(
    OUTPUT ${INSTRUMENTED_BC}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="phase-analysis-pass<interval_length=${PHASE_THRESHOLD}>"
            ${LABELED_BC} -o ${INSTRUMENTED_BC}
    DEPENDS ${LABELED_BC} ${PASS_PLUGIN}
    COMMENT "Running PhaseAnalysisPass to instrument basic blocks"
    WORKING_DIRECTORY ${OUTPUT_DIR}
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
# Step 8: Compile to object file using llc with -O2
# ============================================================================
# Note: We use llc to go from LLVM bitcode to native object file
# This tests that our instrumentation survives the backend
# -relocation-model=pic is required for PIE executables on modern Linux
add_custom_command(
    OUTPUT ${OBJECT_FILE}
    COMMAND ${LLC_EXECUTABLE} -O2 -filetype=obj -relocation-model=pic
            ${INSTRUMENTED_BC} -o ${OBJECT_FILE}
    DEPENDS ${INSTRUMENTED_BC}
    COMMENT "Compiling instrumented bitcode to object file (llc -O2)"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Step 9: Link to executable
# ============================================================================
add_custom_command(
    OUTPUT ${EXECUTABLE}
    COMMAND ${CLANG_EXECUTABLE} ${OBJECT_FILE} -o ${EXECUTABLE}
    DEPENDS ${OBJECT_FILE}
    COMMENT "Linking object file to executable"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Step 10: Disassemble the executable
# ============================================================================
add_custom_command(
    OUTPUT ${DISASM_FILE}
    COMMAND objdump -d ${EXECUTABLE} > ${DISASM_FILE}
    DEPENDS ${EXECUTABLE}
    COMMENT "Disassembling executable"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# ============================================================================
# Target: Build all test2 artifacts
# ============================================================================
add_custom_target(test2_machine_match_target ALL 
    DEPENDS ${INSTRUMENTED_LL} ${LABELED_LL} ${CSV_FILE} ${DISASM_FILE}
)

# ============================================================================
# Test 2.1: Verify CSV file exists and has correct format
# ============================================================================
add_test(
    NAME test2_machine_match_csv_exists
    COMMAND ${CMAKE_COMMAND} -E cat ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test2_machine_match_csv_exists PROPERTIES
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

# ============================================================================
# Test 2.2: Verify executable was created
# ============================================================================
add_test(
    NAME test2_machine_match_executable_exists
    COMMAND ${CMAKE_COMMAND} -E true
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test2_machine_match_executable_exists PROPERTIES
    REQUIRED_FILES ${EXECUTABLE}
)

# ============================================================================
# Test 2.3: Run the executable to verify it doesn't crash
# ============================================================================
add_test(
    NAME test2_machine_match_runs
    COMMAND ${EXECUTABLE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test2_machine_match_runs PROPERTIES
    DEPENDS test2_machine_match_executable_exists
)

# ============================================================================
# Test 2.4: Verify IR to machine code matching
# ============================================================================
# Checks:
#   - nugget_bb_hook_ calls appear in disassembly
#   - nugget_init_ is called in nugget_roi_begin_
#   - Hook counts are reasonable (may differ due to backend optimizations)
add_test(
    NAME test2_machine_match_verification
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_machine_match.py
            ${INSTRUMENTED_LL} ${DISASM_FILE} ${CSV_FILE} ${PHASE_THRESHOLD}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test2_machine_match_verification PROPERTIES
    DEPENDS test2_machine_match_csv_exists
)
