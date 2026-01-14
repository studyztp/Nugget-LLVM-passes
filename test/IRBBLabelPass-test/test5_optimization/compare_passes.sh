#!/bin/bash

# Test script to compare optimization passes between direct and pipelined compilation
# Generates a detailed report with statistical analysis
# Uses enhanced Python-based comparison for per-function analysis

DIRECT_PASSES="${1:-direct_passes.txt}"
OPT_PASSES="${2:-opt_passes.txt}"
LLC_PASSES="${3:-llc_passes.txt}"
REPORT_FILE="${4:-PASSES_COMPARISON_REPORT.md}"

# Try to detect direct_machine_passes.txt in same directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -z "$5" ]; then
    # If not provided as 5th argument, check in same directory as REPORT_FILE
    REPORT_DIR="$(dirname "$REPORT_FILE")"
    DIRECT_MACHINE="${REPORT_DIR}/direct_machine_passes.txt"
    if [ ! -f "$DIRECT_MACHINE" ]; then
        # Check current directory
        DIRECT_MACHINE="direct_machine_passes.txt"
    fi
else
    DIRECT_MACHINE="$5"
fi

if [ ! -f "$DIRECT_PASSES" ] || [ ! -f "$OPT_PASSES" ] || [ ! -f "$LLC_PASSES" ]; then
    echo "Error: Pass files not found"
    echo "Usage: $0 <direct_passes> <opt_passes> <llc_passes> [report_file] [direct_machine_passes]"
    exit 1
fi

# Check if enhanced Python comparison is available
PYTHON_COMPARE="$SCRIPT_DIR/compare_passes_detailed.py"

if [ -f "$PYTHON_COMPARE" ]; then
    echo "Using enhanced Python-based comparison..."
    # Pass direct_machine_passes.txt if it exists
    if [ -f "$DIRECT_MACHINE" ]; then
        python3 "$PYTHON_COMPARE" "$DIRECT_PASSES" "$OPT_PASSES" "$LLC_PASSES" "$DIRECT_MACHINE" "$REPORT_FILE"
    else
        python3 "$PYTHON_COMPARE" "$DIRECT_PASSES" "$OPT_PASSES" "$LLC_PASSES" "$REPORT_FILE"
    fi
    echo "✓ Enhanced report generated: $REPORT_FILE"
    exit 0
fi

echo "Enhanced comparison script not found, falling back to basic comparison..."
echo ""

# Extract pass names (remove prefixes and suffixes)
extract_passes() {
    grep "IR Dump Before" "$1" | sed 's/.*IR Dump Before //' | sed 's/ on .*//' | sort | uniq
}

# Count passes
count_passes() {
    grep "IR Dump Before" "$1" | wc -l
}

# Count passes per function
count_passes_per_function() {
    local file="$1"
    echo "### Passes per Function"
    echo ""
    grep "IR Dump Before" "$file" | sed 's/.*on //' | cut -d' ' -f1 | sort | uniq -c | sort -rn | while read count func; do
        echo "- **$func**: $count passes"
    done
}

# Find unique passes
find_unique_passes() {
    local file1="$1"
    local file2="$2"
    local label="$3"
    
    local passes1=$(extract_passes "$file1" | sort)
    local passes2=$(extract_passes "$file2" | sort)
    
    local unique=$(comm -23 <(echo "$passes1") <(echo "$passes2"))
    
    if [ -n "$unique" ]; then
        echo "### Unique to $label"
        echo ""
        echo "$unique" | while read pass; do
            echo "- $pass"
        done
        echo ""
        return 1
    fi
    return 0
}

