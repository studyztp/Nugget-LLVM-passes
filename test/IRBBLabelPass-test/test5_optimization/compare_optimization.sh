#!/bin/bash
# Script to run optimization pipeline test and compare results

if [ $# -lt 2 ]; then
    echo "Usage: $0 <direct_binary> <optimized_binary> [iterations]"
    echo "  direct_binary: binary compiled directly with -O2 (absolute or relative path)"
    echo "  optimized_binary: binary compiled through optimization pipeline"
    echo "  iterations: number of times to run each (default: 5)"
    echo ""
    echo "Example from test5_optimization directory:"
    echo "  bash ../../compare_optimization.sh ./test5_direct ./test5_optimized 5"
    exit 1
fi

DIRECT_BIN="$1"
OPTIMIZED_BIN="$2"
ITERATIONS=${3:-5}

# Convert to absolute paths if needed
if [[ ! "$DIRECT_BIN" = /* ]]; then
    DIRECT_BIN="$(pwd)/$DIRECT_BIN"
fi

if [[ ! "$OPTIMIZED_BIN" = /* ]]; then
    OPTIMIZED_BIN="$(pwd)/$OPTIMIZED_BIN"
fi

if [ ! -x "$DIRECT_BIN" ]; then
    echo "Error: Direct binary not found or not executable: $DIRECT_BIN"
    exit 1
fi

if [ ! -x "$OPTIMIZED_BIN" ]; then
    echo "Error: Optimized binary not found or not executable: $OPTIMIZED_BIN"
    exit 1
fi

echo "=========================================="
echo "Optimization Pipeline Comparison Test"
echo "=========================================="
echo ""
echo "Direct binary: $DIRECT_BIN"
echo "Optimized binary: $OPTIMIZED_BIN"
echo "Iterations: $ITERATIONS"
echo ""

# Run direct compilation version
echo "Running direct compilation (with -O2)..."
DIRECT_TIMES=()
DIRECT_TOTAL=0
for ((i=1; i<=ITERATIONS; i++)); do
    START=$(date +%s%N)
    "$DIRECT_BIN" > /dev/null
    END=$(date +%s%N)
    ELAPSED=$((($END - $START) / 1000000))  # Convert to milliseconds
    DIRECT_TIMES+=($ELAPSED)
    DIRECT_TOTAL=$((DIRECT_TOTAL + ELAPSED))
    printf "  Run %d: %d ms\n" $i $ELAPSED
done

DIRECT_AVG=$((DIRECT_TOTAL / ITERATIONS))
echo "Average time (direct): $DIRECT_AVG ms"
echo ""

# Run optimization pipeline version
echo "Running optimization pipeline version..."
OPTIMIZED_TIMES=()
OPTIMIZED_TOTAL=0
for ((i=1; i<=ITERATIONS; i++)); do
    START=$(date +%s%N)
    "$OPTIMIZED_BIN" > /dev/null
    END=$(date +%s%N)
    ELAPSED=$((($END - $START) / 1000000))  # Convert to milliseconds
    OPTIMIZED_TIMES+=($ELAPSED)
    OPTIMIZED_TOTAL=$((OPTIMIZED_TOTAL + ELAPSED))
    printf "  Run %d: %d ms\n" $i $ELAPSED
done

OPTIMIZED_AVG=$((OPTIMIZED_TOTAL / ITERATIONS))
echo "Average time (optimized pipeline): $OPTIMIZED_AVG ms"
echo ""

# Calculate difference
DIFF=$((OPTIMIZED_AVG - DIRECT_AVG))
PERCENT_DIFF=$(( (DIFF * 100) / DIRECT_AVG ))

echo "=========================================="
echo "Performance Comparison"
echo "=========================================="
echo "Direct compilation avg:        $DIRECT_AVG ms"
echo "Optimized pipeline avg:        $OPTIMIZED_AVG ms"
echo "Difference:                    $DIFF ms"
echo "Percentage difference:         $PERCENT_DIFF %"
echo ""

if [ $OPTIMIZED_AVG -lt $DIRECT_AVG ]; then
    echo "✓ Optimized pipeline is FASTER"
elif [ $OPTIMIZED_AVG -eq $DIRECT_AVG ]; then
    echo "✓ Both pipelines have EQUAL performance"
else
    echo "✗ Optimized pipeline is SLOWER"
fi

echo "=========================================="
