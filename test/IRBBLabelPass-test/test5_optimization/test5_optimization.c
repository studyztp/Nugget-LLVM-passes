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
// Test Case 5: Optimization Pipeline Comparison
//
// Purpose: Compare two compilation approaches to verify IRBBLabel pass works
// correctly in optimized code:
//
// Pipeline 1 (Direct):   source -> clang -O2 -> binary
// Pipeline 2 (Labeled):  source -> clang -O0 -> opt -O2 -> IRBBLabel ->
//                        llc -O2 -> binary
//
// This test validates:
//   - Pass works on optimized IR (after -O2 transformations)
//   - Basic blocks are correctly labeled even after aggressive optimizations
//   - Optimization passes don't remove or corrupt BB labels
//   - Performance impact of instrumentation is minimal
//   - Labeled code produces correct results (functional correctness)
//
// Why this test matters:
//   - Optimizations merge, split, and eliminate basic blocks
//   - Loop unrolling, inlining, and vectorization change BB structure
//   - Pass must handle optimized BB names (e.g., .lr.ph, ._crit_edge)
//   - Real-world usage will instrument optimized production code
//
// Expected behavior:
//   - Both binaries produce identical output (functional correctness)
//   - CSV contains all basic blocks after optimization
//   - Performance difference < 5% (instrumentation overhead is minimal)
//   - Pass captures comparison reports for optimization passes
//
// Code characteristics:
//   - Compute-heavy to measure optimization impact
//   - Recursive functions (fibonacci) test inlining decisions
//   - Nested loops (matrix multiply) test vectorization and loop opts
//   - Math functions (sin, cos, sqrt) test libm integration
//   - Prime counting tests loop optimizations and branch prediction

#include <stdio.h>
#include <time.h>
#include <math.h>

// Recursive Fibonacci - tests inlining and recursion optimization.
//
// Args:
//   n: Position in Fibonacci sequence
//
// Returns:
//   nth Fibonacci number
//
// This function is intentionally naive (exponential time complexity)
// to test how -O2 optimization handles recursion:
//   - May inline small cases (n <= 1)
//   - May apply memoization or tail-call optimization
//   - Tests pass robustness with recursive call graphs
//
// Expected after -O2:
//   - Some level of inlining for base cases
//   - Possible tail-call optimization
//   - Basic blocks may be merged or reordered
long fibonacci(int n) {
  if (n <= 1) return n;
  return fibonacci(n - 1) + fibonacci(n - 2);
}

// Matrix multiplication - tests loop vectorization and cache optimization.
//
// Args:
//   n: Matrix dimension (n x n matrices)
//   a, b: Input matrices
//   c: Output matrix (a * b)
//
// This function has nested loops (i, j, k) creating complex basic block
// structure. Optimization passes will:
//   - Attempt to vectorize inner loops (SIMD)
//   - Reorder loops for better cache locality
//   - Unroll loops to reduce branch overhead
//   - Generate optimized BB names (e.g., vector.body, scalar.ph)
//
// Expected after -O2:
//   - Vector prologue/epilogue basic blocks
//   - Unrolled loop bodies
//   - Complex BB names like .lr.ph, ._crit_edge
void matrix_multiply(int n, double a[n][n], double b[n][n], double c[n][n]) {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      c[i][j] = 0.0;
      for (int k = 0; k < n; k++) {
        c[i][j] += a[i][k] * b[k][j];
      }
    }
  }
}

// Primality test - tests branch prediction and loop optimization.
//
// Args:
//   n: Integer to test for primality
//
// Returns:
//   1 if n is prime, 0 otherwise
//
// This function has multiple early-exit conditions which create
// many basic blocks. Optimization will:
//   - Predict common branches (even/odd)
//   - Optimize modulo operations
//   - May unroll the divisibility checking loop
//
// Expected after -O2:
//   - Branch optimization (likely/unlikely hints)
//   - Loop unrolling in the i += 6 loop
//   - Possibly inlined when called from count_primes
int is_prime(int n) {
  if (n <= 1) return 0;
  if (n <= 3) return 1;
  if (n % 2 == 0 || n % 3 == 0) return 0;
  
  // Check divisibility by numbers of form 6k ± 1
  for (int i = 5; i * i <= n; i += 6) {
    if (n % i == 0 || n % (i + 2) == 0)
      return 0;
  }
  return 1;
}

