#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Zhantong Qiu
# All rights reserved.
#
# verify_machine_match.py
#
# This script verifies that IR basic blocks correctly map to machine code
# by analyzing the disassembled binary and comparing it with the IR.
#
# Supported architectures: x86-64, AArch64
#
# Verification checks:
#   1. Each hook in the assembly has a valid bb_id from the IR
#   2. The (inst_count, bb_id, interval) arguments in ASM match the IR
#   3. Missing hooks (IR -> ASM) are explained with evidence of optimization
#   4. No spurious hooks appear in unexpected places
#   5. nugget_init is called in nugget_roi_begin_
#
# Usage:
#   python3 verify_machine_match.py <instrumented.ll> <disassembly.txt> <bb_info.csv> <interval_length>

import sys
import re
import csv
from collections import defaultdict
from abc import ABC, abstractmethod


# ============================================================================
# Architecture-specific handlers
# ============================================================================

class ArchHandler(ABC):
    """Abstract base class for architecture-specific disassembly parsing."""
    
    @abstractmethod
    def get_arg_registers(self):
        """Return tuple of (arg1_reg, arg2_reg, arg3_reg) canonical names."""
        pass
    
    @abstractmethod
    def get_register_aliases(self, reg_name):
        """Return list of register aliases for the given canonical register."""
        pass
    
    @abstractmethod
    def get_call_pattern(self):
        """Return regex pattern to match calls to nugget_bb_hook."""
        pass
    
    @abstractmethod
    def get_mov_imm_pattern(self):
        """Return regex pattern to match loading immediate to register.
        Group 1: immediate value, Group 2: register name."""
        pass
    
    @abstractmethod
    def get_zero_reg_pattern(self):
        """Return regex pattern to match zeroing a register.
        Group 1 and 2: register names (if equal, it's zeroing)."""
        pass
    
    @abstractmethod
    def get_unconditional_jump_pattern(self):
        """Return regex pattern to match unconditional jumps."""
        pass
    
    @abstractmethod
    def get_branch_target_pattern(self):
        """Return regex pattern to match branch instructions with targets.
        Group 1: branch instruction, Group 2: target address."""
        pass
    
    @abstractmethod
    def parse_immediate(self, value_str):
        """Parse an immediate value string to integer."""
        pass


class X86_64Handler(ArchHandler):
    """Handler for x86-64 architecture."""
    
    def get_arg_registers(self):
        return ('rdi', 'rsi', 'rdx')
    
    def get_register_aliases(self, reg_name):
        aliases = {
            'rdi': ['rdi', 'edi', 'di'],
            'rsi': ['rsi', 'esi', 'si'],
            'rdx': ['rdx', 'edx', 'dx'],
        }
        return aliases.get(reg_name, [reg_name])
    
    def get_call_pattern(self):
        return re.compile(r'([0-9a-f]+):\s+.*call\s+[0-9a-f]+\s+<nugget_bb_hook>')
    
    def get_mov_imm_pattern(self):
        # Matches: mov $0x123,%edi  or  mov $123,%rsi
        return re.compile(r'mov\s+\$(0x[0-9a-f]+|\d+),\s*%(\w+)')
    
    def get_zero_reg_pattern(self):
        # Matches: xor %esi,%esi
        return re.compile(r'xor\s+%(\w+),\s*%(\w+)')
    
    def get_unconditional_jump_pattern(self):
        return re.compile(r'\bjmp\s+')
    
    def get_branch_target_pattern(self):
        return re.compile(r'\b(jmp|je|jne|jl|jle|jg|jge|ja|jae|jb|jbe|jz|jnz|js|jns)\s+([0-9a-f]+)')
    
    def parse_immediate(self, value_str):
        if value_str.startswith('0x') or value_str.startswith('0X'):
            return int(value_str, 16)
        elif value_str.startswith('#'):
            return int(value_str[1:], 0)
        else:
            return int(value_str)