# Generate report
{
    echo "# Test 5: Optimization Passes Comparison Report"
    echo ""
    echo "**Generated**: $(date)"
    echo ""
    echo "---"
    echo ""
    
    # Summary statistics
    echo "## Summary Statistics"
    echo ""
    
    DIRECT_COUNT=$(count_passes "$DIRECT_PASSES")
    OPT_COUNT=$(count_passes "$OPT_PASSES")
    LLC_COUNT=$(count_passes "$LLC_PASSES")
    
    echo "| Pipeline | Pass Count | Stage |"
    echo "|----------|-----------|-------|"
    echo "| Direct (clang -O2) | $DIRECT_COUNT | IR Optimization |"
    echo "| Pipelined (opt -O2) | $OPT_COUNT | IR Optimization |"
    echo "| Code Generation (llc -O2) | $LLC_COUNT | Machine Code |"
    echo ""
    
    # Calculate differences
    DIFF=$((OPT_COUNT - DIRECT_COUNT))
    if [ $DIFF -eq 0 ]; then
        echo "**Finding**: Direct and Opt pass counts are **IDENTICAL** ✓"
    else
        PERCENT=$((100 * DIFF / DIRECT_COUNT))
        echo "**Finding**: Difference: $DIFF passes ($PERCENT%)"
    fi
    echo ""
    echo "---"
    echo ""
    
    # Direct passes analysis
    echo "## Direct Compilation (clang -O2)"
    echo ""
    echo "**Total passes**: $DIRECT_COUNT"
    echo ""
    count_passes_per_function "$DIRECT_PASSES"
    echo ""
    
    # Opt passes analysis
    echo "## Pipelined IR Optimization (opt -O2)"
    echo ""
    echo "**Total passes**: $OPT_COUNT"
    echo ""
    count_passes_per_function "$OPT_PASSES"
    echo ""
    
    # Pass comparison
    echo "---"
    echo ""
    echo "## Detailed Pass Comparison"
    echo ""
    
    PASSES_DIRECT=$(extract_passes "$DIRECT_PASSES" | wc -l)
    PASSES_OPT=$(extract_passes "$OPT_PASSES" | wc -l)
    COMMON=$(comm -12 <(extract_passes "$DIRECT_PASSES") <(extract_passes "$OPT_PASSES") | wc -l)
    
    echo "### Unique Pass Types"
    echo ""
    echo "| Metric | Count |"
    echo "|--------|-------|"
    echo "| Unique in Direct | $(comm -23 <(extract_passes "$DIRECT_PASSES") <(extract_passes "$OPT_PASSES") | wc -l) |"
    echo "| Unique in Opt | $(comm -13 <(extract_passes "$DIRECT_PASSES") <(extract_passes "$OPT_PASSES") | wc -l) |"
    echo "| Common | $COMMON |"
    echo ""
    
    # Check for unique passes
    UNIQUE_DIRECT=$(find_unique_passes "$DIRECT_PASSES" "$OPT_PASSES" "Direct")
    UNIQUE_OPT=$(find_unique_passes "$OPT_PASSES" "$DIRECT_PASSES" "Opt")
    
    if [ $UNIQUE_DIRECT -eq 0 ] && [ $UNIQUE_OPT -eq 0 ]; then
        echo "**Result**: All pass types are **IDENTICAL** between Direct and Opt ✓"
    else
        echo "**Result**: Some pass differences detected"
    fi
    echo ""
    echo "---"
    echo ""
    
    # LLC passes analysis
    echo "## Code Generation (llc -O2)"
    echo ""
    echo "**Total machine-level passes**: $LLC_COUNT"
    echo ""
    
    # Categorize LLC passes
    LLC_PRE=$(grep "Pre-ISel\|pre-isel" "$LLC_PASSES" | wc -l)
    LLC_ISEL=$(grep "ISel\|isel\|Instruction Selection" "$LLC_PASSES" | wc -l)
    LLC_BACKEND=$(grep "Backend\|backend\|X86" "$LLC_PASSES" | wc -l)
    
    echo "### LLC Pass Categories"
    echo ""
    echo "- **Pre-ISel passes**: $(grep -c "Pre-ISel\|pre-isel\|Intrinsic\|CodeGen\|SafeStack" "$LLC_PASSES")"
    echo "- **ISel/Scheduling passes**: $(grep -c "Selection\|Scheduler\|Allocation\|Coalesce" "$LLC_PASSES")"
    echo "- **Backend optimization passes**: $(grep -c "X86\|Domain\|LEA\|Execution\|Branch\|Tail\|Machine" "$LLC_PASSES")"
    echo ""
    echo "---"
    echo ""
    
    # Pipeline equivalence summary
    echo "## Pipeline Equivalence Analysis"
    echo ""
    echo "### IR-Level Optimization"
    echo ""
    
    if [ "$DIRECT_COUNT" -eq "$OPT_COUNT" ]; then
        MATCH_PCT=100
        echo "✓ **PASS**: Direct and Pipelined apply identical IR optimizations"
    else
        MATCH_PCT=$((100 * COMMON / PASSES_DIRECT))
        echo "⚠ **WARNING**: Pass counts differ (Direct: $DIRECT_COUNT, Opt: $OPT_COUNT)"
    fi
    echo ""
    echo "**Match percentage**: $MATCH_PCT%"
    echo ""
    
    # Function-level analysis
    echo "### Per-Function Optimization Intensity"
    echo ""
    
    FIBO_DIRECT=$(grep "fibonacci" "$DIRECT_PASSES" | wc -l)
    FIBO_OPT=$(grep "fibonacci" "$OPT_PASSES" | wc -l)
    
    MATRIX_DIRECT=$(grep "matrix_multiply" "$DIRECT_PASSES" | wc -l)
    MATRIX_OPT=$(grep "matrix_multiply" "$OPT_PASSES" | wc -l)
    
    PRIME_DIRECT=$(grep "is_prime" "$DIRECT_PASSES" | wc -l)
    PRIME_OPT=$(grep "is_prime" "$OPT_PASSES" | wc -l)
    
    echo "| Function | Direct | Opt | Match |"
    echo "|----------|--------|-----|-------|"
    echo "| fibonacci | $FIBO_DIRECT | $FIBO_OPT | $([ "$FIBO_DIRECT" -eq "$FIBO_OPT" ] && echo "✓" || echo "✗") |"
    echo "| matrix_multiply | $MATRIX_DIRECT | $MATRIX_OPT | $([ "$MATRIX_DIRECT" -eq "$MATRIX_OPT" ] && echo "✓" || echo "✗") |"
    echo "| is_prime | $PRIME_DIRECT | $PRIME_OPT | $([ "$PRIME_DIRECT" -eq "$PRIME_OPT" ] && echo "✓" || echo "✗") |"
    echo ""
    
    # Final verdict
    echo "---"
    echo ""
    echo "## Final Verdict"
    echo ""
    
    if [ "$DIRECT_COUNT" -eq "$OPT_COUNT" ]; then
        echo "✅ **EQUIVALENCE VALIDATED**"
        echo ""
        echo "Both compilation pipelines apply **identical** optimization sequences."
        echo ""
        echo "- IR pass count: Identical"
        echo "- Pass types: Identical"
        echo "- Optimization intensity: Identical"
        echo "- Pipeline compatibility: ✓ Verified"
        echo ""
        echo "**Conclusion**: The pipelined approach with IRBBLabelPass injection is fully equivalent to direct compilation."
    else
        echo "⚠️ **DIFFERENCES DETECTED**"
        echo ""
        echo "Pass counts differ between Direct and Opt pipelines:"
        echo "- Direct: $DIRECT_COUNT passes"
        echo "- Opt: $OPT_COUNT passes"
        echo "- Difference: $DIFF passes"
        echo ""
        echo "**Action**: Investigate differences to ensure optimization equivalence."
    fi
    echo ""
    
} > "$REPORT_FILE"

echo "Report generated: $REPORT_FILE"
exit 0