// Counts prime numbers up to limit.
//
// Args:
//   limit: Upper bound (inclusive) for prime counting
//
// Returns:
//   Number of primes <= limit
//
// This function tests:
//   - Loop optimization with function call inside
//   - Potential for inlining is_prime()
//   - Branch optimization for prime/non-prime paths
int count_primes(int limit) {
  int count = 0;
  for (int i = 2; i <= limit; i++) {
    if (is_prime(i)) count++;
  }
  return count;
}

// Numerical computation - tests floating-point optimization.
//
// Args:
//   iterations: Number of iterations to perform
//
// Returns:
//   Accumulated sum of mathematical function values
//
// This function demonstrates:
//   - Floating-point math library calls (sin, cos, sqrt)
//   - Loop with expensive computations
//   - Potential for FMA (fused multiply-add) optimization
//   - Math function inlining or fast-math optimizations
//
// Expected after -O2:
//   - Fast math approximations if enabled
//   - Vectorization of math operations if possible
//   - Loop unrolling
double compute_sum(int iterations) {
  double sum = 0.0;
  double pi_approx = 3.14159265359;
  
  for (int i = 0; i < iterations; i++) {
    double x = (double)i / iterations;
    // Compute some mathematical function
    sum += sin(x * pi_approx) * cos(x * pi_approx) + sqrt(x + 1.0);
  }
  return sum;
}

// Main function - runs all performance tests.
//
// Returns:
//   Exit code (0 for success)
//
// This function executes 4 compute-intensive tests to measure optimization
// impact and verify functional correctness:
//
// Test 1 - Fibonacci(35): Tests recursion and inlining
//   - Naive recursive implementation (exponential time)
//   - Measures optimization's effect on recursive calls
//   - Expected: -O2 may inline base cases, apply tail-call optimization
//
// Test 2 - Count primes up to 10000: Tests loop and branch optimization
//   - Calls is_prime() 10000 times
//   - Tests branch prediction and potential inlining
//   - Expected: Loop unrolling, optimized modulo operations
//
// Test 3 - Numerical sum (1M iterations): Tests FP and vectorization
//   - Heavy floating-point math (sin, cos, sqrt)
//   - Tests SIMD vectorization of math operations
//   - Expected: Fast-math optimizations, loop vectorization
//
// Test 4 - Matrix multiply (50x50): Tests memory and loop optimization
//   - Triple-nested loops (i, j, k)
//   - Tests cache locality and vectorization
//   - Expected: Loop interchange/unrolling, SIMD operations
//
// Each test measures execution time to detect performance regressions
// from instrumentation. The compare_performance.sh script verifies
// overhead is within acceptable bounds (< 5%).

// Helper function to compute elapsed time in seconds from timespec
static double elapsed_seconds(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    struct timespec start, end;
    double elapsed;
    
    printf("=== Optimization Test ===\n");
    printf("Testing compute-heavy operations\n\n");
    
    // Test 1: Fibonacci - recursive function optimization
    printf("Test 1: Fibonacci(35)\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    long fib_result = fibonacci(35);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = elapsed_seconds(start, end);
    printf("Result: %ld\n", fib_result);
    printf("Time: %.6f seconds\n\n", elapsed);
    
    // Test 2: Prime counting - loop and function call optimization
    printf("Test 2: Count primes up to 10000\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    int prime_count = count_primes(10000);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = elapsed_seconds(start, end);
    printf("Prime count: %d\n", prime_count);
    printf("Time: %.6f seconds\n\n", elapsed);
    
    // Test 3: Numerical computation - FP and math library optimization
    printf("Test 3: Sum computation (1000000 iterations)\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    double sum_result = compute_sum(1000000);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = elapsed_seconds(start, end);
    printf("Sum result: %.6f\n", sum_result);
    printf("Time: %.6f seconds\n\n", elapsed);
    
    // Test 4: Matrix operations - memory access and vectorization
    printf("Test 4: Matrix multiply (50x50)\n");
    int matrix_size = 50;
    double a[matrix_size][matrix_size];
    double b[matrix_size][matrix_size];
    double c[matrix_size][matrix_size];
    
    // Initialize matrices with simple pattern
    for (int i = 0; i < matrix_size; i++) {
        for (int j = 0; j < matrix_size; j++) {
            a[i][j] = (double)(i + j) / matrix_size;
            b[i][j] = (double)(i - j) / matrix_size;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    matrix_multiply(matrix_size, a, b, c);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = elapsed_seconds(start, end);
    printf("Result[0][0]: %.6f\n", c[0][0]);
    printf("Time: %.6f seconds\n\n", elapsed);
    
    printf("=== All tests completed ===\n");
    
    return 0;
}
