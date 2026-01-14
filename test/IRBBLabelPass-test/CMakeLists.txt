cmake_minimum_required(VERSION 3.20)
project(NuggetPassesTests)

# ============================================================================
# Configuration options
# ============================================================================

# Path to LLVM binaries directory
set(LLVM_BIN_DIR "" CACHE PATH "Path to LLVM bin directory (containing clang, opt, etc.)")

# Path to the pass plugin .so file
set(PASS_PLUGIN "" CACHE FILEPATH "Path to the NuggetPasses plugin (.so file)")

# ============================================================================
# Find LLVM tools
# ============================================================================

if(NOT LLVM_BIN_DIR)
    message(FATAL_ERROR "LLVM_BIN_DIR must be set. Use -DLLVM_BIN_DIR=/path/to/llvm/bin")
endif()

if(NOT PASS_PLUGIN)
    message(FATAL_ERROR "PASS_PLUGIN must be set. Use -DPASS_PLUGIN=/path/to/NuggetPasses.so")
endif()

# Verify LLVM tools exist
find_program(CLANG_EXECUTABLE clang PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH REQUIRED)
find_program(CLANGXX_EXECUTABLE clang++ PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH REQUIRED)
find_program(OPT_EXECUTABLE opt PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH REQUIRED)
find_program(LLVM_DIS_EXECUTABLE llvm-dis PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH)
find_program(LLC_EXECUTABLE llc PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH)
find_program(FLANG_EXECUTABLE flang-new PATHS ${LLVM_BIN_DIR} NO_DEFAULT_PATH)

message(STATUS "CLANG: ${CLANG_EXECUTABLE}")
message(STATUS "CLANG++: ${CLANGXX_EXECUTABLE}")
message(STATUS "OPT: ${OPT_EXECUTABLE}")
message(STATUS "LLVM-DIS: ${LLVM_DIS_EXECUTABLE}")
message(STATUS "LLC: ${LLC_EXECUTABLE}")
message(STATUS "FLANG: ${FLANG_EXECUTABLE}")
message(STATUS "PASS PLUGIN: ${PASS_PLUGIN}")

# Verify pass plugin exists
if(NOT EXISTS ${PASS_PLUGIN})
    message(FATAL_ERROR "Pass plugin not found at: ${PASS_PLUGIN}")
endif()

enable_testing()

# ============================================================================
# Add test subdirectories
# ============================================================================

add_subdirectory(test1_simple)
add_subdirectory(test2_dynamic_lib)
add_subdirectory(test3_cpp_static)
add_subdirectory(test4_mixed)

if(LLC_EXECUTABLE)
    add_subdirectory(test5_optimization)
endif()

# ============================================================================
# Summary test
# ============================================================================

add_test(
    NAME summary
    COMMAND ${CMAKE_COMMAND} -E echo "All tests completed successfully!"
)
