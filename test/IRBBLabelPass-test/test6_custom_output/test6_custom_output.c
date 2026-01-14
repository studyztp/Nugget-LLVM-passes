// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Zhantong Qiu
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Test Case 6: Custom output CSV filename
//
// Purpose: Verify that the IRBBLabel pass correctly handles custom parameters
// via the new pass manager's parameter passing syntax:
//
//   -passes="ir-bb-label-pass<output_csv=custom_name.csv>"
//
// This test validates:
//   - Pass option parsing works correctly
//   - Custom CSV filename is used instead of default bb_info.csv
//   - Pass parameter syntax follows LLVM conventions (<key=value>)
//   - CSV generation works identically regardless of filename
//
// Pass parameter details:
//   - Pass name: ir-bb-label-pass
//   - Option syntax: <key=value>
//   - Default: output_csv=bb_info.csv
//   - This test: output_csv=my_custom_bb_output.csv
//
// Expected behavior:
//   - CSV file created with custom name (my_custom_bb_output.csv)
//   - File contains standard format (FunctionName,FunctionID,BasicBlockName,BasicBlockID)
//   - All basic blocks are labeled correctly
//   - No default bb_info.csv file is created
//
// Implementation notes:
//   - Simple code (multiply, divide) to focus on parameter testing
//   - Standard control flow (if/else) to verify basic BB labeling
//   - Uses option escaping in CMake: \<output_csv=...\> to prevent shell interpretation

#include <stdio.h>

// Multiplies two integers.
//
// Args:
//   a: Multiplicand
//   b: Multiplier
//
// Returns:
//   Product of a and b
//
// Simple function with one basic block (entry) to verify
// basic pass functionality with custom output filename.
int multiply(int a, int b) {
  return a * b;
}

// Divides two integers with zero-check.
//
// Args:
//   a: Dividend
//   b: Divisor
//
// Returns:
//   Quotient of a/b, or 0 if b is zero
//
// This function demonstrates conditional logic which creates
// multiple basic blocks (entry, if.then, if.end).
//
// Expected basic blocks:
//   - entry: check if b == 0
//   - if.then: return 0 when b == 0
//   - if.end: perform division and return
int divide(int a, int b) {
  if (b == 0) {
    return 0;
  }
  return a / b;
}

// Main function demonstrating custom CSV output.
//
// Returns:
//   Exit code (0 for success)
//
// This function exercises multiply() and divide() functions
// while the pass generates CSV output to a custom filename
// (my_custom_bb_output.csv) instead of the default bb_info.csv.
//
// Test verification:
//   - Check that my_custom_bb_output.csv exists
//   - Verify it contains all basic blocks from main, multiply, divide
//   - Confirm default bb_info.csv is NOT created
int main() {
  int x = 20;
  int y = 4;
  
  int product = multiply(x, y);
  int quotient = divide(x, y);
  
  printf("Product: %d\n", product);
  printf("Quotient: %d\n", quotient);
  
  return 0;
}
