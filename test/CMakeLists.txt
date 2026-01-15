# SPDX-License-Identifier: BSD-3-Clause
# Top-level test runner for Nugget LLVM passes
# Builds and runs all test suites except Pipeline-test.

cmake_minimum_required(VERSION 3.20)
project(NuggetPasses-AllTests)

# User-provided locations
# When called from parent build, NUGGET_FROM_PARENT_BUILD will be ON and
# LLVM_BIN_DIR and PASS_PLUGIN will be set as normal variables.
# When running standalone, user must provide them as cache variables.
if(NOT NUGGET_FROM_PARENT_BUILD)
  if(NOT PASS_PLUGIN)
    set(PASS_PLUGIN "" CACHE FILEPATH "Path to the NuggetPasses plugin (.so file)")
  endif()
endif()

if(NOT LLVM_BIN_DIR)
    set(LLVM_BIN_DIR "" CACHE PATH "Path to LLVM bin directory (clang, opt, etc.)")
endif()

if(NOT LLVM_BIN_DIR)
    message(FATAL_ERROR "LLVM_BIN_DIR must be set. Use -DLLVM_BIN_DIR=/path/to/llvm/bin")
endif()

if(NOT PASS_PLUGIN)
    message(FATAL_ERROR "PASS_PLUGIN must be set. Use -DPASS_PLUGIN=/path/to/NuggetPasses.so")
endif()

# Only check plugin existence when running standalone (not from parent build)
# During parent build, the plugin doesn't exist yet at configure time
if(NOT NUGGET_FROM_PARENT_BUILD)
  if(NOT EXISTS ${PASS_PLUGIN})
      message(FATAL_ERROR "Pass plugin not found at: ${PASS_PLUGIN}")
  endif()
endif()

# Switches to include/exclude individual suites
option(ENABLE_IRBBLABEL_TESTS "Build IRBBLabelPass test suite" ON)
option(ENABLE_PHASE_ANALYSIS_TESTS "Build PhaseAnalysisPass test suite" ON)
option(ENABLE_PHASE_BOUND_TESTS "Build PhaseBoundPass test suite" ON)

# Enable CTest at the top so all subdir tests are registered
enable_testing()

# Mark that we are in the aggregated multi-suite build so subdirectories can
# disambiguate target/test names.
set(NUGGET_AGGREGATED_BUILD ON)

if(ENABLE_IRBBLABEL_TESTS)
    add_subdirectory(IRBBLabelPass-test)
endif()

if(ENABLE_PHASE_ANALYSIS_TESTS)
    add_subdirectory(PhaseAnalysisPass-test)
endif()

if(ENABLE_PHASE_BOUND_TESTS)
    add_subdirectory(PhaseBoundPass-test)
endif()

# Summary
message(STATUS "LLVM bin: ${LLVM_BIN_DIR}")
message(STATUS "Pass plugin: ${PASS_PLUGIN}")
message(STATUS "IRBBLabel tests: ${ENABLE_IRBBLABEL_TESTS}")
message(STATUS "PhaseAnalysis tests: ${ENABLE_PHASE_ANALYSIS_TESTS}")
message(STATUS "PhaseBound tests: ${ENABLE_PHASE_BOUND_TESTS}")
