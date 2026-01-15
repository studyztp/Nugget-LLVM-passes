/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 Zhantong Qiu
 * All rights reserved.
 *
 * Test 2: Machine Code Matching Test
 *
 * This is a more complex program designed to test that IR basic blocks
 * correctly map to machine basic blocks after full compilation.
 *
 * Features tested:
 *   - Multiple functions with different control flow patterns
 *   - Nested loops
 *   - Switch statements
 *   - Function calls
 *   - Conditional branches
 *
 * The test verifies that nugget_bb_hook_ calls appear in the disassembled
 * binary with the correct function_id and bb_id arguments.
 */

#include <stdint.h>
#include <stdio.h>

/* External declarations for nugget runtime functions */
extern void nugget_init_(uint64_t total_bb_count);
extern void nugget_roi_begin_(void);
extern void nugget_roi_end_(void);
extern void nugget_bb_hook_(uint64_t function_id, uint64_t bb_id, uint64_t interval_length);

/* Prevent inlining to preserve function boundaries */
__attribute__((noinline))
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    
    int a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

__attribute__((noinline))
int factorial(int n) {
    int result = 1;
    while (n > 1) {
        result *= n;
        n--;
    }
    return result;
}

__attribute__((noinline))
int classify_number(int n) {
    /* Switch statement creates multiple basic blocks */
    switch (n % 4) {
        case 0:
            return n * 2;
        case 1:
            return n + 10;
        case 2:
            return n - 5;
        case 3:
            return n / 2;
        default:
            return n;
    }
}

__attribute__((noinline))
int nested_loops(int rows, int cols) {
    int sum = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if ((i + j) % 2 == 0) {
                sum += i * cols + j;
            } else {
                sum -= i + j;
            }
        }
    }
    return sum;
}

__attribute__((noinline))
int conditional_chain(int a, int b, int c) {
    int result = 0;
    
    if (a > b) {
        if (b > c) {
            result = a + b + c;
        } else if (a > c) {
            result = a * 2;
        } else {
            result = c - a;
        }
    } else {
        if (a > c) {
            result = b - a;
        } else {
            result = b + c;
        }
    }
    
    return result;
}

int main(int argc, char *argv[]) {
    int n = 10;
    if (argc > 1) {
        n = argv[1][0] - '0';
        if (n < 0 || n > 9) n = 5;
    }
    
    nugget_roi_begin_();
    
    /* Exercise all functions */
    int fib = fibonacci(n);
    int fact = factorial(n);
    int cls = classify_number(n);
    int nest = nested_loops(n, n);
    int cond = conditional_chain(n, n + 1, n - 1);
    
    /* Prevent optimization from removing results */
    int total = fib + fact + cls + nest + cond;
    
    nugget_roi_end_();
    
    printf("Results: fib=%d, fact=%d, cls=%d, nest=%d, cond=%d, total=%d\n",
           fib, fact, cls, nest, cond, total);
    
    return 0;
}
