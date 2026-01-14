#!/bin/bash
# Script to capture optimization passes applied by opt and llc

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

# Run tool with pass printing and capture output
echo "Optimization passes applied by $TOOL with $OPTS:" > "$OUTPUT_FILE"
$TOOL $OPTS -print-before-all "$INPUT_FILE" -o /dev/null 2>&1 | grep -i "before" >> "$OUTPUT_FILE" || true

exit 0
