#!/usr/bin/env python3
"""
Enhanced pass comparison script that analyzes per-function optimization passes
at both IR-level and machine-level.
"""

import re
import sys
from collections import defaultdict
from pathlib import Path

def parse_passes(filename):
    """Parse pass file and extract function-specific information."""
    functions = defaultdict(list)
    passes = defaultdict(int)
    
    try:
        with open(filename, 'r') as f:
            for line in f:
                # Skip header lines and empty lines
                if 'IR Dump Before' not in line and 'Dump Before' not in line:
                    continue
                
                # Extract pass name and function/block
                # Format can be:
                # *** IR Dump Before PassName (pass-id) ***
                # *** IR Dump Before PassName on FunctionName ***
                # # *** IR Dump Before PassName (pass-id) ***:
                
                match = re.search(r'Dump Before\s+([^(]+)\s*(?:\(([^)]+)\))?(?:\s+on\s+(.+?))?(?:\s+\*\*\*|:)?$', line)
                
                if match:
                    pass_name = match.group(1).strip()
                    pass_id = match.group(2)
                    func_name = match.group(3)
                    
                    # Try to extract function name from line if not found
                    if not func_name:
                        func_match = re.search(r'on\s+(\S+)', line)
                        if func_match:
                            func_name = func_match.group(1)
                        else:
                            func_match = re.search(r'on\s+(.+?)(?:\s+\*\*\*|:)?$', line)
                            if func_match:
                                func_name = func_match.group(1).strip()
                    
                    # Cleanup function name
                    if func_name:
                        func_name = func_name.strip().rstrip('*:').strip()
                    else:
                        func_name = "[module]"
                    
                    functions[func_name].append(pass_name)
                    passes[pass_name] += 1
    except FileNotFoundError:
        print(f"Error: File not found: {filename}", file=sys.stderr)
        return None, None
    
    return functions, passes

