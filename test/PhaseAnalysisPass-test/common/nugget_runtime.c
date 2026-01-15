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
// Nugget Runtime Library - Stub functions for PhaseAnalysisPass testing
//
// This file provides the runtime functions that PhaseAnalysisPass expects
// to find in the module. These are stub implementations used for testing
// that the pass correctly instruments calls to these functions.
//
// In actual use, these would be replaced with real implementations that
// collect runtime data for phase analysis.

#include <stdint.h>

// nugget_init - Initialize the runtime with total basic block count.
//
// Called at the end of nugget_roi_begin_ after PhaseAnalysisPass instruments it.
// In production, this would allocate data structures for tracking BB execution.
//
// Args:
//   total_bb_count: Total number of basic blocks in the instrumented program
void nugget_init(uint64_t total_bb_count) {
    // Stub implementation - does nothing in test
    // Production would: allocate counters array of size total_bb_count
    (void)total_bb_count;
}

// nugget_roi_begin_ - Mark the beginning of the region of interest.
//
// Called by the user program to indicate the start of the ROI.
// PhaseAnalysisPass will insert a call to nugget_init_ at the end of this
// function (before the return).
void nugget_roi_begin_(void) {
    // Stub implementation - does nothing
    // The pass will insert nugget_init_ call here
}

// nugget_roi_end_ - Mark the end of the region of interest.
//
// Called by the user program to indicate the end of the ROI.
// In production, this would finalize and output collected data.
void nugget_roi_end_(void) {
    // Stub implementation - does nothing
    // Production would: dump collected phase data
}

// nugget_bb_hook - Called at the end of each basic block.
//
// PhaseAnalysisPass inserts a call to this function at the end of every
// labeled basic block. This is the main instrumentation hook.
//
// Args:
//   inst_count: Number of instructions in the basic block
//   bb_id: Unique identifier of the basic block (from IRBBLabelPass)
//   threshold: Interval length for phase detection
void nugget_bb_hook(uint64_t inst_count, uint64_t bb_id, uint64_t threshold) {
    // Stub implementation - does nothing in test
    // Production would: accumulate inst_count, record bb_id execution
    (void)inst_count;
    (void)bb_id;
    (void)threshold;
}
