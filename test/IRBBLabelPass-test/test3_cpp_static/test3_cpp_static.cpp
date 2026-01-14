// Test Case 3: C++ with same function names (overloading) and templates
// Purpose: Test handling of name mangling, overloading, and complex C++ features

#include <iostream>
#include <vector>
#include <string>

// Static library simulation - functions in anonymous namespace
namespace {
    int internal_counter = 0;
    
    void increment() {
        internal_counter++;
    }
    
    int get_count() {
        return internal_counter;
    }
}

// Class with same method names but different signatures
class Calculator {
private:
    int value;
    
public:
    Calculator() : value(0) {}
    
    // Overloaded add functions
    int add(int a) {
        value += a;
        return value;
    }
    
    int add(int a, int b) {
        value += (a + b);
        return value;
    }
    
    double add(double a, double b) {
        return a + b;
    }
    
    // Template method
    template<typename T>
    T multiply(T a, T b) {
        return a * b;
    }
};

// Template function
template<typename T>
T max_value(T a, T b) {
    if (a > b) {
        return a;
    }
    return b;
}

// Overloaded free functions with same name
void process(int x) {
    std::cout << "Processing int: " << x << std::endl;
}

void process(double x) {
    std::cout << "Processing double: " << x << std::endl;
}

void process(const std::string& x) {
    std::cout << "Processing string: " << x << std::endl;
}

// Nested classes
class Outer {
public:
    class Inner {
    public:
        void print() {
            std::cout << "Inner class print" << std::endl;
        }
    };
    
    void process() {
        std::cout << "Outer class process" << std::endl;
    }
};

// Lambda functions
auto lambda_test = []() {
    return 42;
};

int main() {
    // Test static library functions
    for (int i = 0; i < 3; i++) {
        increment();
    }
    std::cout << "Counter: " << get_count() << std::endl;
    
    // Test overloaded methods
    Calculator calc;
    calc.add(5);
    calc.add(3, 7);
    double result = calc.add(2.5, 3.5);
    std::cout << "Add result: " << result << std::endl;
    
    // Test template functions
    int int_max = max_value(10, 20);
    double double_max = max_value(3.14, 2.71);
    std::cout << "Max int: " << int_max << ", Max double: " << double_max << std::endl;
    
    // Test template methods
    int mult_result = calc.multiply(5, 6);
    std::cout << "Multiply result: " << mult_result << std::endl;
    
    // Test overloaded functions
    process(42);
    process(3.14);
    process(std::string("Hello"));
    
    // Test nested classes
    Outer outer;
    outer.process();
    Outer::Inner inner;
    inner.print();
    
    // Test lambda
    int lambda_result = lambda_test();
    std::cout << "Lambda result: " << lambda_result << std::endl;
    
    // Vector with control flow
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    for (const auto& num : numbers) {
        if (num % 2 == 0) {
            std::cout << num << " is even" << std::endl;
        } else {
            std::cout << num << " is odd" << std::endl;
        }
    }
    
    return 0;
}
