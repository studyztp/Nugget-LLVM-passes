#!/bin/bash
# Script to capture optimization passes applied by clang, opt, and llc

TOOL="$1"
OPTS="$2"
INPUT_FILE="$3"
OUTPUT_FILE="$4"

if [ -z "$TOOL" ] || [ -z "$OPTS" ] || [ -z "$INPUT_FILE" ] || [ -z "$OUTPUT_FILE" ]; then
    echo "Usage: $0 <tool> <opts> <input_file> <output_file>"
    exit 1
fi

# Create output directory if needed
mkdir -p "$(dirname "$OUTPUT_FILE")"

# Determine tool type and capture passes accordingly
TOOL_NAME=$(basename "$TOOL")

if [[ "$TOOL_NAME" == *"clang"* ]]; then
    # For clang: compile to IR first, then capture passes with opt -O2
    # This shows the actual optimization pipeline that clang applies
    TEMP_IR=$(mktemp --suffix=.ll)
    CLANG_OPTS=$(echo "$OPTS" | sed 's/-lm//g' | sed 's/-l[^ ]*//g')
    
    echo "Optimization passes applied by clang with $OPTS:" > "$OUTPUT_FILE"
    # Compile to unoptimized IR
    $TOOL -O0 -Xclang -disable-O0-optnone -S -emit-llvm $CLANG_OPTS "$INPUT_FILE" -o "$TEMP_IR" 2>/dev/null
    
    # Now capture the opt -O2 passes that would be applied
    if [ -f "$TEMP_IR" ]; then
        opt -O2 -print-before-all "$TEMP_IR" -o /dev/null 2>&1 | grep "IR Dump Before" >> "$OUTPUT_FILE" || true
        rm -f "$TEMP_IR"
    fi
elif [[ "$TOOL_NAME" == "opt" ]]; then
    # For opt: use -print-before-all
    echo "Optimization passes applied by opt with $OPTS:" > "$OUTPUT_FILE"
    $TOOL $OPTS -print-before-all "$INPUT_FILE" -o /dev/null 2>&1 | grep "IR Dump Before" >> "$OUTPUT_FILE" || true
elif [[ "$TOOL_NAME" == "llc" ]]; then
    # For llc: use -print-before-all
    echo "Optimization passes applied by llc with $OPTS:" > "$OUTPUT_FILE"
    $TOOL $OPTS -print-before-all "$INPUT_FILE" -o /dev/null 2>&1 | grep "IR Dump Before" >> "$OUTPUT_FILE" || true
else
    echo "Unknown tool: $TOOL_NAME" > "$OUTPUT_FILE"
    exit 1
fi

exit 0