def generate_report(direct_passes_file, opt_passes_file, llc_passes_file, report_file, direct_machine_passes_file=None):
    """Generate comprehensive comparison report."""
    
    print(f"Parsing passes from files...")
    direct_funcs, direct_passes = parse_passes(direct_passes_file)
    opt_funcs, opt_passes = parse_passes(opt_passes_file)
    
    # Try to parse machine passes - they might have different format
    llc_funcs = None
    llc_passes = None
    if Path(llc_passes_file).exists():
        llc_funcs, llc_passes = parse_passes(llc_passes_file)
    
    # Parse direct machine passes if provided
    direct_machine_funcs = None
    direct_machine_passes = None
    if direct_machine_passes_file and Path(direct_machine_passes_file).exists():
        direct_machine_funcs, direct_machine_passes = parse_passes(direct_machine_passes_file)
    
    if not all([direct_funcs, opt_funcs]):
        print("Error: Could not parse pass files", file=sys.stderr)
        return False
    
    total_direct = sum(len(v) for v in direct_funcs.values())
    total_opt = sum(len(v) for v in opt_funcs.values())
    total_llc = sum(len(v) for v in llc_funcs.values()) if llc_funcs else 0
    total_direct_machine = sum(len(v) for v in direct_machine_funcs.values()) if direct_machine_funcs else 0
    
    # Count unique passes
    unique_direct = set(direct_passes.keys())
    unique_opt = set(opt_passes.keys())
    unique_llc = set(llc_passes.keys()) if llc_passes else set()
    unique_direct_machine = set(direct_machine_passes.keys()) if direct_machine_passes else set()
    
    common_direct_opt = unique_direct & unique_opt
    only_direct = unique_direct - unique_opt
    only_opt = unique_opt - unique_direct
    
    # Generate markdown report
    report = []
    report.append("# Test 5: Optimization Passes Comparison Report")
    report.append("")
    report.append(f"**Generated**: {Path(direct_passes_file).stat().st_mtime}")
    report.append("")
    report.append("---")
    report.append("")
    
    # Summary
    report.append("## Summary Statistics")
    report.append("")
    report.append("| Pipeline | Pass Count | Unique Types | Stage |")
    report.append("|----------|-----------|---------------|-------|")
    report.append(f"| Direct (clang -O2) | {total_direct} | {len(unique_direct)} | IR Optimization |")
    report.append(f"| Pipelined (opt -O2) | {total_opt} | {len(unique_opt)} | IR Optimization |")
    report.append(f"| Machine Code (llc -O2) | {total_llc} | {len(unique_llc)} | Machine Code |")
    if total_direct_machine > 0:
        report.append(f"| Direct Machine (clang -O2 → llc) | {total_direct_machine} | {len(unique_direct_machine)} | Machine Code |")
    report.append("")
    
    diff = total_opt - total_direct
    if diff == 0:
        report.append("**Finding**: Direct and Opt pass counts are **IDENTICAL** ✓")
    else:
        percent = (100 * diff) // total_direct if total_direct > 0 else 0
        report.append(f"**Finding**: IR-Level Difference: {diff} passes ({percent}%)")
    
    machine_diff = total_llc - total_direct
    if machine_diff == 0:
        report.append("**Machine-Level**: Direct and LLC pass counts are **IDENTICAL** ✓")
    else:
        report.append(f"**Machine-Level**: Difference: {machine_diff} passes vs Direct IR")
    
    report.append("")
    report.append("---")
    report.append("")
    
    # Direct IR passes per function
    report.append("## Direct Compilation (clang -O2) - IR Level")
    report.append("")
    report.append("**Total passes**: " + str(total_direct))
    report.append("")
    report.append("### Passes per Function")
    report.append("")
    
    for func, passes_list in sorted(direct_funcs.items(), key=lambda x: len(x[1]), reverse=True):
        report.append(f"- **{func}**: {len(passes_list)} passes")
    
    report.append("")
    
    # Opt IR passes per function
    report.append("## Pipelined IR Optimization (opt -O2) - IR Level")
    report.append("")
    report.append("**Total passes**: " + str(total_opt))
    report.append("")
    report.append("### Passes per Function")
    report.append("")
    
    for func, passes_list in sorted(opt_funcs.items(), key=lambda x: len(x[1]), reverse=True):
        report.append(f"- **{func}**: {len(passes_list)} passes")
    
    report.append("")
    report.append("---")
    report.append("")
    
    # Detailed per-function comparison
    report.append("## Detailed Per-Function Comparison (IR Level)")
    report.append("")
    
    all_functions = set(direct_funcs.keys()) | set(opt_funcs.keys())
    
    # Focus on key functions like matrix_multiply
    key_functions = [f for f in all_functions if any(x in f.lower() for x in ['matrix', 'main', 'compute', 'fibonacci', 'prime', 'count'])]
    
    for func in sorted(key_functions):
        direct_count = len(direct_funcs.get(func, []))
        opt_count = len(opt_funcs.get(func, []))
        
        report.append(f"### Function: `{func}`")
        report.append("")
        report.append(f"| Metric | Direct | Opt | Diff |")
        report.append("|--------|--------|-----|------|")
        report.append(f"| Pass Count | {direct_count} | {opt_count} | {opt_count - direct_count:+d} |")
        
        if direct_count > 0 or opt_count > 0:
            direct_set = set(direct_funcs.get(func, []))
            opt_set = set(opt_funcs.get(func, []))
            common = direct_set & opt_set
            only_direct = direct_set - opt_set
            only_opt = opt_set - direct_set
            
            report.append(f"| Unique Pass Types | {len(direct_set)} | {len(opt_set)} | - |")
            report.append(f"| Common Passes | {len(common)} | {len(common)} | - |")
            
            # Show differences if any
            if only_direct or only_opt:
                report.append("")
                report.append("#### Pass Differences:")
                if only_direct:
                    report.append(f"**Only in Direct** ({len(only_direct)}):")
                    for p in sorted(only_direct)[:5]:
                        report.append(f"  - {p}")
                    if len(only_direct) > 5:
                        report.append(f"  - ... and {len(only_direct) - 5} more")
                if only_opt:
                    report.append(f"**Only in Opt** ({len(only_opt)}):")
                    for p in sorted(only_opt)[:5]:
                        report.append(f"  - {p}")
                    if len(only_opt) > 5:
                        report.append(f"  - ... and {len(only_opt) - 5} more")
        
        report.append("")
    
    report.append("---")
    report.append("")
    
    # Detailed pass list for key functions
    report.append("## Detailed Pass Breakdown for Key Functions")
    report.append("")
    
    for func in sorted(key_functions):
        if func not in direct_funcs and func not in opt_funcs:
            continue
        
        direct_passes_list = direct_funcs.get(func, [])
        opt_passes_list = opt_funcs.get(func, [])
        
        if len(direct_passes_list) == 0 and len(opt_passes_list) == 0:
            continue
        
        report.append(f"### Passes Applied to `{func}`")
        report.append("")
        report.append("#### Direct (clang -O2) IR Optimization Pipeline")
        report.append("")
        report.append(f"Total: {len(direct_passes_list)} passes")
        report.append("")
        report.append("```")
        for i, p in enumerate(direct_passes_list[:20], 1):
            report.append(f"{i}. {p}")
        if len(direct_passes_list) > 20:
            report.append(f"... and {len(direct_passes_list) - 20} more passes")
        report.append("```")
        report.append("")
        
        report.append("#### Opt (-O2) IR Optimization Pipeline")
        report.append("")
        report.append(f"Total: {len(opt_passes_list)} passes")
        report.append("")
        report.append("```")
        for i, p in enumerate(opt_passes_list[:20], 1):
            report.append(f"{i}. {p}")
        if len(opt_passes_list) > 20:
            report.append(f"... and {len(opt_passes_list) - 20} more passes")
        report.append("```")
        report.append("")
    
    report.append("---")
    report.append("")
    
    # Machine code passes per function
    report.append("## Machine Code Optimization (llc -O2) - Machine Level")
    report.append("")
    report.append("**Total machine-level passes**: " + str(total_llc))
    report.append("")
    
    if llc_funcs and len(llc_funcs) > 0:
        report.append("### Machine Passes per Function/Entity")
        report.append("")
        
        for func, passes_list in sorted(llc_funcs.items(), key=lambda x: len(x[1]), reverse=True)[:15]:
            report.append(f"- **{func}**: {len(passes_list)} passes")
        report.append("")
    else:
        report.append("_Note: Machine-level passes from llc do not include function-specific information in standard output._")
        report.append("")
        report.append("To get per-function machine-level passes, run:")
        report.append("```bash")
        report.append("./extract_machine_passes_per_function.sh <optimized_ir.ll> machine_passes_per_func.txt")
        report.append("```")
        report.append("")
    
    # Direct Machine Passes Comparison
    if total_direct_machine > 0:
        report.append("---")
        report.append("")
        report.append("## Direct Machine Code Optimization (clang -O2 → llc -O2)")
        report.append("")
        report.append("**Total machine-level passes from direct compilation**: " + str(total_direct_machine))
        report.append("")
        
        # Compare direct machine passes with llc passes
        report.append("### Comparison: Direct Machine vs LLC Passes")
        report.append("")
        report.append("| Metric | Direct Machine | LLC | Diff |")
        report.append("|--------|--------|-----|------|")
        report.append(f"| Pass Count | {total_direct_machine} | {total_llc} | {total_direct_machine - total_llc:+d} |")
        report.append(f"| Unique Pass Types | {len(unique_direct_machine)} | {len(unique_llc)} | {len(unique_direct_machine) - len(unique_llc):+d} |")
        
        if unique_direct_machine and unique_llc:
            common_machine = unique_direct_machine & unique_llc
            only_direct_machine = unique_direct_machine - unique_llc
            only_llc = unique_llc - unique_direct_machine
            report.append(f"| Common Passes | {len(common_machine)} | {len(common_machine)} | - |")
            report.append("")
            
            # Show machine pass differences
            if only_direct_machine or only_llc:
                report.append("#### Machine Pass Differences:")
                if only_direct_machine:
                    report.append(f"**Only in Direct Machine** ({len(only_direct_machine)}):")
                    for p in sorted(only_direct_machine)[:5]:
                        report.append(f"  - {p}")
                    if len(only_direct_machine) > 5:
                        report.append(f"  - ... and {len(only_direct_machine) - 5} more")
                if only_llc:
                    report.append(f"**Only in LLC** ({len(only_llc)}):")
                    for p in sorted(only_llc)[:5]:
                        report.append(f"  - {p}")
                    if len(only_llc) > 5:
                        report.append(f"  - ... and {len(only_llc) - 5} more")
                report.append("")
        
        if direct_machine_funcs and len(direct_machine_funcs) > 0:
            report.append("### Direct Machine Passes per Function/Entity")
            report.append("")
            
            for func, passes_list in sorted(direct_machine_funcs.items(), key=lambda x: len(x[1]), reverse=True)[:15]:
                report.append(f"- **{func}**: {len(passes_list)} passes")
            report.append("")
    
    report.append("---")
    report.append("")
    
    # Unique pass types
    report.append("---")
    report.append("")
    report.append("## Unique Pass Types Analysis (IR Level)")
    report.append("")
    
    report.append("### Unique Pass Types")
    report.append("")
    report.append("| Category | Count |")
    report.append("|----------|-------|")
    report.append(f"| Unique in Direct | {len(only_direct)} |")
    report.append(f"| Unique in Opt | {len(only_opt)} |")
    report.append(f"| Common | {len(common_direct_opt)} |")
    report.append("")
    
    if only_direct:
        report.append("### Passes Only in Direct Compilation")
        report.append("")
        for p in sorted(only_direct):
            report.append(f"- {p}")
        report.append("")
    
    if only_opt:
        report.append("### Passes Only in Opt Pipeline")
        report.append("")
        for p in sorted(only_opt):
            report.append(f"- {p}")
        report.append("")
    
    # Pipeline equivalence
    report.append("---")
    report.append("")
    report.append("## Pipeline Equivalence Analysis")
    report.append("")
    
    if total_direct == total_opt and len(only_direct) == 0 and len(only_opt) == 0:
        report.append("✓ **IR-Level Optimization**: Direct and Opt pipelines are **EQUIVALENT**")
    else:
        report.append("⚠ **IR-Level Optimization**: Pass counts differ (Direct: {}, Opt: {})".format(total_direct, total_opt))
        match_percent = (len(common_direct_opt) * 100) // max(len(unique_direct), len(unique_opt)) if max(len(unique_direct), len(unique_opt)) > 0 else 0
        report.append("")
        report.append(f"**Match percentage**: {match_percent}%")
    
    report.append("")
    
    # Per-function comparison
    report.append("### Per-Function Optimization Intensity")
    report.append("")
    report.append("| Function | Direct | Opt | Match |")
    report.append("|----------|--------|-----|-------|")
    
    for func in sorted(key_functions):
        direct_count = len(direct_funcs.get(func, []))
        opt_count = len(opt_funcs.get(func, []))
        match = "✓" if direct_count == opt_count else "✗"
        report.append(f"| {func} | {direct_count} | {opt_count} | {match} |")
    
    report.append("")
    
    # Final verdict
    report.append("---")
    report.append("")
    report.append("## Final Verdict")
    report.append("")
    
    if total_direct == total_opt and len(only_direct) == 0 and len(only_opt) == 0:
        report.append("✅ **PASS**: Direct and Opt pipelines apply equivalent optimizations")
    else:
        report.append("⚠️ **DIFFERENCES DETECTED**")
        report.append("")
        report.append(f"Pass counts differ between Direct and Opt pipelines:")
        report.append(f"- Direct: {total_direct} passes")
        report.append(f"- Opt: {total_opt} passes")
        report.append(f"- Difference: {diff} passes")
        report.append("")
        report.append("**Action**: Investigate differences to ensure optimization equivalence.")
    
    # Write report
    report_content = "\n".join(report)
    
    try:
        with open(report_file, 'w') as f:
            f.write(report_content)
        print(f"✓ Report generated: {report_file}")
        return True
    except Exception as e:
        print(f"Error writing report: {e}", file=sys.stderr)
        return False

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: compare_passes_detailed.py <direct_passes> <opt_passes> <llc_passes> [direct_machine_passes] [report_file]")
        print("")
        print("Example:")
        print("  python3 compare_passes_detailed.py direct_passes.txt opt_passes.txt llc_passes.txt direct_machine_passes.txt PASSES_COMPARISON_REPORT.md")
        sys.exit(1)
    
    direct_file = sys.argv[1]
    opt_file = sys.argv[2]
    llc_file = sys.argv[3]
    
    # Handle optional parameters
    direct_machine_file = None
    report_file = "PASSES_COMPARISON_REPORT.md"
    
    if len(sys.argv) > 4:
        # Check if 4th argument is a file (direct_machine_passes)
        if sys.argv[4].endswith('.txt') and not sys.argv[4].endswith('REPORT.md'):
            direct_machine_file = sys.argv[4]
            report_file = sys.argv[5] if len(sys.argv) > 5 else "PASSES_COMPARISON_REPORT.md"
        else:
            report_file = sys.argv[4]
    
    # Try to auto-detect direct_machine_passes.txt in same directory if not provided
    if not direct_machine_file:
        from pathlib import Path
        direct_dir = Path(direct_file).parent
        candidate = direct_dir / "direct_machine_passes.txt"
        if candidate.exists():
            direct_machine_file = str(candidate)
    
    success = generate_report(direct_file, opt_file, llc_file, report_file, direct_machine_file)
    sys.exit(0 if success else 1)
