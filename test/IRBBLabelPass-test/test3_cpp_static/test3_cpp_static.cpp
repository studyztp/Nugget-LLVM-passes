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
// Test Case 3: C++ features with name mangling and templates
//
// Purpose: Verify that the IRBBLabel pass correctly handles advanced C++ features:
//   - Name mangling (C++ function names encoded with type information)
//   - Function overloading (same function name, different parameters)
//   - Template functions and methods (code generation for each instantiation)
//   - Anonymous namespaces (internal linkage, similar to static)
//   - Lambda expressions (compiler-generated function objects)
//   - Nested classes and their methods
//   - Range-based for loops (C++11 feature)
//
// This test validates:
//   - Pass correctly handles mangled function names (e.g., _ZNK3$_0clEv)
//   - Each template instantiation creates separate IR functions
//   - Overloaded functions are treated as distinct entities
//   - Lambda functions are captured and labeled
//   - Complex class hierarchies don't break the pass
//
// Expected behavior:
//   - CSV contains mangled names for C++ functions
//   - Template instantiations appear as separate functions
//   - Each overload gets unique function ID
//   - Lambda functions labeled like regular functions

#include <iostream>
#include <vector>
#include <string>

// Static library simulation - functions in anonymous namespace.
//
// Anonymous namespace provides internal linkage (similar to C static).
// These functions are only visible within this translation unit and
// won't conflict with identically-named functions in other files.
//
// The compiler generates unique internal names (often with .L or similar prefix).
namespace {
    int internal_counter = 0;
    
    // Increments the internal counter.
    //
    // This function demonstrates anonymous namespace usage and
    // simple state modification in internal linkage functions.
    void increment() {
        internal_counter++;
    }
    
    // Returns the current counter value.
    //
    // Returns:
    //   Current value of internal_counter
    int get_count() {
        return internal_counter;
    }
}

// Calculator class demonstrating method overloading.
//
// This class showcases:
//   - Function overloading (multiple add() methods with different signatures)
//   - Template methods (multiply() works with any type)
//   - Private member variables
//   - Constructor initialization
//
// Each overloaded method compiles to a distinct function in IR with
// mangled names encoding parameter types (e.g., _ZN10Calculator3addEi vs _ZN10Calculator3addEii).
class Calculator {
private:
  int value;  // Accumulated value for demonstration
  
public:
  // Constructor - initializes value to zero.
  Calculator() : value(0) {}
  
  // Adds single integer to accumulated value (overload 1/3).
  //
  // Args:
  //   a: Integer to add to accumulated value
  //
  // Returns:
  //   New accumulated value after addition
  int add(int a) {
    value += a;
    return value;
  }
  
  // Adds two integers to accumulated value (overload 2/3).
  //
  // Args:
  //   a: First integer to add
  //   b: Second integer to add
  //
  // Returns:
  //   New accumulated value after addition
  int add(int a, int b) {
    value += (a + b);
    return value;
  }
  
  // Adds two doubles and returns result (overload 3/3).
  //
  // Args:
  //   a: First double to add
  //   b: Second double to add
  //
  // Returns:
  //   Sum of a and b (doesn't affect accumulated value)
  //
  // Note: This overload has different return type and doesn't modify state.
  double add(double a, double b) {
    return a + b;
  }
  
  // Template method - multiplies two values of any type.
  //
  // Template parameter T: Type of the operands (int, double, etc.)
  //
  // Args:
  //   a: First operand of type T
  //   b: Second operand of type T
  //
  // Returns:
  //   Product of a and b
  //
  // Each instantiation (T=int, T=double, etc.) generates a separate
  // function in the IR with mangled name encoding the type.
  template<typename T>
  T multiply(T a, T b) {
    return a * b;
  }
};

// Template function - returns maximum of two values.
//
// Template parameter T: Type that supports comparison operator
//
// Args:
//   a: First value of type T
//   b: Second value of type T
//
// Returns:
//   The larger of a and b
//
// This function creates separate instantiations for each type used
// (int, double, etc.), each appearing as a distinct function in IR.
//
// Expected basic blocks per instantiation:
//   - entry: compare a > b
//   - if.then: return a when true
//   - if.end: return b when false
template<typename T>
T max_value(T a, T b) {
  if (a > b) {
    return a;
  }
  return b;
}

