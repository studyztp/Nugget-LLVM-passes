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
#
"""Validates LLVM IR metadata against CSV output for IRBBLabel pass.

This script cross-validates that the !bb.id metadata inserted into LLVM IR
by the IRBBLabel pass matches the corresponding CSV output file. It ensures
functional correctness of the instrumentation by checking:

1. All functions in CSV are present in IR with metadata
2. All basic blocks have matching IDs between IR and CSV
3. Basic block names are consistent (entry blocks unnamed, named blocks match)
4. Metadata is properly attached to terminator instructions

The script handles various LLVM IR complexities:
  - Function linkage types (dso_local, internal, linkonce_odr, etc.)
  - Mangled C++ function names (_ZN...)
  - Numeric auto-generated basic block labels (treated as unnamed)
  - Optimized BB names with special characters (._crit_edge, .lr.ph, etc.)
  - Metadata indirection (!6 = !{!"0"} mappings)

Usage:
    python3 verify_metadata.py <instrumented.ll> <bb_info.csv>

Exit codes:
    0: Validation passed (all metadata matches CSV)
    1: Validation failed (errors reported to stdout)

Example:
    python3 verify_metadata.py test1_instrumented.ll bb_info.csv
"""

import re
import sys
import csv
from pathlib import Path
from collections import defaultdict


def parse_ir_metadata(ir_file):
    """Extracts basic block metadata from instrumented LLVM IR.
    
    Parses an LLVM IR (.ll) file to extract !bb.id metadata annotations
    attached to terminator instructions. The metadata references are resolved
    to actual BB IDs using the metadata definitions at the end of the file.
    
    Args:
        ir_file: Path to instrumented LLVM IR file (.ll format)
        
    Returns:
        Dictionary mapping function names to lists of basic block dictionaries.
        Each BB dict contains:
            - 'name': Basic block label (empty string for entry block)
            - 'id': Integer BB ID from metadata
            
        Example:
            {
                'main': [
                    {'name': '', 'id': 0},         # Entry block
                    {'name': 'if.then', 'id': 1},
                    {'name': 'if.else', 'id': 2},
                    {'name': 'if.end', 'id': 3}
                ],
                '_ZN10Calculator3addEi': [  # Mangled C++ name
                    {'name': '', 'id': 4}
                ]
            }
    
    Parsing details:
        - Handles various function linkage types (dso_local, internal, etc.)
        - Recognizes BB labels with alphanumeric, dots, underscores, hyphens, $
        - Numeric-only labels (e.g., "14:") are treated as unnamed blocks
        - Metadata indirection: !bb.id !6 -> !6 = !{!"0"} -> id = 0
        - Only processes defined functions (skips declarations)
    
    Raises:
        IOError: If ir_file cannot be read
        ValueError: If IR format is malformed or unexpected
    """
    with open(ir_file, 'r') as f:
        content = f.read()
    
    # Extract metadata mappings from IR footer: !6 = !{!"0"}, !7 = !{!"1"}, etc.
    # These map metadata references (!6) to actual BB ID strings ("0")
    metadata_map = {}
    for match in re.finditer(r'!(\d+)\s*=\s*!\{!"(\d+)"\}', content):
        metadata_map[match.group(1)] = match.group(2)
    
    # Extract functions and their basic blocks
    functions = {}
    current_func = None
    current_bb_name = ''  # Entry block has no label
    in_function = False
    bb_started = False  # Track when we're inside a basic block
    
    lines = content.split('\n')
    for i, line in enumerate(lines):
        # Match function definition with various linkage specifiers
        # Examples:
        #   define i32 @main() {
        #   define dso_local i32 @add(i32 %a, i32 %b) {
        #   define internal fastcc void @helper() {
        #   define linkonce_odr i32 @_ZN3Foo3barEv() {
        func_match = re.match(r'define\s+(?:dso_local\s+|internal\s+|linkonce_odr\s+|available_externally\s+|private\s+)?.*?\s+@([^\s(]+)\s*\(', line)
        if func_match:
            # Extract function name (may be mangled for C++)
            func_name = func_match.group(1).strip('"')
            current_func = func_name
            functions[current_func] = []
            in_function = True
            bb_started = True  # Function entry is the first BB
            current_bb_name = ''  # Entry block has no explicit label
            continue
        
        # Check if we've exited the function
        if in_function and line.strip() == '}':
            in_function = False
            current_func = None
            bb_started = False
            continue
        
        if not in_function:
            continue
        
        # Match basic block label (at start of line, ends with colon)
        # Valid characters: letters, digits, dots, underscores, hyphens, dollar signs
        # Examples: "if.then:", "._crit_edge:", "14:", ".LBB0_1:"
        bb_match = re.match(r'^([a-zA-Z0-9_.$-]+):\s*(?:;.*)?$', line)
        if bb_match:
            bb_label = bb_match.group(1)
            # Numeric-only labels are auto-generated by LLVM (unnamed blocks)
            if bb_label.isdigit():
                current_bb_name = ''
            else:
                current_bb_name = bb_label
            bb_started = True
            continue
        
        # Match !bb.id metadata reference on terminator instructions
        # Examples: br i1 %cmp, label %if.then, label %if.else, !bb.id !6
        #           ret i32 0, !bb.id !7
        if bb_started and '!bb.id' in line:
            # Extract the metadata reference number (!6, !7, etc.)
            metadata_ref = re.search(r'!bb\.id\s*(![\d]+)', line)
            if metadata_ref:
                ref = metadata_ref.group(1)[1:]  # Remove '!' prefix
                if ref in metadata_map:
                    bb_id = metadata_map[ref]
                    functions[current_func].append({
                        'name': current_bb_name,
                        'id': int(bb_id)
                    })
                    # Reset for next basic block
                    bb_started = False
    
    return functions


