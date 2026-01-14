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
# CMake build configuration for Nugget LLVM passes plugin.
#
# This CMake file builds an LLVM pass plugin (shared library) that can be loaded
# by LLVM tools (opt, clang, etc.) to run custom analysis and transformation
# passes on LLVM IR.
#
# Build Requirements:
#   - CMake 3.20 or later
#   - LLVM installation with development files
#   - C++17 compiler
#
# Build Instructions:
#   cmake -S . -B build -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm
#   cmake --build build
#
# Output:
#   build/NuggetPasses.so (Linux)
#   build/NuggetPasses.dylib (macOS)
#   build/NuggetPasses.dll (Windows)
#
# Usage:
#   opt -load-pass-plugin=./NuggetPasses.so \
#       -passes="ir-bb-label-pass" \
#       input.ll -o output.bc

cmake_minimum_required(VERSION 3.20)
project(NuggetPasses LANGUAGES CXX)

# ============================================================================
# Find LLVM Installation
# ============================================================================
# find_package(LLVM) locates LLVM CMake config files and sets variables:
#   LLVM_DIR: Directory containing LLVMConfig.cmake
#   LLVM_CMAKE_DIR: Directory with LLVM CMake modules
#   LLVM_INCLUDE_DIRS: LLVM header directories
#   LLVM_DEFINITIONS: Compiler definitions (-D flags)
#
# REQUIRED: Build fails if LLVM is not found
# CONFIG: Use LLVMConfig.cmake (not FindLLVM.cmake)
find_package(LLVM REQUIRED CONFIG)

# Print LLVM configuration for verification
message(STATUS "LLVM_DIR=${LLVM_DIR}")
message(STATUS "LLVM_CMAKE_DIR=${LLVM_CMAKE_DIR}")
message(STATUS "LLVM_INCLUDE_DIRS=${LLVM_INCLUDE_DIRS}")

# ============================================================================
# Import LLVM CMake Modules
# ============================================================================
# Add LLVM's CMake module directory to CMAKE_MODULE_PATH so we can use
# LLVM's custom CMake functions like add_llvm_pass_plugin().
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
# AddLLVM provides add_llvm_pass_plugin() and other pass-building utilities
include(AddLLVM)

# ============================================================================
# Build Pass Plugin
# ============================================================================
# add_llvm_pass_plugin() creates a shared library configured for LLVM pass plugin usage:
#   - Sets correct shared library properties
#   - Configures proper symbol visibility
#   - Links against LLVM libraries
#
# Source files:
#   PluginRegistration.cpp: Plugin entry point and pass registration
#   IRBBLabelPass.cpp: IRBBLabelPass implementation
#
# PARTIAL_SOURCES_INTENDED: Indicates some source files are intentionally
# excluded (e.g., PhaseAnalysisPass.cpp is in tree but not built yet)
add_llvm_pass_plugin(NuggetPasses
  PluginRegistration.cpp
  IRBBLabelPass.cpp
  PARTIAL_SOURCES_INTENDED
)

# ============================================================================
# Compiler Configuration
# ============================================================================
# Include directories - Ensure LLVM headers are visible to compiler.
# PRIVATE: These includes are only for building this target, not consumers.
# Critical for non-standard LLVM installations (~/llvm-dir, /opt/llvm, etc.)
target_include_directories(NuggetPasses PRIVATE ${LLVM_INCLUDE_DIRS})

# Compiler definitions - Pass LLVM's -D flags to compiler.
# These may include version macros, feature flags, etc.
target_compile_definitions(NuggetPasses PRIVATE ${LLVM_DEFINITIONS})

# C++ standard - Require C++17 (minimum for modern LLVM).
# LLVM 16+ requires C++17, LLVM 18+ may require C++20.
target_compile_features(NuggetPasses PRIVATE cxx_std_17)
