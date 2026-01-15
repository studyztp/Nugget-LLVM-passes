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
// Test Case 1: Simple PhaseAnalysisPass instrumentation test
//
// Purpose: Verify that PhaseAnalysisPass correctly:
//   1. Inserts nugget_init_ call at the end of nugget_roi_begin_
//   2. Inserts nugget_bb_hook_ calls in all labeled basic blocks
//
// This test validates the core instrumentation functionality with a simple
// program that has a few basic blocks and uses the nugget ROI markers.

#include <stdio.h>

// External declarations for nugget runtime functions
// These are defined in nugget_runtime.c
extern void nugget_roi_begin_(void);
extern void nugget_roi_end_(void);

// Simple function with multiple basic blocks
int compute(int x) {
    int result = 0;
    
    // Creates if.then and if.else basic blocks
    if (x > 10) {
        result = x * 2;
    } else {
        result = x + 5;
    }
    
    // Creates a loop with header, body, and exit blocks
    for (int i = 0; i < x; i++) {
        result += i;
    }
    
    return result;
}

// Another simple function
int helper(int a, int b) {
    return a + b;
}

int main() {
    int value = 15;
    
    // Mark the beginning of the region of interest
    // PhaseAnalysisPass should insert nugget_init_ call here
    nugget_roi_begin_();
    
    // Some computation that will be instrumented
    int result = compute(value);
    result = helper(result, 10);
    
    // Conditional to add more basic blocks
    if (result > 100) {
        printf("Result is large: %d\n", result);
    } else {
        printf("Result is small: %d\n", result);
    }
    
    // Mark the end of the region of interest
    nugget_roi_end_();
    
    return 0;
}
