# expect_fail.cmake
#
# Runs a command that is expected to fail (including via signal/abort) and
# verifies the expected error message appears in its output.
#
# Required variables (pass via -D on the cmake command line):
#   CMD            - semicolon-separated command and arguments
#   EXPECTED_ERROR - substring that must appear in stdout or stderr
#
# Optional variables:
#   PASS_ARG       - a single argument that may contain semicolons (e.g. -passes=...)
#                    passed as a quoted single arg to avoid CMake list splitting
#   EXTRA_ARGS     - additional semicolon-separated arguments appended after PASS_ARG
#
# Usage:
#   cmake -DCMD="opt;-load-pass-plugin=..." \
#         -DPASS_ARG="-passes=phase-bound-pass<a=1;b=2>" \
#         -DEXTRA_ARGS="input.bc;-o;output.bc" \
#         -DEXPECTED_ERROR="some error" \
#         -P expect_fail.cmake

if(NOT DEFINED CMD)
    message(FATAL_ERROR "CMD is required")
endif()
if(NOT DEFINED EXPECTED_ERROR)
    message(FATAL_ERROR "EXPECTED_ERROR is required")
endif()

if(DEFINED PASS_ARG AND DEFINED EXTRA_ARGS)
    execute_process(
        COMMAND ${CMD} "${PASS_ARG}" ${EXTRA_ARGS}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout_output
        ERROR_VARIABLE stderr_output
    )
elseif(DEFINED PASS_ARG)
    execute_process(
        COMMAND ${CMD} "${PASS_ARG}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout_output
        ERROR_VARIABLE stderr_output
    )
else()
    execute_process(
        COMMAND ${CMD}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout_output
        ERROR_VARIABLE stderr_output
    )
endif()

if(result EQUAL 0)
    message(FATAL_ERROR
        "Command succeeded (exit 0) but was expected to fail.\n"
        "stdout: ${stdout_output}\n"
        "stderr: ${stderr_output}")
endif()

set(combined "${stdout_output}${stderr_output}")
string(FIND "${combined}" "${EXPECTED_ERROR}" pos)
if(pos EQUAL -1)
    message(FATAL_ERROR
        "Command failed as expected, but the expected error message was not found.\n"
        "Expected: ${EXPECTED_ERROR}\n"
        "stdout: ${stdout_output}\n"
        "stderr: ${stderr_output}")
endif()

message(STATUS "OK: Command failed with expected message: ${EXPECTED_ERROR}")
