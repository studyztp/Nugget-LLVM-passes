# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# PhaseBoundPass Test Suite
#
# This test suite validates the PhaseBoundPass LLVM instrumentation pass.
# PhaseBoundPass instruments specific basic blocks (markers) to track
# phase boundaries during program execution.
#
# Test structure:
#   test1_simple/          - Basic marker instrumentation test
#
# Requirements:
#   - LLVM toolchain (clang, opt, llvm-link, llvm-dis)
#   - NuggetPasses.so plugin (IRBBLabelPass and PhaseBoundPass)
#   - Python 3 for verification scripts
#
# Usage:
#   cmake -B build -S . \
#         -DLLVM_BIN_DIR=/path/to/llvm/bin \
#         -DPASS_PLUGIN=/path/to/NuggetPasses.so
#   cmake --build build
#   cd build && ctest --output-on-failure

cmake_minimum_required(VERSION 3.20)
project(PhaseBoundPass-Test LANGUAGES C CXX)

# ============================================================================
# Configuration
# ============================================================================

# User must provide LLVM_BIN_DIR and PASS_PLUGIN via command-line:
#   -DLLVM_BIN_DIR=/path/to/llvm/bin
#   -DPASS_PLUGIN=/path/to/NuggetPasses.so
#
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

# Print configuration for verification
message(STATUS "CLANG: ${CLANG_EXECUTABLE}")
message(STATUS "CLANG++: ${CLANGXX_EXECUTABLE}")
message(STATUS "OPT: ${OPT_EXECUTABLE}")
message(STATUS "LLVM-DIS: ${LLVM_DIS_EXECUTABLE}")
message(STATUS "LLVM-LINK: ${LLVM_LINK_EXECUTABLE}")
message(STATUS "PASS PLUGIN: ${PASS_PLUGIN}")

# Verify pass plugin exists before proceeding
if(NOT EXISTS ${PASS_PLUGIN})
    message(FATAL_ERROR "Pass plugin not found at: ${PASS_PLUGIN}")
endif()

# Enable CTest framework for test execution
enable_testing()

# Prefixes used to avoid name collisions when multiple suites are included by
# the aggregated top-level project. When built standalone, prefixes stay empty.
set(NUGGET_TARGET_PREFIX "")
set(NUGGET_TEST_PREFIX "")
if(NUGGET_AGGREGATED_BUILD)
    set(NUGGET_TARGET_PREFIX "PhaseBoundPass-")
    set(NUGGET_TEST_PREFIX "PhaseBoundPass-")
endif()

# ============================================================================
# Add test subdirectories
# ============================================================================
# Each subdirectory contains a test case with its own CMakeLists.txt.
# Tests are built and registered automatically by subdirectory CMakeLists.

add_subdirectory(test1_simple)         # Basic marker instrumentation test
