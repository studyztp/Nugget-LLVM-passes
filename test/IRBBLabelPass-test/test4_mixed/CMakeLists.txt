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
# Test 4: Mixed C++ and Fortran (multi-language module)
#
# This test validates the IRBBLabel pass on merged multi-language LLVM modules:
#   - C++ code with mangled names and complex features
#   - Fortran subroutines with bind(C) for C interoperability
#   - Both languages compiled to separate IR files
#   - Linked together using llvm-link into single module
#   - Pass processes the merged module
#
# Why this test matters:
#   - Real HPC codes mix C++/Fortran extensively
#   - Pass must handle both C++ mangling and Fortran conventions
#   - Tests llvm-link compatibility (merged modules)
#   - Verifies pass doesn't crash on multi-language IR
#
# Compilation pipeline:
#   1. C++ source -> clang++ -> cpp.ll
#   2. Fortran source -> flang-new -> fortran.ll  
#   3. llvm-link cpp.ll fortran.ll -> merged.bc
#   4. opt + IRBBLabel pass on merged.bc -> instrumented.bc
#   5. llvm-dis -> instrumented.ll (readable)
#
# Fortran interoperability:
#   - Fortran uses bind(C) to export functions with C-compatible names
#   - C++ uses extern "C" to call Fortran functions
#   - No name mangling on Fortran functions (fortran_add, fortran_factorial)
#
# Expected CSV output:
#   - C++ functions with mangled names (_ZN...)
#   - Fortran functions with bind(C) names (fortran_add, fortran_factorial)
#   - All basic blocks from both languages
#
# Tests registered:
#   1. test4_mixed_csv_exists
#   2. test4_mixed_csv_has_content
#   3. test4_mixed_metadata_validation

cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Test 4: Mixed C++ and Fortran
# ============================================================================

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(CPP_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/test4_mixed_cpp_fortran.cpp)
set(F90_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/fortran_part.f90)

set(CPP_LL ${OUTPUT_DIR}/test4_cpp.ll)
set(F90_LL ${OUTPUT_DIR}/test4_fortran.ll)
set(LINKED_BC ${OUTPUT_DIR}/test4_linked.bc)
set(BC_FILE ${OUTPUT_DIR}/test4_mixed_instrumented.bc)
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)
set(READABLE_LL ${OUTPUT_DIR}/test4_mixed_instrumented.ll)

# Step 1a: Compile C++ part to LLVM IR
add_custom_command(
    OUTPUT ${CPP_LL}
    COMMAND ${CLANGXX_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm 
            ${CPP_SOURCE} -o ${CPP_LL}
    DEPENDS ${CPP_SOURCE}
    COMMENT "Compiling C++ part to LLVM IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 1b: Compile Fortran part to LLVM IR
add_custom_command(
    OUTPUT ${F90_LL}
    COMMAND ${FLANG_EXECUTABLE} -O0 -S -emit-llvm 
            ${F90_SOURCE} -o ${F90_LL}
    DEPENDS ${F90_SOURCE}
    COMMENT "Compiling Fortran part to LLVM IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2: Link both IR files into single bitcode module
add_custom_command(
    OUTPUT ${LINKED_BC}
    COMMAND llvm-link ${CPP_LL} ${F90_LL} -o ${LINKED_BC}
    DEPENDS ${CPP_LL} ${F90_LL}
    COMMENT "Linking C++ and Fortran IR into single module"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 3: Run the pass
add_custom_command(
    OUTPUT ${BC_FILE}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label-pass" ${LINKED_BC} -o ${BC_FILE}
    DEPENDS ${LINKED_BC} ${PASS_PLUGIN}
    COMMENT "Running ir-bb-label-pass on combined C++/Fortran module"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 4: Convert back to readable format
add_custom_command(
    OUTPUT ${READABLE_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${BC_FILE} -o ${READABLE_LL}
    DEPENDS ${BC_FILE}
    COMMENT "Converting combined bitcode to readable IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Create target
add_custom_target(test4_mixed_target ALL DEPENDS ${READABLE_LL})

# Add tests
add_test(
    NAME test4_mixed_csv_exists
    COMMAND ${CMAKE_COMMAND} -E cat ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test4_mixed_csv_exists PROPERTIES 
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

add_test(
    NAME test4_mixed_csv_has_content
    COMMAND ${CMAKE_COMMAND} 
            -DCSV_FILE=${CSV_FILE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_csv.cmake
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test4_mixed_csv_has_content PROPERTIES DEPENDS test4_mixed_csv_exists)
add_test(
    NAME test4_mixed_metadata_validation
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_metadata.py 
            ${READABLE_LL} ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test4_mixed_metadata_validation PROPERTIES 
    DEPENDS test4_mixed_csv_has_content)