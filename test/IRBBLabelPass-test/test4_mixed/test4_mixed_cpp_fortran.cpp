// Test Case 4 - C++ Part: Mixed C++ and Fortran interoperability test
// Purpose: Test the most complex scenario with multiple languages

#include <iostream>
#include <vector>
#include <memory>

// Fortran function declarations (extern "C" for C linkage)
extern "C" {
    void fortran_add(int a, int b, int* result);
    void fortran_factorial(int n, int* result);
}

// C++ helper functions
int cpp_multiply(int a, int b) {
    int result = 0;
    for (int i = 0; i < b; i++) {
        result += a;
    }
    return result;
}

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

// C++ function with complex control flow
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

// Recursive C++ function
int cpp_fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return cpp_fibonacci(n - 1) + cpp_fibonacci(n - 2);
}

int main() {
    std::cout << "=== Mixed C++ and Fortran Test ===" << std::endl;
    
    // Test C++ functions
    std::cout << "\n=== Testing C++ Functions ===" << std::endl;
    int mult_result = cpp_multiply(7, 6);
    std::cout << "C++ multiply(7, 6) = " << mult_result << std::endl;
    
    int power_result = cpp_power(2, 5);
    std::cout << "C++ power(2, 5) = " << power_result << std::endl;
    
    cpp_classify_number(-5);
    cpp_classify_number(0);
    cpp_classify_number(10);
    cpp_classify_number(7);
    
    // Test Fortran functions
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
    
    // Complex nested loops
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
