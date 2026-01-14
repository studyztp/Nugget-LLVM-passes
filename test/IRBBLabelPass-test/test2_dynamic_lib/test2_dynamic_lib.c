// Test Case 2: More complicated test with dynamic libraries and empty functions
// Purpose: Test if empty functions and external library calls are handled correctly

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Empty function - should not crash the pass
void empty_function() {
}

// Function with only declaration (no body) - handled by isDeclaration() check
extern void external_function();

// Function with single return
int get_constant() {
    return 42;
}

// Function using dynamic library functions
double calculate_distance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

char* process_string(const char* input) {
    if (input == NULL) {
        return NULL;
    }
    
    size_t len = strlen(input);
    char* result = (char*)malloc(len + 1);
    
    if (result == NULL) {
        return NULL;
    }
    
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

int main() {
    // Call empty function
    empty_function();
    
    // Test constant function
    int val = get_constant();
    printf("Constant value: %d\n", val);
    
    // Test math library function
    double dist = calculate_distance(0.0, 0.0, 3.0, 4.0);
    printf("Distance: %.2f\n", dist);
    
    // Test string processing
    const char* test_str = "hello world";
    char* upper = process_string(test_str);
    
    if (upper != NULL) {
        printf("Uppercase: %s\n", upper);
        free(upper);
    }
    
    // Complex control flow
    for (int i = 0; i < 5; i++) {
        if (i % 2 == 0) {
            printf("Even: %d\n", i);
        } else {
            printf("Odd: %d\n", i);
        }
    }
    
    return 0;
}
