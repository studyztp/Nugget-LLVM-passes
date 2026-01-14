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
// Test Case 1: Simple C code with basic blocks and control flow
//
// Purpose: Verify that the IRBBLabel pass correctly instruments basic C code
// with straightforward control flow (function calls, if/else branches).
//
// This test validates:
//   - Basic function labeling and ID assignment
//   - Basic block identification in simple functions
//   - Control flow with if/else branches
//   - CSV output generation with correct function and BB metadata
//   - IR metadata (!bb.id) insertion at appropriate terminator instructions
//
// Expected behavior:
//   - Pass should label all functions (add, subtract, main)
//   - Each basic block should receive a unique ID
//   - CSV should contain FunctionName, FunctionID, BasicBlockName, BasicBlockID
//   - Instrumented IR should have !bb.id metadata on branch/return instructions

#include <stdio.h>

// Performs integer addition.
//
// Args:
//   a: First integer operand
//   b: Second integer operand
//
// Returns:
//   Sum of a and b
//
// This function contains one basic block (entry block) that directly returns.
int add(int a, int b) {
  return a + b;
}

// Performs integer subtraction.
//
// Args:
//   a: Minuend (value to subtract from)
//   b: Subtrahend (value to subtract)
//
// Returns:
//   Difference of a minus b
//
// This function contains one basic block (entry block) that directly returns.
int subtract(int a, int b) {
  return a - b;
}

// Main entry point demonstrating simple control flow.
//
// Returns:
//   Exit code (0 for success)
//
// This function contains multiple basic blocks:
//   - Entry block: variable initialization and function calls
//   - if.then: block executed when sum > 10
//   - if.else: block executed when sum <= 10
//   - if.end: merge block after conditional
//
// The pass should label all 4 basic blocks with unique IDs and record them
// in the CSV output.
int main() {
  int x = 10;
  int y = 5;
  
  // Call add function - tests inter-procedural labeling
  int sum = add(x, y);
  // Call subtract function
  int diff = subtract(x, y);
  
  // Conditional branch - creates multiple basic blocks (if.then, if.else, if.end)
  if (sum > 10) {
    printf("Sum is greater than 10: %d\n", sum);
  } else {
    printf("Sum is not greater than 10: %d\n", sum);
  }
  
  printf("Difference: %d\n", diff);
  
  return 0;
}
