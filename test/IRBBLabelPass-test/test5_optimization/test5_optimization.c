// Test Case 5: Optimization Pipeline Test
// Purpose: Compare direct compilation vs. pass-instrumented optimization pipeline
// This code is designed to be compute-heavy to measure optimization differences

#include <stdio.h>
#include <time.h>
#include <math.h>

// Fibonacci - recursive, good for testing loop unrolling
long fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// Matrix operations - good for testing vectorization
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

// Prime number calculation - good for testing loop optimizations
int is_prime(int n) {
    if (n <= 1) return 0;
    if (n <= 3) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return 0;
    }
    return 1;
}

int count_primes(int limit) {
    int count = 0;
    for (int i = 2; i <= limit; i++) {
        if (is_prime(i)) count++;
    }
    return count;
}

// Numerical computation - good for testing floating point optimizations
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

int main() {
    clock_t start, end;
    double cpu_time_used;
    
    printf("=== Optimization Test ===\n");
    printf("Testing compute-heavy operations\n\n");
    
    // Test 1: Fibonacci
    printf("Test 1: Fibonacci(35)\n");
    start = clock();
    long fib_result = fibonacci(35);
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Result: %ld\n", fib_result);
    printf("Time: %.6f seconds\n\n", cpu_time_used);
    
    // Test 2: Prime counting
    printf("Test 2: Count primes up to 10000\n");
    start = clock();
    int prime_count = count_primes(10000);
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Prime count: %d\n", prime_count);
    printf("Time: %.6f seconds\n\n", cpu_time_used);
    
    // Test 3: Numerical computation
    printf("Test 3: Sum computation (1000000 iterations)\n");
    start = clock();
    double sum_result = compute_sum(1000000);
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Sum result: %.6f\n", sum_result);
    printf("Time: %.6f seconds\n\n", cpu_time_used);
    
    // Test 4: Matrix operations
    printf("Test 4: Matrix multiply (50x50)\n");
    int matrix_size = 50;
    double a[matrix_size][matrix_size];
    double b[matrix_size][matrix_size];
    double c[matrix_size][matrix_size];
    
    // Initialize matrices
    for (int i = 0; i < matrix_size; i++) {
        for (int j = 0; j < matrix_size; j++) {
            a[i][j] = (double)(i + j) / matrix_size;
            b[i][j] = (double)(i - j) / matrix_size;
        }
    }
    
    start = clock();
    matrix_multiply(matrix_size, a, b, c);
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Result[0][0]: %.6f\n", c[0][0]);
    printf("Time: %.6f seconds\n\n", cpu_time_used);
    
    printf("=== All tests completed ===\n");
    
    return 0;
}
