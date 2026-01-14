#!/bin/bash

# Performance comparison script for test5_optimization
# Compares runtime of direct vs optimized compilation binaries

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <direct_binary> <optimized_binary> [iterations]"
    exit 1
fi

DIRECT_BIN="$1"
OPTIMIZED_BIN="$2"
ITERATIONS="${3:-5}"

if [ ! -x "$DIRECT_BIN" ]; then
    echo "Error: Direct binary not found or not executable: $DIRECT_BIN"
    exit 1
fi

if [ ! -x "$OPTIMIZED_BIN" ]; then
    echo "Error: Optimized binary not found or not executable: $OPTIMIZED_BIN"
    exit 1
fi

echo "==========================================="
echo "Optimization Pipeline Comparison Test"
echo "==========================================="
echo "Iterations: $ITERATIONS"
echo ""

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

echo "Running direct compilation binary..."
DIRECT_AVG=$(time_binary "$DIRECT_BIN")

echo "Running optimized pipeline binary..."
OPTIMIZED_AVG=$(time_binary "$OPTIMIZED_BIN")

echo ""
echo "==========================================="
echo "Results:"
echo "==========================================="
echo "Direct compilation avg:     ${DIRECT_AVG} ms"
echo "Optimized pipeline avg:     ${OPTIMIZED_AVG} ms"

# Calculate difference
DIFF=$((OPTIMIZED_AVG - DIRECT_AVG))
echo "Difference:                 ${DIFF} ms"

# Calculate percentage difference (avoid division by zero)
if [ $DIRECT_AVG -gt 0 ]; then
    PERCENT=$((100 * DIFF / DIRECT_AVG))
    echo "Percentage difference:      ${PERCENT} %"
    echo ""
    
    # Determine which is faster
    if [ $DIFF -eq 0 ]; then
        echo "✓ Both pipelines have EQUAL performance"
    elif [ $DIFF -lt 0 ]; then
        echo "✓ Optimized pipeline is FASTER"
    else
        # Allow up to 10% slowdown as acceptable
        if [ $PERCENT -le 10 ]; then
            echo "✓ Performance within acceptable range (≤10% slower)"
        else
            echo "✗ Warning: Optimized pipeline is significantly SLOWER (>10%)"
        fi
    fi
else
    echo "Warning: Could not calculate percentage (direct time = 0)"
fi

echo "==========================================="

# Exit 0 if performance is acceptable (within 10% or faster)
if [ $DIRECT_AVG -eq 0 ] || [ $DIFF -le 0 ]; then
    exit 0
fi

PERCENT=$((100 * DIFF / DIRECT_AVG))
if [ $PERCENT -le 10 ]; then
    exit 0
else
    exit 1
fi