// Overloaded free function - processes integer (overload 1/3).
//
// Args:
//   x: Integer to process
//
// This demonstrates C++ function overloading at namespace scope.
// Each overload compiles to a different function with mangled name.
void process(int x) {
  std::cout << "Processing int: " << x << std::endl;
}

// Overloaded free function - processes double (overload 2/3).
//
// Args:
//   x: Double to process
void process(double x) {
  std::cout << "Processing double: " << x << std::endl;
}

// Overloaded free function - processes string (overload 3/3).
//
// Args:
//   x: String to process (passed by const reference for efficiency)
void process(const std::string& x) {
  std::cout << "Processing string: " << x << std::endl;
}

// Outer class demonstrating nested class structure.
//
// This tests:
//   - Nested class definitions (Inner inside Outer)
//   - Methods in both outer and inner classes
//   - Proper scoping and name mangling for nested entities
class Outer {
public:
  // Inner class - demonstrates nested class compilation.
  //
  // Nested classes compile to separate types with mangled names
  // that encode the outer class relationship.
  class Inner {
  public:
    // Prints message from inner class.
    //
    // This method will have a mangled name like _ZN5Outer5Inner5printEv
    // encoding the Outer::Inner::print hierarchy.
    void print() {
      std::cout << "Inner class print" << std::endl;
    }
  };
  
  // Processes something in outer class.
  //
  // This method has a simpler mangled name since it's directly
  // in Outer: _ZN5Outer7processEv
  void process() {
    std::cout << "Outer class process" << std::endl;
  }
};

// Lambda function assigned to variable.
//
// Lambdas in C++ are compiler-generated function objects (closures).
// The compiler creates a unique class with operator() for each lambda.
//
// This lambda:
//   - Takes no parameters: []()
//   - Returns int value 42
//   - Compiles to a callable object with mangled operator() name
//
// Expected: The lambda's operator() appears as a function in IR
auto lambda_test = []() {
  return 42;
};

// Main function exercising all C++ features.
//
// Returns:
//   Exit code (0 for success)
//
// This function demonstrates:
//   - Anonymous namespace function calls
//   - Method overloading with different signatures
//   - Template instantiation with multiple types
//   - Function overloading at namespace scope
//   - Nested class instantiation and usage
//   - Lambda invocation
//   - Range-based for loops (C++11)
//
// Expected: CSV contains all mangled function names from:
//   - increment(), get_count() (anonymous namespace)
//   - Calculator::add(int), Calculator::add(int,int), Calculator::add(double,double)
//   - Calculator::multiply<int>(int,int)
//   - max_value<int>(int,int), max_value<double>(double,double)
//   - process(int), process(double), process(string)
//   - Outer::process(), Outer::Inner::print()
//   - Lambda operator()
int main() {
  // Test static library functions in anonymous namespace
  // This creates basic blocks for loop (for.cond, for.body, for.inc)
  for (int i = 0; i < 3; i++) {
    increment();
  }
  std::cout << "Counter: " << get_count() << std::endl;
  
  // Test overloaded methods
  Calculator calc;
  calc.add(5);                    // Calls add(int)
  calc.add(3, 7);                 // Calls add(int, int)
  double result = calc.add(2.5, 3.5);  // Calls add(double, double)
  std::cout << "Add result: " << result << std::endl;
  
  // Test template functions with different type instantiations
  int int_max = max_value(10, 20);            // Instantiates max_value<int>
  double double_max = max_value(3.14, 2.71);  // Instantiates max_value<double>
  std::cout << "Max int: " << int_max << ", Max double: " << double_max << std::endl;
  
  // Test template methods
  int mult_result = calc.multiply(5, 6);  // Instantiates multiply<int>
  std::cout << "Multiply result: " << mult_result << std::endl;
  
  // Test overloaded functions with different parameter types
  process(42);                     // Calls process(int)
  process(3.14);                   // Calls process(double)
  process(std::string("Hello"));   // Calls process(const string&)
  
  // Test nested classes
  Outer outer;
  outer.process();
  Outer::Inner inner;
  inner.print();
  
  // Test lambda invocation
  int lambda_result = lambda_test();
  std::cout << "Lambda result: " << lambda_result << std::endl;
  
  // Vector with range-based for loop and control flow
  // This creates complex basic block structure:
  //   - Range-based for loop (iterator-based)
  //   - Nested if/else inside loop body
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
