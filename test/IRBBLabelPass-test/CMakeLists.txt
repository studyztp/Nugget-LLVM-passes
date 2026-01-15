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
# CMake configuration for IRBBLabel pass test suite
#
# This file configures the test infrastructure for validating the IRBBLabel pass.
# The tests verify that the pass correctly:
#   - Instruments LLVM IR with basic block metadata (!bb.id)
#   - Generates CSV files with function and BB information
#   - Handles various code features (C, C++, Fortran, optimization, etc.)
#   - Works correctly in different compilation pipelines
#
# Required CMake variables (set via -D flag):
#   LLVM_BIN_DIR: Path to LLVM toolchain binaries (clang, opt, llvm-dis, etc.)
#   PASS_PLUGIN: Path to compiled NuggetPasses.so shared library
#
# Usage:
#   cmake -S . -B build \
#     -DLLVM_BIN_DIR=/path/to/llvm/bin \
#     -DPASS_PLUGIN=/path/to/NuggetPasses.so
#   cmake --build build
#   cd build && ctest --output-on-failure
#
# Test structure:
#   - test1_simple: Basic C code with simple control flow
#   - test2_dynamic_lib: Dynamic libraries, empty functions, edge cases
#   - test3_cpp_static: C++ features (templates, overloading, mangling)
#   - test4_mixed: Mixed C++ and Fortran (multi-language modules)
#   - test5_optimization: Optimization pipeline comparison (-O2 effects)
#   - test6_custom_output: Custom CSV filename parameter testing
#
# Each test runs 4 validations:
#   1. CSV file exists
#   2. CSV format is valid (correct headers)
#   3. CSV content is valid (proper data, non-empty)
#   4. IR metadata matches CSV (verify_metadata.py)

cmake_minimum_required(VERSION 3.20)
project(NuggetPassesTests)

# ============================================================================
# Configuration options
# ============================================================================

# LLVM_BIN_DIR: Directory containing LLVM binaries
# This should point to the bin/ directory of your LLVM installation.
# Required tools: clang, clang++, opt, llvm-dis, llc, flang-new
# Example: /usr/local/llvm-23/bin or ~/llvm-dir/bin
set(LLVM_BIN_DIR "" CACHE PATH "Path to LLVM bin directory (containing clang, opt, etc.)")

# PASS_PLUGIN: Path to the compiled pass plugin
# This is the shared library (.so on Linux) built from the pass/ directory.
# The plugin contains the IRBBLabel pass implementation.
# Example: ../pass/build/NuggetPasses.so
set(PASS_PLUGIN "" CACHE FILEPATH "Path to the NuggetPasses plugin (.so file)")

# ============================================================================
# Find LLVM tools
# ============================================================================
# Locate all required LLVM tools in the specified bin directory.
# NO_DEFAULT_PATH ensures we only use tools from LLVM_BIN_DIR, not system PATH.
# This prevents accidentally using incompatible versions from /usr/bin.

if(NOT LLVM_BIN_DIR)
    message(FATAL_ERROR "LLVM_BIN_DIR must be set. Use -DLLVM_BIN_DIR=/path/to/llvm/bin")
endif()

if(NOT PASS_PLUGIN)
    message(FATAL_ERROR "PASS_PLUGIN must be set. Use -DPASS_PLUGIN=/path/to/NuggetPasses.so")
endif()

# Required tools for all tests
find_program(CLANG_EXECUTABLE clang PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH REQUIRED)
find_program(CLANGXX_EXECUTABLE clang++ PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH REQUIRED)
find_program(OPT_EXECUTABLE opt PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH REQUIRED)

# Optional tools (some tests only)
find_program(LLVM_DIS_EXECUTABLE llvm-dis PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH)  # Disassembler
find_program(LLC_EXECUTABLE llc PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH)             # Code generator
find_program(FLANG_EXECUTABLE flang-new PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH)    # Fortran compiler

# Print configuration for verification
message(STATUS "CLANG: ${CLANG_EXECUTABLE}")
message(STATUS "CLANG++: ${CLANGXX_EXECUTABLE}")
message(STATUS "OPT: ${OPT_EXECUTABLE}")
message(STATUS "LLVM-DIS: ${LLVM_DIS_EXECUTABLE}")
message(STATUS "LLC: ${LLC_EXECUTABLE}")
message(STATUS "FLANG: ${FLANG_EXECUTABLE}")
message(STATUS "PASS PLUGIN: ${PASS_PLUGIN}")

# Verify pass plugin exists before proceeding
if(NOT NUGGET_FROM_PARENT_BUILD AND NOT EXISTS ${PASS_PLUGIN})
    message(FATAL_ERROR "Pass plugin not found at: ${PASS_PLUGIN}")
endif()

# Enable CTest framework for test execution
enable_testing()

# Prefixes used to avoid name collisions when multiple suites are included by
# the aggregated top-level project. When built standalone, prefixes stay empty.
set(NUGGET_TARGET_PREFIX "")
set(NUGGET_TEST_PREFIX "")
if(NUGGET_AGGREGATED_BUILD)
    set(NUGGET_TARGET_PREFIX "IRBBLabelPass-")
    set(NUGGET_TEST_PREFIX "IRBBLabelPass-")
endif()

# ============================================================================
# Add test subdirectories
# ============================================================================
# Each subdirectory contains a test case with its own CMakeLists.txt.
# Tests are built and registered automatically by subdirectory CMakeLists.

add_subdirectory(test1_simple)         # Basic C code
add_subdirectory(test2_dynamic_lib)    # Dynamic libraries, edge cases
add_subdirectory(test3_cpp_static)     # C++ features, name mangling
add_subdirectory(test4_mixed)          # C++ and Fortran interoperability

# Test 5 requires llc for binary generation
if(LLC_EXECUTABLE)
    add_subdirectory(test5_optimization)  # Optimization pipeline comparison
endif()

add_subdirectory(test6_custom_output)  # Custom CSV filename

# ============================================================================

