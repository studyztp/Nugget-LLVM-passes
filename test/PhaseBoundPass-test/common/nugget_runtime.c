// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Zhantong Qiu
// All rights reserved.
//
// nugget_runtime.c - Runtime stub functions for PhaseBoundPass testing
//
// This file provides the runtime functions that PhaseBoundPass expects
// to find in the module. These are stub implementations used for testing.

#include <stdint.h>
#include <stdio.h>

// nugget_init - Initialize the runtime with marker counts.
//
// Called at the end of nugget_roi_begin_ after PhaseBoundPass instruments it.
// In production, this would set up tracking for the specified number of
// warmup, start, and end marker executions.
//
// Args:
//   warmup_count: Number of times to execute warmup marker before start
//   start_count: Number of times to execute start marker before measurement
//   end_count: Number of times to execute end marker before stopping
void nugget_init(uint64_t warmup_count, uint64_t start_count, uint64_t end_count) {
    // Stub implementation
    (void)warmup_count;
    (void)start_count;
    (void)end_count;
}

// nugget_roi_begin_ - Mark the beginning of the region of interest.
//
// Called by the user program to indicate the start of the ROI.
// PhaseBoundPass will insert a call to nugget_init at the end of this function.
void nugget_roi_begin_(void) {
    // Stub - PhaseBoundPass will insert nugget_init call here
}

// nugget_roi_end_ - Mark the end of the region of interest.
void nugget_roi_end_(void) {
    // Stub implementation
}

// nugget_warmup_marker_hook - Called at warmup marker basic block.
//
// PhaseBoundPass inserts this call at the warmup marker BB.
void nugget_warmup_marker_hook(void) {
    // Stub implementation
}

// nugget_start_marker_hook - Called at start marker basic block.
//
// PhaseBoundPass inserts this call at the start marker BB.
void nugget_start_marker_hook(void) {
    // Stub implementation
}

// nugget_end_marker_hook - Called at end marker basic block.
//
// PhaseBoundPass inserts this call at the end marker BB.
void nugget_end_marker_hook(void) {
    // Stub implementation
}
