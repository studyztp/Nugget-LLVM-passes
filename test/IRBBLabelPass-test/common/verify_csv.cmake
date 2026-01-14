# CMake script to verify CSV file has content beyond just the header

if(NOT EXISTS ${CSV_FILE})
    message(FATAL_ERROR "CSV file does not exist: ${CSV_FILE}")
endif()

file(STRINGS ${CSV_FILE} CSV_LINES)
list(LENGTH CSV_LINES LINE_COUNT)

if(LINE_COUNT LESS 2)
    message(FATAL_ERROR "CSV file has no data rows (only header or empty). Lines: ${LINE_COUNT}")
endif()

message(STATUS "CSV file has ${LINE_COUNT} lines (including header)")
message(STATUS "✓ CSV verification passed")
