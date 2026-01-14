cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Test 1: Simple C code
# ============================================================================

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/test1_simple.c)
set(LL_FILE ${OUTPUT_DIR}/test1_simple.ll)
set(BC_FILE ${OUTPUT_DIR}/test1_simple_instrumented.bc)
set(CSV_FILE ${OUTPUT_DIR}/bb_info.csv)
set(READABLE_LL ${OUTPUT_DIR}/test1_simple_instrumented.ll)

# Step 1: Compile source to LLVM IR
add_custom_command(
    OUTPUT ${LL_FILE}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm 
            ${SOURCE_FILE} -o ${LL_FILE}
    DEPENDS ${SOURCE_FILE}
    COMMENT "Compiling test1_simple.c to LLVM IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2: Run the pass
add_custom_command(
    OUTPUT ${BC_FILE}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label" ${LL_FILE} -o ${BC_FILE}
    DEPENDS ${LL_FILE} ${PASS_PLUGIN}
    COMMENT "Running ir-bb-label pass on test1_simple"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 3: Convert back to readable format
add_custom_command(
    OUTPUT ${READABLE_LL}
    COMMAND ${LLVM_DIS_EXECUTABLE} ${BC_FILE} -o ${READABLE_LL}
    DEPENDS ${BC_FILE}
    COMMENT "Converting test1_simple bitcode to readable IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Create target
add_custom_target(test1_simple_target ALL DEPENDS ${READABLE_LL})

# Add tests
add_test(
    NAME test1_simple
    COMMAND ${CMAKE_COMMAND} -E echo "Checking test1_simple results..."
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

add_test(
    NAME test1_simple_csv_exists
    COMMAND ${CMAKE_COMMAND} -E cat ${CSV_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test1_simple_csv_exists PROPERTIES 
    DEPENDS test1_simple
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

add_test(
    NAME test1_simple_csv_has_content
    COMMAND ${CMAKE_COMMAND} 
            -DCSV_FILE=${CSV_FILE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../common/verify_csv.cmake
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test1_simple_csv_has_content PROPERTIES DEPENDS test1_simple_csv_exists)
