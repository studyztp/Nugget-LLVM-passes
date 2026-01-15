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
# PhaseAnalysisPass Test Suite
#
# This CMakeLists.txt configures the test environment for PhaseAnalysisPass
# and includes all test subdirectories.
#
# The tests verify that the pass correctly:
#   - Inserts nugget_init_ call at the start of nugget_roi_begin_
#   - Inserts nugget_bb_hook_ calls in all basic blocks
#   - Reads !bb.id metadata from IRBBLabelPass
#   - Skips nugget functions (nugget_init_, nugget_roi_begin_, etc.)
#
# Test Structure:
#   common/         - Shared runtime functions (nugget_runtime.c) and 
#                     verification scripts
#   test1_simple/   - Basic instrumentation test
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

cmake_minimum_required(VERSION 3.20)
project(PhaseAnalysisPassTests)

# ============================================================================
# Configuration options
# ============================================================================

# LLVM_BIN_DIR: Directory containing LLVM binaries
# This should point to the bin/ directory of your LLVM installation.
# Required tools: clang, clang++, opt, llvm-dis, llvm-link
# Example: /usr/local/llvm-18/bin or ~/llvm-dir/bin
set(LLVM_BIN_DIR "" CACHE PATH "Path to LLVM bin directory (containing clang, opt, etc.)")

# PASS_PLUGIN: Path to the compiled pass plugin
# This is the shared library (.so on Linux) built from the pass/ directory.
# The plugin contains the PhaseAnalysisPass implementation.
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
find_program(LLVM_DIS_EXECUTABLE llvm-dis PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH REQUIRED)
find_program(LLVM_LINK_EXECUTABLE llvm-link PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH REQUIRED)

# Optional tools (required for test2_machine_match)
find_program(LLC_EXECUTABLE llc PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH)

# Print configuration for verification
message(STATUS "CLANG: ${CLANG_EXECUTABLE}")
message(STATUS "CLANG++: ${CLANGXX_EXECUTABLE}")
message(STATUS "OPT: ${OPT_EXECUTABLE}")
message(STATUS "LLVM-DIS: ${LLVM_DIS_EXECUTABLE}")
message(STATUS "LLVM-LINK: ${LLVM_LINK_EXECUTABLE}")
message(STATUS "LLC: ${LLC_EXECUTABLE}")
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
    set(NUGGET_TARGET_PREFIX "PhaseAnalysisPass-")
    set(NUGGET_TEST_PREFIX "PhaseAnalysisPass-")
endif()

# ============================================================================
# Architecture Detection for Test 2
# ============================================================================
# Test 2 requires disassembly trace which is architecture-specific
# Currently supported: x86_64, aarch64
set(TEST2_SUPPORTED_ARCH FALSE)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
    set(TEST2_SUPPORTED_ARCH TRUE)
    message(STATUS "Test 2 architecture: x86-64 (supported)")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    set(TEST2_SUPPORTED_ARCH TRUE)
    message(STATUS "Test 2 architecture: AArch64 (supported)")
else()
    message(WARNING "Test 2 (machine match) skipped: architecture '${CMAKE_SYSTEM_PROCESSOR}' not supported for disassembly trace. Supported architectures: x86-64, AArch64.")
endif()

# ============================================================================
# Add test subdirectories
# ============================================================================
# Each subdirectory contains a test case with its own CMakeLists.txt.
# Tests are built and registered automatically by subdirectory CMakeLists.

add_subdirectory(test1_simple)         # Basic instrumentation test

# Test 2 requires llc for machine code generation and supported architecture
if(LLC_EXECUTABLE AND TEST2_SUPPORTED_ARCH)
    add_subdirectory(test2_machine_match)  # IR to machine code matching
elseif(LLC_EXECUTABLE AND NOT TEST2_SUPPORTED_ARCH)
    message(STATUS "Test 2 skipped: Architecture not supported (see warning above)")
endif()