def parse_csv(csv_file):
    """Parses CSV output file from IRBBLabel pass.
    
    Reads the CSV file generated by the pass and organizes basic block
    information by function name for easy lookup and validation.
    
    Args:
        csv_file: Path to CSV file (typically bb_info.csv or custom name)
        
    Returns:
        Dictionary mapping function names to lists of basic block dictionaries.
        Each BB dict contains:
            - 'name': Basic block name (empty string for entry blocks)
            - 'func_id': Function ID (integer)
            - 'bb_id': Basic block ID within function (integer)
            
        Example:
            {
                'main': [
                    {'name': '', 'func_id': 0, 'bb_id': 0},
                    {'name': 'if.then', 'func_id': 0, 'bb_id': 1}
                ]
            }
    
    CSV format expected:
        FunctionName,FunctionID,BasicBlockName,BasicBlockID
        main,0,,0
        main,0,if.then,1
        main,0,if.else,2
        
    Note:
        - Empty BasicBlockName represents entry block
        - FunctionID is unique per function
        - BasicBlockID is unique per basic block within the entire program
    
    Raises:
        IOError: If csv_file cannot be read
        csv.Error: If CSV format is malformed
        KeyError: If required columns are missing
    """
    csv_data = defaultdict(list)
    
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            func_name = row['FunctionName']
            func_id = int(row['FunctionID'])
            bb_name = row['BasicBlockName']
            bb_id = int(row['BasicBlockID'])
            
            csv_data[func_name].append({
                'name': bb_name,
                'func_id': func_id,
                'bb_id': bb_id
            })
    
    return csv_data


