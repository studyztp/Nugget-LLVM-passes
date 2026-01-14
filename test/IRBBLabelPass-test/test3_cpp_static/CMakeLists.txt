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
# Test 3: C++ features with name mangling and templates
#
# This test validates the IRBBLabel pass on advanced C++ constructs:
#   - Name mangling (C++ functions encoded with type info)
#   - Function overloading (same name, different signatures)
#   - Template functions and methods (separate instantiations per type)
#   - Anonymous namespaces (internal linkage)
#   - Lambda expressions (compiler-generated function objects)
#   - Nested classes and their methods
#   - Range-based for loops (C++11)
#
# Why this test matters:
#   - Pass must handle mangled function names (_ZN...)
#   - Each template instantiation creates a separate IR function
#   - Overloaded functions are distinct entities in IR
#   - Lambda closures compile to operator() methods
#   - Tests pass compatibility with modern C++ features
#
# Compiler:
#   Uses clang++ instead of clang for C++ compilation
#
# Expected CSV output:
#   Function names will be mangled (e.g., _ZN10Calculator3addEi)
#   Each template instantiation appears as separate function
#
# Tests registered:
#   1. test3_cpp_static_csv_exists
#   2. test3_cpp_static_csv_has_content
#   3. test3_cpp_static_metadata_validation

cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Test 3: C++ static code
# ============================================================================

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/test3_cpp_static.cpp)
set(LL_FILE ${OUTPUT_DIR}/test3_cpp_static.ll)
set(BC_FILE ${OUTPUT_DIR}/test3_cpp_static_instrumented.bc)
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)
set(READABLE_LL ${OUTPUT_DIR}/test3_cpp_static_instrumented.ll)

# Step 1: Compile source to LLVM IR
add_custom_command(
    OUTPUT ${LL_FILE}
    COMMAND ${CLANGXX_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm 
            ${SOURCE_FILE} -o ${LL_FILE}
    DEPENDS ${SOURCE_FILE}
    COMMENT "Compiling test3_cpp_static.cpp to LLVM IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2: Run the pass
add_custom_command(
    OUTPUT ${BC_FILE}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label-pass" ${LL_FILE} -o ${BC_FILE}
    DEPENDS ${LL_FILE} ${PASS_PLUGIN}
    COMMENT "Running ir-bb-label-pass on test3_cpp_static"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 3: Convert back to readable format
add_custom_command(
    OUTPUT ${READABLE_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${BC_FILE} -o ${READABLE_LL}
    DEPENDS ${BC_FILE}
    COMMENT "Converting test3_cpp_static bitcode to readable IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Create target
add_custom_target(test3_cpp_static_target ALL DEPENDS ${READABLE_LL})

# Add tests
add_test(
    NAME test3_cpp_static_csv_exists
    COMMAND ${CMAKE_COMMAND} -E cat ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test3_cpp_static_csv_exists PROPERTIES 
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

add_test(
    NAME test3_cpp_static_csv_has_content
    COMMAND ${CMAKE_COMMAND} 
            -DCSV_FILE=${CSV_FILE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_csv.cmake
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test3_cpp_static_csv_has_content PROPERTIES DEPENDS test3_cpp_static_csv_exists)
add_test(
    NAME test3_cpp_static_metadata_validation
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_metadata.py 
            ${READABLE_LL} ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test3_cpp_static_metadata_validation PROPERTIES 
    DEPENDS test3_cpp_static_csv_has_content)