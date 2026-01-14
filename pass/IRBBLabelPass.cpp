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


#include "IRBBLabelPass.hh"

PreservedAnalyses IRBBLabelPass::run(Module &M, ModuleAnalysisManager &) {
    LLVMContext &C = M.getContext();
    uint64_t function_counter = 0;
    uint64_t basic_block_global_counter = 0;
    bb_info_list_.clear();

    for (Function &F : M) {
        if (F.isDeclaration()) continue; // skip function declarations

        for (BasicBlock &BB : F) {
            // Create unique BB ID
            uint64_t bb_id = basic_block_global_counter++;

            // Set metadata on the terminator instruction
            Instruction *T = BB.getTerminator();
            if (T) {
                MDNode *N = MDNode::get(C, MDString::get(C, std::to_string(bb_id)));
                T->setMetadata(kBbIdKey, N);
            } else {
                // Print warning if no terminator
                errs() << "Warning: BasicBlock " << BB.getName()
                        << " in function " << F.getName()
                        << " has no terminator instruction.\n";
            }

            // Collect BB info
            BasicBlockInfo bb_info;
            bb_info.function_name = F.getName().str();
            bb_info.function_id = function_counter;
            bb_info.basic_block_name = BB.getName().str();
            bb_info.basic_block_inst_count = BB.size();
            bb_info.basic_block_id = bb_id;

            bb_info_list_.push_back(bb_info);
        }
        function_counter++;
    }

    // Output collected BB info into a CSV file
    std::error_code EC;
    raw_fd_ostream csv_file(options_[0].option_value, EC, sys::fs::OF_Text);
    if (EC) {
        errs() << "Error opening file " << options_[0].option_value << ": " << EC.message() << "\n";
        return PreservedAnalyses::all();
    }
    // Write CSV header
    csv_file << "FunctionName,FunctionID,BasicBlockName,BasicBlockInstCount,BasicBlockID\n";
    // Write BB info
    for (const auto &bb_info : bb_info_list_) {
        csv_file << bb_info.function_name << ","
                << bb_info.function_id << ","
                << bb_info.basic_block_name << ","
                << bb_info.basic_block_inst_count << ","
                << bb_info.basic_block_id << "\n";
    }
    csv_file.close();
    return PreservedAnalyses::all();
}