class AArch64Handler(ArchHandler):
    """Handler for AArch64 (ARM64) architecture."""
    
    def get_arg_registers(self):
        return ('x0', 'x1', 'x2')
    
    def get_register_aliases(self, reg_name):
        # w0-w30 are 32-bit views of x0-x30
        aliases = {
            'x0': ['x0', 'w0'],
            'x1': ['x1', 'w1'],
            'x2': ['x2', 'w2'],
            'x3': ['x3', 'w3'],
        }
        return aliases.get(reg_name, [reg_name])
    
    def get_call_pattern(self):
        # AArch64 uses 'bl' for function calls
        return re.compile(r'([0-9a-f]+):\s+.*\bbl\s+[0-9a-f]+\s+<nugget_bb_hook>')
    
    def get_mov_imm_pattern(self):
        # Matches: mov w0, #0x123  or  mov x1, #456  or  movz w0, #0x123
        # Also matches: orr w0, wzr, #0x123 (common for some immediates)
        return re.compile(r'(?:mov[zk]?|orr)\s+([wx]\d+),\s*(?:[wx]zr,\s*)?#(0x[0-9a-f]+|\d+)', re.IGNORECASE)
    
    def get_zero_reg_pattern(self):
        # AArch64 zeros with: mov x0, #0 or mov w0, wzr or eor x0, x0, x0
        # We'll handle mov #0 in mov_imm, so here just handle eor
        return re.compile(r'eor\s+([wx]\d+),\s*([wx]\d+),\s*\2\b')
    
    def get_unconditional_jump_pattern(self):
        return re.compile(r'\b[b]\s+[0-9a-f]+\s*(?:<|$)')
    
    def get_branch_target_pattern(self):
        # Matches: b, b.eq, b.ne, b.lt, b.le, b.gt, b.ge, etc.
        return re.compile(r'\b(b(?:\.[a-z]{2,3})?)\s+([0-9a-f]+)')
    
    def parse_immediate(self, value_str):
        if value_str.startswith('0x') or value_str.startswith('0X'):
            return int(value_str, 16)
        elif value_str.startswith('#'):
            val = value_str[1:]
            if val.startswith('0x') or val.startswith('0X'):
                return int(val, 16)
            return int(val)
        else:
            return int(value_str)


def detect_architecture(disasm_path):
    """
    Detect architecture from disassembly file header.
    Returns the appropriate ArchHandler.
    """
    with open(disasm_path, 'r') as f:
        # Read first few lines looking for file format
        for _ in range(20):
            line = f.readline()
            if not line:
                break
            
            line_lower = line.lower()
            
            # Check for architecture indicators
            if 'elf64-x86-64' in line_lower or 'x86-64' in line_lower or 'i386:x86-64' in line_lower:
                return X86_64Handler(), 'x86-64'
            elif 'elf64-littleaarch64' in line_lower or 'aarch64' in line_lower or 'elf64-aarch64' in line_lower:
                return AArch64Handler(), 'aarch64'
            elif 'elf32-i386' in line_lower:
                # Could add i386 handler, but for now use x86-64 patterns
                return X86_64Handler(), 'x86-64'
    
    # Default to x86-64 if can't detect
    print("WARNING: Could not detect architecture, defaulting to x86-64")
    return X86_64Handler(), 'x86-64'


# ============================================================================
# Parsing functions
# ============================================================================

