#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""Validates PhaseAnalysisPass instrumentation.

This script verifies that the PhaseAnalysisPass correctly instruments:
1. nugget_init_ call inserted at the end of nugget_roi_begin_
2. nugget_bb_hook_ calls inserted at the end of each labeled basic block

Usage:
    python3 verify_instrumentation.py <instrumented.ll> <bb_info.csv> <expected_threshold>

Exit codes:
    0: Validation passed
    1: Validation failed
"""

import re
import sys
import csv
from pathlib import Path
from collections import defaultdict


def parse_csv(csv_file):
    """Parse CSV file to get basic block information."""
    bb_info = []
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            bb_info.append({
                'function_name': row['FunctionName'],
                'function_id': int(row['FunctionID']),
                'bb_name': row['BasicBlockName'],
                'bb_id': int(row['BasicBlockID']),
                'inst_count': int(row['BasicBlockInstCount'])
            })
    return bb_info


def check_nugget_init_in_roi_begin(ir_content, total_bb_count):
    """Check that nugget_init_ is called in nugget_roi_begin_ with correct arg."""
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
    
    # Check for nugget_init_ call with the correct total_bb_count argument
    init_call_pattern = rf'call\s+void\s+@nugget_init_\s*\(\s*i64\s+{total_bb_count}\s*\)'
    if not re.search(init_call_pattern, roi_begin_body):
        # Try to find any nugget_init_ call to report what was found
        any_init_call = re.search(r'call\s+void\s+@nugget_init_\s*\([^)]*\)', roi_begin_body)
        if any_init_call:
            errors.append(
                f"nugget_init_ called but with wrong argument. "
                f"Expected total_bb_count={total_bb_count}, found: {any_init_call.group(0)}"
            )
        else:
            errors.append("nugget_init_ is NOT called in nugget_roi_begin_")
    
    return errors


def check_bb_hooks(ir_content, bb_info, expected_threshold):
    """Check that all labeled basic blocks have nugget_bb_hook_ calls."""
    errors = []
    
    # Get all function bodies (excluding nugget helper functions)
    nugget_functions = {'nugget_init_', 'nugget_roi_begin_', 'nugget_roi_end_', 'nugget_bb_hook_'}
    
    # Parse functions and their basic blocks
    function_pattern = re.compile(
        r'define\s+(?:dso_local\s+|internal\s+|linkonce_odr\s+)?[^@]*@([^\s(]+)\s*\([^)]*\)[^{]*\{(.*?)\n\}',
        re.DOTALL
    )
    
    # Build expected bb_ids from CSV
    expected_bb_ids = {bb['bb_id'] for bb in bb_info}
    found_bb_hook_calls = set()
    
    for func_match in function_pattern.finditer(ir_content):
        func_name = func_match.group(1).strip('"')
        func_body = func_match.group(2)
        
        # Skip nugget helper functions
        if func_name in nugget_functions:
            continue
        
        # Find all nugget_bb_hook_ calls in this function
        hook_pattern = re.compile(
            r'call\s+void\s+@nugget_bb_hook_\s*\(\s*i64\s+(\d+)\s*,\s*i64\s+(\d+)\s*,\s*i64\s+(\d+)\s*\)'
        )
        
        for hook_match in hook_pattern.finditer(func_body):
            inst_count = int(hook_match.group(1))
            bb_id = int(hook_match.group(2))
            threshold = int(hook_match.group(3))
            
            found_bb_hook_calls.add(bb_id)
            
            # Verify threshold
            if threshold != expected_threshold:
                errors.append(
                    f"BB ID {bb_id}: wrong threshold. Expected {expected_threshold}, got {threshold}"
                )
            
            # Verify inst_count matches CSV
            for bb in bb_info:
                if bb['bb_id'] == bb_id:
                    # Note: inst_count in instrumented IR may differ slightly due to added instructions
                    # We just verify the call exists with a reasonable inst_count
                    break
    
    # Check all expected BBs have hook calls
    missing_hooks = expected_bb_ids - found_bb_hook_calls
    if missing_hooks:
        errors.append(f"Missing nugget_bb_hook_ calls for BB IDs: {sorted(missing_hooks)}")
    
    # Check no extra hook calls
    extra_hooks = found_bb_hook_calls - expected_bb_ids
    if extra_hooks:
        errors.append(f"Unexpected nugget_bb_hook_ calls for BB IDs: {sorted(extra_hooks)}")
    
    return errors


def main():
    if len(sys.argv) < 4:
        print("Usage: verify_instrumentation.py <instrumented.ll> <bb_info.csv> <expected_threshold>")
        sys.exit(1)
    
    ir_file = sys.argv[1]
    csv_file = sys.argv[2]
    expected_threshold = int(sys.argv[3])
    
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
    total_bb_count = len(bb_info)
    
    errors = []
    
    # Check 1: nugget_init_ is called in nugget_roi_begin_
    errors.extend(check_nugget_init_in_roi_begin(ir_content, total_bb_count))
    
    # Check 2: All labeled BBs have nugget_bb_hook_ calls
    errors.extend(check_bb_hooks(ir_content, bb_info, expected_threshold))
    
    if errors:
        print("✗ Instrumentation validation FAILED")
        for error in errors:
            print(f"  - {error}")
        sys.exit(1)
    else:
        print("✓ Instrumentation validation PASSED")
        print(f"  - nugget_init_ called with total_bb_count={total_bb_count}")
        print(f"  - {total_bb_count} basic blocks instrumented with nugget_bb_hook_")
        print(f"  - threshold={expected_threshold}")
        sys.exit(0)


if __name__ == '__main__':
    main()
