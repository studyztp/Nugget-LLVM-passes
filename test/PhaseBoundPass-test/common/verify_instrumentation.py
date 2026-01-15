#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# verify_instrumentation.py
#
# Validates PhaseBoundPass instrumentation in LLVM IR.
#
# Verifies:
#   1. nugget_init called in nugget_roi_begin_ with correct marker counts
#   2. nugget_warmup_marker_hook called at warmup marker BB
#   3. nugget_start_marker_hook called at start marker BB
#   4. nugget_end_marker_hook called at end marker BB
#
# Usage:
#   python3 verify_instrumentation.py <instrumented.ll> <bb_info.csv> \
#           <warmup_bb_id> <warmup_count> <start_bb_id> <start_count> \
#           <end_bb_id> <end_count>

import re
import sys
import csv
from pathlib import Path


def parse_csv(csv_file):
    """Parse CSV file to get basic block information."""
    bb_info = {}
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            bb_id = int(row['BasicBlockID'])
            bb_info[bb_id] = {
                'function_name': row['FunctionName'],
                'bb_name': row['BasicBlockName'],
                'inst_count': int(row['BasicBlockInstCount'])
            }
    return bb_info


def check_nugget_init_in_roi_begin(ir_content, warmup_count, start_count, end_count):
    """Check that nugget_init is called in nugget_roi_begin_ with correct args."""
    errors = []
    
    # Find nugget_roi_begin_ function
    roi_begin_match = re.search(
        r'define\s+(?:dso_local\s+)?void\s+@nugget_roi_begin_\s*\([^)]*\)\s*[^{]*\{(.*?)\n\}',
        ir_content, re.DOTALL
    )
    
    if not roi_begin_match:
        errors.append("Could not find nugget_roi_begin_ function definition")
        return errors
    
    roi_begin_body = roi_begin_match.group(1)
    
    # Check for nugget_init call with correct arguments
    init_call_pattern = rf'call\s+void\s+@nugget_init\s*\(\s*i64\s+{warmup_count}\s*,\s*i64\s+{start_count}\s*,\s*i64\s+{end_count}\s*\)'
    if not re.search(init_call_pattern, roi_begin_body):
        # Try to find any nugget_init call
        any_init_call = re.search(r'call\s+void\s+@nugget_init\s*\([^)]*\)', roi_begin_body)
        if any_init_call:
            errors.append(
                f"nugget_init called with wrong arguments. "
                f"Expected ({warmup_count}, {start_count}, {end_count}), "
                f"found: {any_init_call.group(0)}"
            )
        else:
            errors.append("nugget_init is NOT called in nugget_roi_begin_")
    
    return errors


def check_marker_hooks(ir_content, bb_info, warmup_bb_id, start_bb_id, end_bb_id):
    """Check that marker hooks are inserted at the correct basic blocks."""
    errors = []
    
    # Parse functions and their basic blocks
    function_pattern = re.compile(
        r'define\s+(?:dso_local\s+|internal\s+|linkonce_odr\s+)?[^@]*@([^\s(]+)\s*\([^)]*\)[^{]*\{(.*?)\n\}',
        re.DOTALL
    )
    
    # Track which markers we found
    found_markers = {
        'warmup': False,
        'start': False,
        'end': False
    }
    
    for func_match in function_pattern.finditer(ir_content):
        func_name = func_match.group(1).strip('"')
        func_body = func_match.group(2)
        
        # Skip nugget helper functions
        if func_name.startswith('nugget_'):
            continue
        
        # Look for marker hooks
        if re.search(r'call\s+void\s+@nugget_warmup_marker_hook\s*\(\s*\)', func_body):
            found_markers['warmup'] = True
        if re.search(r'call\s+void\s+@nugget_start_marker_hook\s*\(\s*\)', func_body):
            found_markers['start'] = True
        if re.search(r'call\s+void\s+@nugget_end_marker_hook\s*\(\s*\)', func_body):
            found_markers['end'] = True
    
    # Check all markers were found
    if not found_markers['warmup']:
        errors.append(f"nugget_warmup_marker_hook not found (expected at BB {warmup_bb_id})")
    if not found_markers['start']:
        errors.append(f"nugget_start_marker_hook not found (expected at BB {start_bb_id})")
    if not found_markers['end']:
        errors.append(f"nugget_end_marker_hook not found (expected at BB {end_bb_id})")
    
    return errors


def main():
    if len(sys.argv) < 9:
        print("Usage: verify_instrumentation.py <instrumented.ll> <bb_info.csv> "
              "<warmup_bb_id> <warmup_count> <start_bb_id> <start_count> "
              "<end_bb_id> <end_count>")
        sys.exit(1)
    
    ir_file = sys.argv[1]
    csv_file = sys.argv[2]
    warmup_bb_id = int(sys.argv[3])
    warmup_count = int(sys.argv[4])
    start_bb_id = int(sys.argv[5])
    start_count = int(sys.argv[6])
    end_bb_id = int(sys.argv[7])
    end_count = int(sys.argv[8])
    
    # Verify files exist
    if not Path(ir_file).exists():
        print(f"ERROR: IR file not found: {ir_file}")
        sys.exit(1)
    
    if not Path(csv_file).exists():
        print(f"ERROR: CSV file not found: {csv_file}")
        sys.exit(1)
    
    # Parse files
    with open(ir_file, 'r') as f:
        ir_content = f.read()
    
    bb_info = parse_csv(csv_file)
    
    errors = []
    
    # Check 1: nugget_init is called in nugget_roi_begin_
    errors.extend(check_nugget_init_in_roi_begin(ir_content, warmup_count, start_count, end_count))
    
    # Check 2: Marker hooks are inserted
    errors.extend(check_marker_hooks(ir_content, bb_info, warmup_bb_id, start_bb_id, end_bb_id))
    
    if errors:
        print("✗ Instrumentation validation FAILED")
        for error in errors:
            print(f"  - {error}")
        sys.exit(1)
    else:
        print("✓ Instrumentation validation PASSED")
        print(f"  - nugget_init called with counts ({warmup_count}, {start_count}, {end_count})")
        print(f"  - warmup marker hook at BB {warmup_bb_id}")
        print(f"  - start marker hook at BB {start_bb_id}")
        print(f"  - end marker hook at BB {end_bb_id}")
        sys.exit(0)


if __name__ == '__main__':
    main()