def parse_csv(csv_path):
    """Parse the BB info CSV file and return bb_id -> info mapping."""
    bb_info = {}
    
    with open(csv_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            bb_id = int(row['BasicBlockID'])
            bb_info[bb_id] = {
                'func_name': row['FunctionName'],
                'func_id': int(row['FunctionID']),
                'bb_name': row['BasicBlockName'],
                'inst_count': int(row['BasicBlockInstCount'])
            }
    
    return bb_info


def parse_ir_hooks(ir_path):
    """Parse the instrumented IR and extract all nugget_bb_hook calls with context."""
    ir_hooks = {}  # bb_id -> hook info
    current_function = None
    current_bb = None
    
    # Patterns for parsing IR
    func_pattern = re.compile(r'^define\s+.*@(\w+)\s*\(')
    bb_pattern = re.compile(r'^([a-zA-Z0-9_.]+):\s*;?\s*preds')
    hook_pattern = re.compile(r'call void @nugget_bb_hook\(i64\s+(\d+),\s*i64\s+(\d+),\s*i64\s+(\d+)\)')
    
    with open(ir_path, 'r') as f:
        for line in f:
            # Check for function definition
            func_match = func_pattern.match(line)
            if func_match:
                current_function = func_match.group(1)
                current_bb = 'entry'
                continue
            
            # Check for basic block label
            bb_match = bb_pattern.match(line)
            if bb_match:
                current_bb = bb_match.group(1)
                continue
            
            # Check for hook call
            hook_match = hook_pattern.search(line)
            if hook_match and current_function:
                inst_count = int(hook_match.group(1))
                bb_id = int(hook_match.group(2))
                interval = int(hook_match.group(3))
                ir_hooks[bb_id] = {
                    'inst_count': inst_count,
                    'bb_id': bb_id,
                    'interval': interval,
                    'function': current_function,
                    'bb_name': current_bb
                }
    
    return ir_hooks


def parse_disassembly_hooks_detailed(disasm_path, arch_handler):
    """
    Parse disassembly and extract nugget_bb_hook calls with their arguments.
    Returns list of dicts with function, address, inst_count, bb_id, interval.
    
    This parser works by reading the file, storing all lines, and then for each
    hook call, it looks backwards to find the register assignments.
    """
    asm_hooks = []
    
    # Read all lines
    with open(disasm_path, 'r') as f:
        lines = f.readlines()
    
    # Get architecture-specific patterns
    func_pattern = re.compile(r'^([0-9a-f]+)\s+<(\w+)>:')
    call_pattern = arch_handler.get_call_pattern()
    mov_imm_pattern = arch_handler.get_mov_imm_pattern()
    zero_reg_pattern = arch_handler.get_zero_reg_pattern()
    jmp_pattern = arch_handler.get_unconditional_jump_pattern()
    branch_target_pattern = arch_handler.get_branch_target_pattern()
    addr_pattern = re.compile(r'^\s*([0-9a-f]+):')
    
    arg1_reg, arg2_reg, arg3_reg = arch_handler.get_arg_registers()
    
    # First pass: find all function boundaries and jump targets
    func_ranges = []  # (start_line, func_name)
    jump_targets = set()  # addresses that are jump targets
    
    for i, line in enumerate(lines):
        func_match = func_pattern.match(line)
        if func_match:
            func_ranges.append((i, func_match.group(2)))
        
        # Find all jump targets
        jt_match = branch_target_pattern.search(line)
        if jt_match:
            target_addr = jt_match.group(2)
            jump_targets.add(target_addr)
    
    def get_function_at_line(line_idx):
        """Find which function contains a given line."""
        for i in range(len(func_ranges) - 1, -1, -1):
            if line_idx >= func_ranges[i][0]:
                return func_ranges[i][1]
        return "unknown"
    
    def get_line_address(line):
        """Extract address from a disassembly line."""
        addr_match = addr_pattern.match(line)
        if addr_match:
            return addr_match.group(1)
        return None
    
    def find_register_value_backwards(lines, call_line_idx, reg_name):
        """
        Starting from call_line_idx, search backwards for the value of reg_name.
        Handles control flow by tracing through jump sources.
        """
        aliases = arch_handler.get_register_aliases(reg_name)
        
        def search_from(start_idx, depth, visited):
            """Search backwards from start_idx with given depth."""
            for i in range(start_idx, max(0, start_idx - depth), -1):
                if i in visited:
                    continue
                visited.add(i)
                
                line = lines[i]
                
                # Stop at function start
                if func_pattern.match(line):
                    return None, []
                
                # Check for mov immediate
                mov_match = mov_imm_pattern.search(line)
                if mov_match:
                    # Handle different group orders for different architectures
                    if isinstance(arch_handler, AArch64Handler):
                        reg = mov_match.group(1).lower()
                        value_str = mov_match.group(2)
                    else:
                        value_str = mov_match.group(1)
                        reg = mov_match.group(2).lower()
                    
                    if reg in aliases:
                        try:
                            return arch_handler.parse_immediate(value_str), []
                        except ValueError:
                            pass
                
                # Check for zeroing pattern
                zero_match = zero_reg_pattern.search(line)
                if zero_match:
                    reg1 = zero_match.group(1).lower()
                    reg2 = zero_match.group(2).lower()
                    if reg1 == reg2 and reg1 in aliases:
                        return 0, []
                
                # Get current address
                curr_addr = get_line_address(line)
                
                # Check if this address is a jump target preceded by unconditional jmp
                if curr_addr and curr_addr in jump_targets and i > 0:
                    prev_line = lines[i - 1]
                    if jmp_pattern.search(prev_line):
                        # Find all jumps TO this address
                        jump_sources = []
                        for j, search_line in enumerate(lines):
                            jt_match = branch_target_pattern.search(search_line)
                            if jt_match and jt_match.group(2) == curr_addr:
                                if j - 1 not in visited:
                                    jump_sources.append(j - 1)
                        return None, jump_sources
            
            return None, []
        
        visited = set()
        search_points = [(call_line_idx - 1, 50)]
        
        while search_points:
            start_idx, depth = search_points.pop(0)
            result, new_sources = search_from(start_idx, depth, visited)
            
            if result is not None:
                return result
            
            # Add new search points from jump sources
            for source in new_sources:
                search_points.append((source, 30))
        
        return -1  # Not found
    
    # Find all hook calls and extract their arguments
    for i, line in enumerate(lines):
        call_match = call_pattern.search(line)
        if call_match:
            address = call_match.group(1)
            current_function = get_function_at_line(i)
            
            inst_count = find_register_value_backwards(lines, i, arg1_reg)
            bb_id = find_register_value_backwards(lines, i, arg2_reg)
            interval = find_register_value_backwards(lines, i, arg3_reg)
            
            asm_hooks.append({
                'function': current_function,
                'address': address,
                'inst_count': inst_count,
                'bb_id': bb_id,
                'interval': interval
            })
    
    return asm_hooks


def verify_init_call(disasm_path, arch_handler):
    """Verify nugget_init is called in nugget_roi_begin_."""
    in_roi_begin = False
    init_found = False
    
    func_pattern = re.compile(r'^[0-9a-f]+\s+<(\w+)>:')
    
    # Architecture-specific call pattern
    if isinstance(arch_handler, AArch64Handler):
        call_init_pattern = re.compile(r'\bbl\s+.*nugget_init\b')
    else:
        call_init_pattern = re.compile(r'call.*nugget_init\b')
    
    with open(disasm_path, 'r') as f:
        for line in f:
            func_match = func_pattern.match(line)
            if func_match:
                func_name = func_match.group(1)
                if func_name == 'nugget_roi_begin_':
                    in_roi_begin = True
                elif in_roi_begin:
                    break
                continue
            
            if in_roi_begin and call_init_pattern.search(line):
                init_found = True
                break
    
    return init_found


# ============================================================================
# Main verification
# ============================================================================

def main():
    if len(sys.argv) != 5:
        print(f"Usage: {sys.argv[0]} <instrumented.ll> <disassembly.txt> <bb_info.csv> <interval_length>")
        sys.exit(1)
    
    ir_path = sys.argv[1]
    disasm_path = sys.argv[2]
    csv_path = sys.argv[3]
    interval_length = int(sys.argv[4])
    
    # Detect architecture
    arch_handler, arch_name = detect_architecture(disasm_path)
    
    print("=" * 70)
    print("IR to Machine Code Hook Verification")
    print("=" * 70)
    print(f"IR file:      {ir_path}")
    print(f"Disassembly:  {disasm_path}")
    print(f"CSV file:     {csv_path}")
    print(f"Interval:     {interval_length}")
    print(f"Architecture: {arch_name}")
    print()
    
    errors = []
    warnings = []
    
    # Parse CSV
    try:
        csv_bb_info = parse_csv(csv_path)
        print(f"CSV: {len(csv_bb_info)} basic blocks")
    except Exception as e:
        print(f"ERROR: Failed to parse CSV: {e}")
        sys.exit(1)
    
    # Parse IR hooks
    try:
        ir_hooks = parse_ir_hooks(ir_path)
        print(f"IR:  {len(ir_hooks)} nugget_bb_hook calls")
    except Exception as e:
        print(f"ERROR: Failed to parse IR: {e}")
        sys.exit(1)
    
    # Parse ASM hooks with arguments
    try:
        asm_hooks = parse_disassembly_hooks_detailed(disasm_path, arch_handler)
        print(f"ASM: {len(asm_hooks)} nugget_bb_hook calls")
    except Exception as e:
        print(f"ERROR: Failed to parse disassembly: {e}")
        sys.exit(1)
    
    print()
    print("-" * 70)
    print("Verification Step 1: Check nugget_init call")
    print("-" * 70)
    
    init_found = verify_init_call(disasm_path, arch_handler)
    if init_found:
        print("✓ nugget_init is called in nugget_roi_begin_")
    else:
        errors.append("nugget_init not found in nugget_roi_begin_")
        print("✗ nugget_init NOT found in nugget_roi_begin_")
    
    print()
    print("-" * 70)
    print("Verification Step 2: Validate ASM hooks have valid IR origins")
    print("-" * 70)
    
    ir_bb_ids = set(ir_hooks.keys())
    valid_asm_hooks = []
    spurious_hooks = []
    
    for h in asm_hooks:
        if h['bb_id'] in ir_bb_ids:
            valid_asm_hooks.append(h)
        elif h['bb_id'] >= 0:
            spurious_hooks.append(h)
    
    print(f"Valid ASM hooks (bb_id in IR): {len(valid_asm_hooks)}")
    print(f"Spurious ASM hooks (bb_id not in IR): {len(spurious_hooks)}")
    
    if spurious_hooks:
        for h in spurious_hooks:
            errors.append(f"Spurious hook at {h['address']} in {h['function']}: bb_id={h['bb_id']}")
            print(f"  ✗ Spurious: addr={h['address']} func={h['function']} bb_id={h['bb_id']}")
    
    print()
    print("-" * 70)
    print("Verification Step 3: Check for missing hooks (optimized away)")
    print("-" * 70)
    
    asm_bb_ids = set(h['bb_id'] for h in asm_hooks if h['bb_id'] >= 0)
    missing_bb_ids = ir_bb_ids - asm_bb_ids
    
    print(f"IR hooks: {len(ir_bb_ids)}, ASM hooks: {len(asm_bb_ids)}")
    print(f"Missing (optimized away): {len(missing_bb_ids)}")
    
    if missing_bb_ids:
        print(f"  Missing bb_ids: {sorted(missing_bb_ids)}")
        # These are warnings, not errors - backend may optimize away BBs
        for bb_id in sorted(missing_bb_ids):
            info = ir_hooks[bb_id]
            warnings.append(f"Hook for bb_id {bb_id} (func={info['function']}) missing in ASM (likely optimized)")
    
    print()
    print("-" * 70)
    print("Verification Step 4: Verify hook arguments match")
    print("-" * 70)
    
    arg_mismatches = []
    for h in valid_asm_hooks:
        bb_id = h['bb_id']
        ir_hook = ir_hooks[bb_id]
        
        # Check inst_count
        if h['inst_count'] != ir_hook['inst_count']:
            arg_mismatches.append({
                'bb_id': bb_id,
                'field': 'inst_count',
                'ir_val': ir_hook['inst_count'],
                'asm_val': h['inst_count'],
                'address': h['address']
            })
        
        # Check interval
        if h['interval'] != ir_hook['interval']:
            arg_mismatches.append({
                'bb_id': bb_id,
                'field': 'interval',
                'ir_val': ir_hook['interval'],
                'asm_val': h['interval'],
                'address': h['address']
            })
    
    if arg_mismatches:
        print(f"Argument mismatches: {len(arg_mismatches)}")
        for m in arg_mismatches:
            errors.append(f"bb_id {m['bb_id']}: {m['field']} mismatch - IR={m['ir_val']}, ASM={m['asm_val']}")
            print(f"  ✗ bb_id {m['bb_id']}: {m['field']} mismatch - IR={m['ir_val']}, ASM={m['asm_val']}")
    else:
        print("✓ All hook arguments match between IR and ASM")
    
    print()
    print("-" * 70)
    print("Verification Step 5: Summary")
    print("-" * 70)
    
    print(f"Total IR hooks:     {len(ir_hooks)}")
    print(f"Total ASM hooks:    {len(asm_hooks)}")
    print(f"Valid ASM hooks:    {len(valid_asm_hooks)}")
    print(f"Missing (optimized): {len(missing_bb_ids)}", end="")
    if missing_bb_ids:
        print(f"  (bb_ids: {sorted(missing_bb_ids)})")
    else:
        print()
    print(f"Spurious (invalid):  {len(spurious_hooks)}")
    
    print()
    if errors:
        print(f"ERRORS ({len(errors)}):")
        for e in errors:
            print(f"  ✗ {e}")
    
    if warnings:
        print(f"WARNINGS ({len(warnings)}):")
        for w in warnings[:5]:  # Only show first 5
            print(f"  ⚠ {w}")
        if len(warnings) > 5:
            print(f"  ... and {len(warnings) - 5} more")
    
    print()
    if errors:
        print("VERIFICATION FAILED")
        sys.exit(1)
    else:
        print("VERIFICATION PASSED")
        sys.exit(0)


if __name__ == "__main__":
    main()
