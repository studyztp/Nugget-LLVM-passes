#!/bin/bash

# Performance comparison script for test5_optimization
# Compares runtime of direct vs optimized compilation binaries
# Stores results to performance_results.log

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <direct_binary> <optimized_binary> [iterations] [log_file]"
    exit 1
fi

DIRECT_BIN="$1"
OPTIMIZED_BIN="$2"
ITERATIONS="${3:-5}"
LOG_FILE="${4:-performance_results.log}"

if [ ! -x "$DIRECT_BIN" ]; then
    echo "Error: Direct binary not found or not executable: $DIRECT_BIN" | tee -a "$LOG_FILE"
    exit 1
fi

if [ ! -x "$OPTIMIZED_BIN" ]; then
    echo "Error: Optimized binary not found or not executable: $OPTIMIZED_BIN" | tee -a "$LOG_FILE"
    exit 1
fi

# Initialize log file with timestamp
{
    echo "==========================================="
    echo "Optimization Pipeline Comparison Test"
    echo "Timestamp: $(date)"
    echo "==========================================="
    echo "Direct binary:   $DIRECT_BIN"
    echo "Optimized binary: $OPTIMIZED_BIN"
    echo "Iterations: $ITERATIONS"
    echo ""
} > "$LOG_FILE"

# Function to run and time a binary
time_binary() {
    local binary="$1"
    local total=0
    
    for i in $(seq 1 $ITERATIONS); do
        # Use time to measure real execution time in milliseconds
        local start=$(date +%s%N)
        "$binary" > /dev/null 2>&1
        local end=$(date +%s%N)
        local elapsed=$(( (end - start) / 1000000 ))
        total=$((total + elapsed))
    done
    
    echo $((total / ITERATIONS))
}

echo "Running direct compilation binary..." | tee -a "$LOG_FILE"
DIRECT_AVG=$(time_binary "$DIRECT_BIN")

echo "Running optimized pipeline binary..." | tee -a "$LOG_FILE"
OPTIMIZED_AVG=$(time_binary "$OPTIMIZED_BIN")

{
    echo ""
    echo "==========================================="
    echo "Results:"
    echo "==========================================="
    echo "Direct compilation avg:     ${DIRECT_AVG} ms"
    echo "Optimized pipeline avg:     ${OPTIMIZED_AVG} ms"
} >> "$LOG_FILE"

# Calculate difference
DIFF=$((OPTIMIZED_AVG - DIRECT_AVG))
{
    echo "Difference:                 ${DIFF} ms"
} >> "$LOG_FILE"

# Calculate percentage difference (avoid division by zero)
if [ $DIRECT_AVG -gt 0 ]; then
    PERCENT=$((100 * DIFF / DIRECT_AVG))
    {
        echo "Percentage difference:      ${PERCENT} %"
        echo ""
    } >> "$LOG_FILE"
    
    # Determine which is faster
    if [ $DIFF -eq 0 ]; then
        echo "✓ Both pipelines have EQUAL performance" | tee -a "$LOG_FILE"
    elif [ $DIFF -lt 0 ]; then
        echo "✓ Optimized pipeline is FASTER" | tee -a "$LOG_FILE"
    else
        # Allow up to 5% slowdown as acceptable
        if [ $PERCENT -le 5 ]; then
            echo "✓ Performance within acceptable range (≤5% slower)" | tee -a "$LOG_FILE"
        else
            echo "✗ FAILED: Optimized pipeline is significantly SLOWER (>5%)" | tee -a "$LOG_FILE"
        fi
    fi
else
    echo "Warning: Could not calculate percentage (direct time = 0)" | tee -a "$LOG_FILE"
fi

echo ""  | tee -a "$LOG_FILE"
echo "==========================================="  | tee -a "$LOG_FILE"

# Exit 0 if performance is acceptable (within 10% or faster)
if [ $DIRECT_AVG -eq 0 ] || [ $DIFF -le 0 ]; then
    exit 0
fi

PERCENT=$((100 * DIFF / DIRECT_AVG))
if [ $PERCENT -le 5 ]; then
    exit 0
else
    exit 1
fi
