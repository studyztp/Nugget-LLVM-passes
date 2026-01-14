// Test Case 1: Simple test with small amount of basic blocks and functions
// Purpose: Verify the pass runs correctly on basic C code

#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int main() {
    int x = 10;
    int y = 5;
    
    int sum = add(x, y);
    int diff = subtract(x, y);
    
    if (sum > 10) {
        printf("Sum is greater than 10: %d\n", sum);
    } else {
        printf("Sum is not greater than 10: %d\n", sum);
    }
    
    printf("Difference: %d\n", diff);
    
    return 0;
}
