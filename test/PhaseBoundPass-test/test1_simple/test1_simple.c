// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Zhantong Qiu
// All rights reserved.
//
// test1_simple.c - Simple test for PhaseBoundPass marker instrumentation
//
// This test contains a program with identifiable basic blocks that we'll
// use as markers for warmup, start, and end of the region of interest.

#include <stdint.h>
#include <stdio.h>

// Forward declarations of marker hook functions
void nugget_warmup_marker_hook(void);
void nugget_start_marker_hook(void);
void nugget_end_marker_hook(void);
void nugget_roi_begin_(void);
void nugget_roi_end_(void);

// Simple computation function
uint64_t compute(uint64_t n) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i < n; i++) {
        sum += i * 2;
    }
    return sum;
}

// Helper function to create distinct basic blocks
uint64_t helper(uint64_t x) {
    if (x > 100) {
        return x * 2;
    } else {
        return x + 10;
    }
}

int main(void) {
    uint64_t result = 0;
    
    // Call ROI begin
    nugget_roi_begin_();
    
    // BB 0: Warmup marker location
    result = compute(5);
    
    // BB 1: Start marker location  
    result += helper(50);
    
    // BB 2: Main computation
    result += compute(10);
    result += helper(150);
    
    // BB 3: End marker location
    result += compute(3);
    
    // Call ROI end
    nugget_roi_end_();
    
    printf("Result: %lu\n", result);
    
    return 0;
}
