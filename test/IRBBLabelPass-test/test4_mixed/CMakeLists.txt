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
            -passes="ir-bb-label" ${LINKED_BC} -o ${BC_FILE}
    DEPENDS ${LINKED_BC} ${PASS_PLUGIN}
    COMMENT "Running ir-bb-label pass on combined C++/Fortran module"
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
    NAME test4_mixed
    COMMAND ${CMAKE_COMMAND} -E echo "Checking test4_mixed results..."
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

add_test(
    NAME test4_mixed_csv_exists
    COMMAND ${CMAKE_COMMAND} -E cat ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test4_mixed_csv_exists PROPERTIES 
    DEPENDS test4_mixed
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
