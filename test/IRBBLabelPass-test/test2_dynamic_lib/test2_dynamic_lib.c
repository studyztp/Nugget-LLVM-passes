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
// Test Case 2: Dynamic libraries and edge cases
//
// Purpose: Verify that the IRBBLabel pass handles complex edge cases including:
//   - Empty functions (no basic blocks with instructions)
//   - Functions with single return statements
//   - External library function calls (sqrt, strlen, malloc, free)
//   - Function declarations without definitions (isDeclaration() check)
//   - NULL pointer checks and early returns
//   - Complex control flow with nested conditionals and loops
//
// This test validates:
//   - Pass doesn't crash on empty or trivial functions
//   - External library functions are recognized via isDeclaration()
//   - Basic blocks in functions with dynamic allocations
//   - Error handling paths (NULL checks) create proper basic blocks
//   - Loop constructs generate appropriate basic block structures
//
// Expected behavior:
//   - empty_function() should be processed but generate minimal BBs
//   - external_function() should be skipped (isDeclaration() == true)
//   - Functions with library calls should have BBs labeled correctly
//   - CSV should reflect all defined functions and their basic blocks

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Empty function - tests pass robustness.
//
// This function has no executable statements, only entry and return.
// The pass should handle this gracefully without crashing.
// Expected: 1 basic block (entry block with immediate return)
void empty_function() {
}

// Function with only declaration (no body) - handled by isDeclaration() check.
//
// This is a forward declaration without implementation. The pass should
// skip it entirely since F.isDeclaration() will return true.
// Expected: Not included in CSV (no basic blocks to instrument)
extern void external_function();

// Function with single return - minimal control flow.
//
// Returns:
//   Integer constant 42
//
// This tests the simplest possible function with one basic block.
// Expected: 1 basic block (entry block with return)
int get_constant() {
  return 42;
}

// Calculates Euclidean distance using math library.
//
// Args:
//   x1, y1: Coordinates of first point
//   x2, y2: Coordinates of second point
//
// Returns:
//   Distance between the two points
//
// This function demonstrates:
//   - Use of external library function (sqrt from <math.h>)
//   - Arithmetic operations creating intermediate values
//   - Function should be linked with -lm flag
//
// Expected: 1 basic block (entry block with computations and return)
double calculate_distance(double x1, double y1, double x2, double y2) {
  double dx = x2 - x1;
  double dy = y2 - y1;
  return sqrt(dx * dx + dy * dy);
}

// Converts string to uppercase using dynamic memory allocation.
//
// Args:
//   input: NULL-terminated C string to convert
//
// Returns:
//   Newly allocated string in uppercase, or NULL if allocation fails
//   Caller is responsible for freeing the returned pointer
//
// This function demonstrates:
//   - NULL pointer checks (creates if.then and if.end basic blocks)
//   - Dynamic memory allocation (malloc) and error handling
//   - Loop constructs (for loop creates loop.cond, loop.body, loop.end blocks)
//   - String manipulation using library functions (strlen)
//   - Multiple return paths (early returns on error)
//
// Expected basic blocks:
//   - entry: function entry, check if input is NULL
//   - if.then (first): early return NULL if input is NULL
//   - if.end (first): call strlen and malloc
//   - if.then (second): early return NULL if malloc failed
//   - if.end (second): fall through to loop
//   - for.cond: loop condition check
//   - for.body: character conversion logic (with nested if/else)
//   - for.inc: loop increment
//   - for.end: after loop, set null terminator and return
char* process_string(const char* input) {
  // Early return if input is NULL
  if (input == NULL) {
    return NULL;
  }
  
  size_t len = strlen(input);
  char* result = (char*)malloc(len + 1);
  
  // Early return if allocation failed
  if (result == NULL) {
    return NULL;
  }
  
  // Convert each character to uppercase (ASCII 'a'-'z' to 'A'-'Z')
  // This loop creates multiple basic blocks for condition, body, and increment
  for (size_t i = 0; i < len; i++) {
    if (input[i] >= 'a' && input[i] <= 'z') {
      result[i] = input[i] - 32;  // Convert to uppercase
    } else {
      result[i] = input[i];
    }
  }
  result[len] = '\0';
  
  return result;
}

// Main function demonstrating all edge cases.
//
// Returns:
//   Exit code (0 for success)
//
// This function exercises all test scenarios:
//   - Calls empty_function() to verify pass doesn't crash
//   - Calls trivial function (get_constant) 
//   - Uses math library function (calculate_distance)
//   - Uses string processing with dynamic allocation
//   - Demonstrates proper memory management (free)
//   - Contains loops and conditionals for control flow testing
//
// Expected basic blocks:
//   - entry: initialization and function calls
//   - if.then: branch when upper != NULL
//   - if.end: after NULL check
//   - for.cond: loop condition for iteration
//   - for.body: loop body with if/else for even/odd
//   - if.then (in loop): even number branch
//   - if.else (in loop): odd number branch
//   - if.end (in loop): merge point after even/odd check
//   - for.inc: loop increment
//   - for.end: after loop completes
int main() {
  // Call empty function - verify pass handles it gracefully
  empty_function();
  
  // Test constant function - single basic block
  int val = get_constant();
  printf("Constant value: %d\n", val);
  
  // Test math library function - verify external calls work
  // This tests sqrt() from <math.h>, requiring -lm linker flag
  double dist = calculate_distance(0.0, 0.0, 3.0, 4.0);
  printf("Distance: %.2f\n", dist);
  
  // Test string processing with dynamic allocation and error handling
  const char* test_str = "hello world";
  char* upper = process_string(test_str);
  
  // NULL check creates conditional basic blocks
  if (upper != NULL) {
    printf("Uppercase: %s\n", upper);
    free(upper);  // Proper memory management
  }
  
  // Complex control flow: loop with nested conditional
  // This creates multiple basic blocks (for.cond, for.body, for.inc, for.end)
  // Plus nested if/else blocks inside the loop body
  for (int i = 0; i < 5; i++) {
    if (i % 2 == 0) {
      printf("Even: %d\n", i);
    } else {
      printf("Odd: %d\n", i);
    }
  }
  
  return 0;
}
