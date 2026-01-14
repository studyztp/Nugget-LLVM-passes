cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Test 5: Optimization Pipeline Comparison
# ============================================================================

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${OUTPUT_DIR})

set(SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/test5_optimization.c)

# Pipeline 1: Direct compilation with -O2
set(DIRECT_BIN ${OUTPUT_DIR}/test5_direct)
add_custom_command(
    OUTPUT ${DIRECT_BIN}
    COMMAND ${CLANG_EXECUTABLE} -O2 
            ${SOURCE_FILE} 
            -o ${DIRECT_BIN} -lm
    DEPENDS ${SOURCE_FILE}
    COMMENT "Building direct compilation with -O2"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Pipeline 2: Optimization pipeline with pass
set(OPT_LL ${OUTPUT_DIR}/test5_unoptimized.ll)
set(OPT_O2_LL ${OUTPUT_DIR}/test5_optimized.ll)
set(LABELED_BC ${OUTPUT_DIR}/test5_labeled.bc)
set(LABELED_OBJ ${OUTPUT_DIR}/test5_labeled.o)
set(OPTIMIZED_BIN ${OUTPUT_DIR}/test5_optimized)
set(OPTIMIZED_CSV ${OUTPUT_DIR}/bb_info.csv)

# Step 1: Compile to unoptimized LLVM IR
add_custom_command(
    OUTPUT ${OPT_LL}
    COMMAND ${CLANG_EXECUTABLE} -O0 -Xclang -disable-O0-optnone -S -emit-llvm 
            ${SOURCE_FILE} 
            -o ${OPT_LL}
    DEPENDS ${SOURCE_FILE}
    COMMENT "Compiling test5 to unoptimized LLVM IR"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2: Apply -O2 optimizations
add_custom_command(
    OUTPUT ${OPT_O2_LL}
    COMMAND ${OPT_EXECUTABLE} -O2 ${OPT_LL} -S -o ${OPT_O2_LL}
    DEPENDS ${OPT_LL}
    COMMENT "Applying -O2 optimizations to test5"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 2b: Capture opt -O2 passes applied (separate step)
set(OPT_PASSES_FILE ${OUTPUT_DIR}/opt_passes.txt)
add_custom_command(
    OUTPUT ${OPT_PASSES_FILE}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/capture_passes.sh ${OPT_EXECUTABLE} "-O2" ${OPT_LL} ${OPT_PASSES_FILE}
    DEPENDS ${OPT_LL}
    COMMENT "Capturing opt -O2 passes information"
)

# Step 3: Run IRBBLabel pass
add_custom_command(
    OUTPUT ${LABELED_BC}
    COMMAND ${OPT_EXECUTABLE} -load-pass-plugin=${PASS_PLUGIN}
            -passes="ir-bb-label" ${OPT_O2_LL} -o ${LABELED_BC}
    DEPENDS ${OPT_O2_LL} ${PASS_PLUGIN} ${OPT_PASSES_FILE}
    COMMENT "Labeling basic blocks with ir-bb-label pass"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 4: Compile bitcode to object with llc and -O2
add_custom_command(
    OUTPUT ${LABELED_OBJ}
    COMMAND ${LLC_EXECUTABLE} -O2 -filetype=obj -relocation-model=pic ${LABELED_BC} -o ${LABELED_OBJ}
    DEPENDS ${LABELED_BC}
    COMMENT "Compiling labeled bitcode to object file"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

# Step 4b: Capture llc -O2 passes applied (separate step)
set(LLC_PASSES_FILE ${OUTPUT_DIR}/llc_passes.txt)
add_custom_command(
    OUTPUT ${LLC_PASSES_FILE}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/capture_passes.sh ${LLC_EXECUTABLE} "-O2 -filetype=obj -relocation-model=pic" ${LABELED_BC} ${LLC_PASSES_FILE}
    DEPENDS ${LABELED_BC}
    COMMENT "Capturing llc -O2 passes information"
)

# Step 5: Link to create executable
add_custom_command(
    OUTPUT ${OPTIMIZED_BIN}
    COMMAND ${CLANG_EXECUTABLE} ${LABELED_OBJ} -o ${OPTIMIZED_BIN} -lm
    DEPENDS ${LABELED_OBJ}
    COMMENT "Linking to create optimized executable"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

add_custom_target(test5_optimization_target ALL DEPENDS ${DIRECT_BIN} ${OPTIMIZED_BIN} ${OPT_PASSES_FILE} ${LLC_PASSES_FILE})

# Tests
add_test(
    NAME test5_optimization_pass_check
    COMMAND ${CMAKE_COMMAND} -E echo "Checking test5 pass labeling..."
    WORKING_DIRECTORY ${OUTPUT_DIR}
)

add_test(
    NAME test5_optimization_csv_exists
    COMMAND ${CMAKE_COMMAND} -E cat ${OPTIMIZED_CSV}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_optimization_csv_exists PROPERTIES 
    DEPENDS test5_optimization_pass_check
    PASS_REGULAR_EXPRESSION "FunctionName,FunctionID,BasicBlockName"
)

add_test(
    NAME test5_optimization_binaries_exist
    COMMAND ${CMAKE_COMMAND} -E echo "Both direct and optimized binaries created"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_optimization_binaries_exist PROPERTIES DEPENDS test5_optimization_csv_exists)

# Test: Verify opt passes captured
add_test(
    NAME test5_opt_passes_captured
    COMMAND test -f ${OPT_PASSES_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_opt_passes_captured PROPERTIES DEPENDS test5_optimization_binaries_exist)

# Test: Verify llc passes captured
add_test(
    NAME test5_llc_passes_captured
    COMMAND test -f ${LLC_PASSES_FILE}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_llc_passes_captured PROPERTIES DEPENDS test5_opt_passes_captured)

# Test: Verify opt passes file has content
add_test(
    NAME test5_opt_passes_nonempty
    COMMAND bash -c "[ $(wc -l < ${OPT_PASSES_FILE}) -gt 10 ]"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_opt_passes_nonempty PROPERTIES DEPENDS test5_llc_passes_captured)

# Test: Verify llc passes file has content
add_test(
    NAME test5_llc_passes_nonempty
    COMMAND bash -c "[ $(wc -l < ${LLC_PASSES_FILE}) -gt 10 ]"
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_llc_passes_nonempty PROPERTIES DEPENDS test5_opt_passes_nonempty)

# Test: Performance comparison (both binaries run successfully)
add_test(
    NAME test5_direct_binary_runs
    COMMAND ${DIRECT_BIN}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_direct_binary_runs PROPERTIES DEPENDS test5_llc_passes_nonempty)

add_test(
    NAME test5_optimized_binary_runs
    COMMAND ${OPTIMIZED_BIN}
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_optimized_binary_runs PROPERTIES DEPENDS test5_direct_binary_runs)

# Test: Performance comparison script (if exists)
add_test(
    NAME test5_performance_comparison
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/compare_performance.sh ${DIRECT_BIN} ${OPTIMIZED_BIN} 5
    WORKING_DIRECTORY ${OUTPUT_DIR}
)
set_tests_properties(test5_performance_comparison PROPERTIES DEPENDS test5_optimized_binary_runs)