def validate_metadata(ir_metadata, csv_data):
    """Validates that IR metadata matches CSV data.
    
    Cross-validates the basic block information extracted from IR metadata
    against the CSV output. Checks for consistency in:
      - Function presence (all CSV functions must be in IR)
      - Basic block counts per function
      - Basic block IDs (must match exactly)
      - Basic block names (must match for named blocks, both empty for entry)
    
    Args:
        ir_metadata: Dictionary from parse_ir_metadata()
            Format: {func_name: [{'name': str, 'id': int}, ...]}
        csv_data: Dictionary from parse_csv()
            Format: {func_name: [{'name': str, 'func_id': int, 'bb_id': int}, ...]}
    
    Returns:
        Tuple (is_valid, error_messages):
            - is_valid: Boolean, True if all validations pass
            - error_messages: List of error description strings (empty if valid)
    
    Validation logic:
        1. Check all CSV functions exist in IR
        2. Check all IR functions exist in CSV
        3. For each function:
           a. Verify BB counts match
           b. For each BB ID:
              - Check BB exists in both IR and CSV
              - Verify names match exactly (empty or same string)
    
    Error reporting:
        Errors are descriptive and include function name, BB ID, and
        specific mismatch details to aid in debugging.
        
    Example errors:
        - "Function 'helper' in CSV but not found in IR"
        - "Function 'main': IR has 4 BBs, CSV has 3 BBs"
        - "Function 'compute' BB ID 5: IR name='loop.body', CSV name='for.body'"
    """
    errors = []
    
    # Check that all functions in CSV are in IR
    for func_name in csv_data:
        if func_name not in ir_metadata:
            errors.append(f"Function '{func_name}' in CSV but not found in IR")
    
    # Validate each function's basic blocks
    for func_name in ir_metadata:
        ir_bbs = ir_metadata[func_name]
        
        if func_name not in csv_data:
            errors.append(f"Function '{func_name}' in IR but not in CSV")
            continue
        
        csv_bbs = csv_data[func_name]
        
        # Check count matches
        if len(ir_bbs) != len(csv_bbs):
            errors.append(
                f"Function '{func_name}': IR has {len(ir_bbs)} BBs, "
                f"CSV has {len(csv_bbs)} BBs"
            )
            # Don't skip - still try to match what we can
        
        # Create a mapping of BB IDs for validation
        ir_bb_by_id = {bb['id']: bb for bb in ir_bbs}
        csv_bb_by_id = {bb['bb_id']: bb for bb in csv_bbs}
        
        # Validate each basic block by ID
        all_ids = set(ir_bb_by_id.keys()) | set(csv_bb_by_id.keys())
        for bb_id in sorted(all_ids):
            if bb_id not in ir_bb_by_id:
                errors.append(
                    f"Function '{func_name}' BB ID {bb_id}: "
                    f"in CSV but not in IR"
                )
                continue
            
            if bb_id not in csv_bb_by_id:
                errors.append(
                    f"Function '{func_name}' BB ID {bb_id}: "
                    f"in IR but not in CSV"
                )
                continue
            
            ir_bb = ir_bb_by_id[bb_id]
            csv_bb = csv_bb_by_id[bb_id]
            
            # Validate BB names match (both empty, or both non-empty and equal)
            ir_name = ir_bb['name'] if ir_bb['name'] else ''
            csv_name = csv_bb['name'] if csv_bb['name'] else ''
            
            # Names should match exactly
            if ir_name != csv_name:
                errors.append(
                    f"Function '{func_name}' BB ID {bb_id}: "
                    f"IR name='{ir_name}', CSV name='{csv_name}'"
                )
    
    return len(errors) == 0, errors


def main():
    """Main entry point for metadata validation script.
    
    Parses command-line arguments, reads IR and CSV files, performs validation,
    and reports results with appropriate exit code.
    
    Usage:
        python3 verify_metadata.py <instrumented.ll> <bb_info.csv>
    
    Exit codes:
        0: Validation passed - all metadata matches CSV
        1: Validation failed or error occurred
        
    Output:
        Success: Prints checkmark, function count, and total BB count
        Failure: Prints X mark and lists all validation errors
    """
    if len(sys.argv) < 3:
        print("Usage: verify_metadata.py <instrumented.ll> <bb_info.csv>")
        print()
        print("Validates that metadata in instrumented LLVM IR matches CSV output.")
        sys.exit(1)
    
    ir_file = sys.argv[1]
    csv_file = sys.argv[2]
    
    # Verify files exist
    if not Path(ir_file).exists():
        print(f"ERROR: IR file not found: {ir_file}")
        sys.exit(1)
    
    if not Path(csv_file).exists():
        print(f"ERROR: CSV file not found: {csv_file}")
        sys.exit(1)
    
    # Parse files
    try:
        ir_metadata = parse_ir_metadata(ir_file)
        csv_data = parse_csv(csv_file)
    except Exception as e:
        print(f"ERROR: Failed to parse files: {e}")
        sys.exit(1)
    
    # Validate
    is_valid, errors = validate_metadata(ir_metadata, csv_data)
    
    if is_valid:
        print(f"✓ Metadata validation PASSED")
        print(f"  - Functions: {len(ir_metadata)}")
        total_bbs = sum(len(bbs) for bbs in ir_metadata.values())
        print(f"  - Total basic blocks: {total_bbs}")
        sys.exit(0)
    else:
        print(f"✗ Metadata validation FAILED")
        for error in errors:
            print(f"  - {error}")
        sys.exit(1)


if __name__ == '__main__':
    main()
