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


def check_marker_hooks(ir_content, bb_info, warmup_bb_id, warmup_count,
                       start_bb_id, end_bb_id):
    """Check that marker hooks are inserted at the correct basic blocks.

    Parses !bb.id metadata to verify each hook is in the BB with the expected ID,
    not merely present somewhere in the module. When warmup_count is 0, the
    warmup marker hook is expected to be absent.
    """
    errors = []

    # Build metadata reference map: !N -> bb_id string
    metadata_map = {}
    for match in re.finditer(r'!(\d+)\s*=\s*!\{!"(\d+)"\}', ir_content):
        metadata_map[match.group(1)] = int(match.group(2))

    # Map each marker hook name to its expected bb_id
    # Skip warmup marker when warmup_count is 0
    marker_hooks = {
        'nugget_start_marker_hook': ('start', start_bb_id),
        'nugget_end_marker_hook': ('end', end_bb_id),
    }
    if warmup_count > 0:
        marker_hooks['nugget_warmup_marker_hook'] = ('warmup', warmup_bb_id)

    # Track which markers were found and at which bb_id
    found_markers = {name: None for name in marker_hooks}

    # Walk through the IR line by line, tracking the current BB's !bb.id
    in_function = False
    current_func = None
    # Accumulate lines per BB so we can find the !bb.id on the terminator
    bb_lines = []
    bb_id_for_block = None

    func_pattern = re.compile(
        r'define\s+(?:dso_local\s+|internal\s+|linkonce_odr\s+|available_externally\s+|private\s+)?'
        r'.*?\s+@([^\s(]+)\s*\(')
    bb_label_pattern = re.compile(r'^([a-zA-Z0-9_.$-]+):\s*(?:;.*)?$')

    def flush_bb(lines, bb_id):
        """Check accumulated BB lines for marker hooks; record their bb_id."""
        for line in lines:
            for hook_name in marker_hooks:
                if re.search(rf'call\s+void\s+@{hook_name}\s*\(\s*\)', line):
                    found_markers[hook_name] = bb_id

    lines = ir_content.split('\n')
    for line in lines:
        # Detect function entry
        fm = func_pattern.match(line)
        if fm:
            func_name = fm.group(1).strip('"')
            if func_name.startswith('nugget_'):
                in_function = False
                continue
            in_function = True
            current_func = func_name
            bb_lines = []
            bb_id_for_block = None
            continue

        if not in_function:
            continue

        # Detect function exit
        if line.strip() == '}':
            flush_bb(bb_lines, bb_id_for_block)
            in_function = False
            bb_lines = []
            bb_id_for_block = None
            continue

        # Detect new BB label -> flush previous BB
        bm = bb_label_pattern.match(line)
        if bm:
            flush_bb(bb_lines, bb_id_for_block)
            bb_lines = []
            bb_id_for_block = None
            continue

        bb_lines.append(line)

        # If this line has !bb.id, record the bb_id for the current block
        md_ref = re.search(r'!bb\.id\s*!(\d+)', line)
        if md_ref:
            ref_num = md_ref.group(1)
            if ref_num in metadata_map:
                bb_id_for_block = metadata_map[ref_num]

    # Validate each marker was found at the correct BB
    for hook_name, (label, expected_bb_id) in marker_hooks.items():
        actual_bb_id = found_markers[hook_name]
        if actual_bb_id is None:
            errors.append(f"{hook_name} not found (expected at BB {expected_bb_id})")
        elif actual_bb_id != expected_bb_id:
            errors.append(
                f"{hook_name} found at BB {actual_bb_id}, "
                f"expected at BB {expected_bb_id}")

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
    errors.extend(check_marker_hooks(ir_content, bb_info, warmup_bb_id, warmup_count,
                                     start_bb_id, end_bb_id))
    
    if errors:
        print("✗ Instrumentation validation FAILED")
        for error in errors:
            print(f"  - {error}")
        sys.exit(1)
    else:
        print("✓ Instrumentation validation PASSED")
        print(f"  - nugget_init called with counts ({warmup_count}, {start_count}, {end_count})")
        if warmup_count > 0:
            print(f"  - warmup marker hook at BB {warmup_bb_id}")
        else:
            print(f"  - warmup marker skipped (count=0)")
        print(f"  - start marker hook at BB {start_bb_id}")
        print(f"  - end marker hook at BB {end_bb_id}")
        sys.exit(0)


if __name__ == '__main__':
    main()
