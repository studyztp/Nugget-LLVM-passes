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

# Function to run binary and extract total CPU time from its output
# The binary prints "Time: X.XXXXXX seconds" for each test - we sum them
get_cpu_time() {
    local binary="$1"
    local total_us=0
    
    # Run binary and capture output
    local output
    output=$("$binary" 2>&1)
    
    # Extract all "Time: X.XXXXXX seconds" lines and sum them
    # Convert to microseconds for integer arithmetic
    while IFS= read -r line; do
        if [[ "$line" =~ Time:\ ([0-9]+)\.([0-9]+)\ seconds ]]; then
            local secs="${BASH_REMATCH[1]}"
            local frac="${BASH_REMATCH[2]}"
            # Pad/truncate fraction to 6 digits (microseconds)
            frac=$(printf "%-6s" "$frac" | tr ' ' '0' | cut -c1-6)
            local us=$((10#$secs * 1000000 + 10#$frac))
            total_us=$((total_us + us))
        fi
    done <<< "$output"
    
    echo "$total_us"
}

# Run multiple iterations and average the CPU time
time_binary() {
    local binary="$1"
    local total=0
    
    for i in $(seq 1 $ITERATIONS); do
        local cpu_us
        cpu_us=$(get_cpu_time "$binary")
        total=$((total + cpu_us))
    done
    
    # Return average in microseconds
    echo $((total / ITERATIONS))
}

echo "Running direct compilation binary ($ITERATIONS iterations)..." | tee -a "$LOG_FILE"
DIRECT_AVG=$(time_binary "$DIRECT_BIN")

echo "Running optimized pipeline binary ($ITERATIONS iterations)..." | tee -a "$LOG_FILE"
OPTIMIZED_AVG=$(time_binary "$OPTIMIZED_BIN")

# Convert microseconds to human-readable format
format_time() {
    local us=$1
    local sign=""
    
    # Handle negative values
    if [ $us -lt 0 ]; then
        sign="-"
        us=$((-us))
    fi
    
    if [ $us -ge 1000000 ]; then
        # Show in seconds with 3 decimal places
        local secs=$((us / 1000000))
        local frac=$(( (us % 1000000) / 1000 ))
        printf "%s%d.%03d s" "$sign" "$secs" "$frac"
    elif [ $us -ge 1000 ]; then
        # Show in milliseconds with 3 decimal places
        local ms=$((us / 1000))
        local frac=$((us % 1000))
        printf "%s%d.%03d ms" "$sign" "$ms" "$frac"
    else
        printf "%s%d µs" "$sign" "$us"
    fi
}

{
    echo ""
    echo "==========================================="
    echo "Results (CPU time measured inside binary):"
    echo "==========================================="
    echo "Direct compilation avg:     $(format_time $DIRECT_AVG) ($DIRECT_AVG µs)"
    echo "Optimized pipeline avg:     $(format_time $OPTIMIZED_AVG) ($OPTIMIZED_AVG µs)"
} >> "$LOG_FILE"

# Calculate difference in microseconds
DIFF=$((OPTIMIZED_AVG - DIRECT_AVG))
{
    echo "Difference:                 $(format_time $DIFF) ($DIFF µs)"
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
        # Allow up to 10% slowdown as acceptable
        if [ $PERCENT -le 10 ]; then
            echo "✓ Performance within acceptable range (≤10% slower)" | tee -a "$LOG_FILE"
        else
            echo "✗ FAILED: Optimized pipeline is significantly SLOWER (>10%)" | tee -a "$LOG_FILE"
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
if [ $PERCENT -le 10 ]; then
    exit 0
else
    exit 1
fi
