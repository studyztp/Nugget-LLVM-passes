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
// Test Case 4 - C++ Part: Mixed C++ and Fortran interoperability
//
// Purpose: Verify that the IRBBLabel pass handles multi-language LLVM modules:
//   - C++ and Fortran code compiled to separate IR files
//   - Linked together using llvm-link into single module
//   - Pass must handle both C++ mangled names and Fortran calling conventions
//   - extern "C" linkage for cross-language function calls
//
// This test validates:
//   - Pass works on merged multi-language LLVM modules
//   - C++ name mangling doesn't interfere with Fortran functions
//   - Fortran functions with bind(C) are properly labeled
//   - CSV contains functions from both languages
//   - Complex C++ features (loops, recursion) + Fortran subroutines
//
// Expected behavior:
//   - CSV includes both C++ functions (mangled names) and Fortran subroutines
//   - Fortran functions appear with their bind(C) names (fortran_add, fortran_factorial)
//   - All basic blocks from both languages are instrumented
//   - No crashes or errors during multi-language IR processing

#include <iostream>
#include <vector>
#include <memory>

// Fortran function declarations with extern "C" for C linkage.
//
// These declarations allow C++ code to call Fortran subroutines.
// The Fortran code uses bind(C) to export these functions with
// C-compatible names (no Fortran name mangling).
//
// Args:
//   fortran_add: Adds two integers, result via pointer
//   fortran_factorial: Computes factorial, result via pointer
extern "C" {
  void fortran_add(int a, int b, int* result);
  void fortran_factorial(int n, int* result);
}

// C++ helper function - multiplies using repeated addition.
//
// Args:
//   a: Multiplicand
//   b: Multiplier (number of times to add a)
//
// Returns:
//   Product of a and b
//
// Implementation uses loop instead of multiplication operator to
// generate more interesting basic block structure for testing.
//
// Expected basic blocks:
//   - entry: initialize result = 0
//   - for.cond: check loop condition (i < b)
//   - for.body: perform addition
//   - for.inc: increment i
//   - for.end: return result
int cpp_multiply(int a, int b) {
  int result = 0;
  for (int i = 0; i < b; i++) {
    result += a;
  }
  return result;
}

// C++ power function - computes base raised to exponent.
//
// Args:
//   base: Base value
//   exp: Exponent (non-negative integer)
//
// Returns:
//   base^exp
//
// Expected basic blocks:
//   - entry: check if exp == 0
//   - if.then: return 1 for exp == 0
//   - if.end: initialize result = 1, fall through to loop
//   - for.cond: check loop condition
//   - for.body: multiply result *= base
//   - for.inc: increment i
//   - for.end: return result
int cpp_power(int base, int exp) {
  if (exp == 0) {
    return 1;
  }
  int result = 1;
  for (int i = 0; i < exp; i++) {
    result *= base;
  }
  return result;
}

// C++ function with complex control flow - classifies number properties.
//
// Args:
//   n: Integer to classify
//
// This function demonstrates complex if-else-if chains which create
// multiple basic blocks for each condition branch.
//
// Expected basic blocks:
//   - entry: check if n < 0
//   - if.then: negative branch
//   - if.else: check if n == 0
//   - if.then2: zero branch
//   - if.else3: check if n % 2 == 0
//   - if.then4: positive even branch
//   - if.else5: positive odd branch
//   - if.end: merge point
void cpp_classify_number(int n) {
  if (n < 0) {
    std::cout << n << " is negative" << std::endl;
  } else if (n == 0) {
    std::cout << n << " is zero" << std::endl;
  } else if (n % 2 == 0) {
    std::cout << n << " is positive and even" << std::endl;
  } else {
    std::cout << n << " is positive and odd" << std::endl;
  }
}

// Recursive C++ function - computes Fibonacci numbers.
//
// Args:
//   n: Position in Fibonacci sequence (0-indexed)
//
// Returns:
//   nth Fibonacci number
//
// This function demonstrates recursion which creates interesting
// basic block structure and tests pass handling of recursive calls.
//
// Expected basic blocks:
//   - entry: check if n <= 1
//   - if.then: return n for base case
//   - if.end: recursive case, compute fib(n-1) + fib(n-2)
int cpp_fibonacci(int n) {
  if (n <= 1) {
    return n;
  }
  return cpp_fibonacci(n - 1) + cpp_fibonacci(n - 2);
}

// Main function demonstrating C++/Fortran interoperability.
//
// Returns:
//   Exit code (0 for success)
//
// This function exercises:
//   - C++ functions (cpp_multiply, cpp_power, cpp_classify_number, cpp_fibonacci)
//   - Fortran subroutines via extern "C" (fortran_add, fortran_factorial)
//   - Nested loops with complex control flow
//
// Expected CSV output:
//   - All C++ functions with mangled names
//   - Fortran functions with C-compatible names (fortran_add, fortran_factorial)
//   - main function with many basic blocks from control flow
//
// This tests the pass's ability to handle a merged LLVM module
// containing IR from multiple languages and compilers.
int main() {
  std::cout << "=== Mixed C++ and Fortran Test ===" << std::endl;
  
  // Test C++ functions
  std::cout << "\n=== Testing C++ Functions ===" << std::endl;
  int mult_result = cpp_multiply(7, 6);
  std::cout << "C++ multiply(7, 6) = " << mult_result << std::endl;
  
  int power_result = cpp_power(2, 5);
  std::cout << "C++ power(2, 5) = " << power_result << std::endl;
  
  // Test complex control flow
  cpp_classify_number(-5);
  cpp_classify_number(0);
  cpp_classify_number(10);
  cpp_classify_number(7);
  
  // Test Fortran functions via C interoperability
  std::cout << "\n=== Testing Fortran Integration ===" << std::endl;
  int fortran_add_result;
  fortran_add(15, 27, &fortran_add_result);
  std::cout << "Fortran add(15, 27) = " << fortran_add_result << std::endl;
  
  int fortran_fact_result;
  fortran_factorial(6, &fortran_fact_result);
  std::cout << "Fortran factorial(6) = " << fortran_fact_result << std::endl;
  
  // Test C++ recursion
  std::cout << "\n=== Testing C++ Recursion ===" << std::endl;
  for (int i = 0; i <= 8; i++) {
    std::cout << "Fibonacci(" << i << ") = " << cpp_fibonacci(i) << std::endl;
  }
  
  // Complex nested loops - tests nested control flow BB generation
  std::cout << "\n=== Testing Nested Control Flow ===" << std::endl;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (i == j) {
        std::cout << "Diagonal: (" << i << "," << j << ")" << std::endl;
      } else if (i < j) {
        std::cout << "Upper: (" << i << "," << j << ")" << std::endl;
      } else {
        std::cout << "Lower: (" << i << "," << j << ")" << std::endl;
      }
    }
  }
  
  return 0;
}
