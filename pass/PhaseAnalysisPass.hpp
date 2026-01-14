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


#ifndef LLVM_TRANSFORMS_UTILS_PHASEANALYSIS_H
#define LLVM_TRANSFORMS_UTILS_PHASEANALYSIS_H

#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Transforms/Utils/Instrumentation.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include <bits/stdc++.h>
#include <string>
#include <vector>

namespace llvm {

class Module;

class PhaseAnalysisPass : public PassInfoMixin<PhaseAnalysisPass> {
public:
  struct basicBlockInfo {
    // function related
    std::string functionName;
    uint64_t functionId;
    bool ifStartOfFunction;

    // basic block related
    std::string basicBlockName;
    uint64_t basicBlockCount;
    uint64_t basicBlockId;

    // pointers to the basic block and function
    BasicBlock* basicBlock;
    Function* function;
  };

private:
  uint64_t totalFunctionCount;
  uint64_t totalBasicBlockCount;
  uint64_t threshold;
  bool usingPapiToAnalyze = false;

  std::vector<basicBlockInfo> basicBlockList;

  // check if the function is empty by checking the number of 
  bool emptyFunction(Function &F);

  void modifyROIFunctionsForBBV(Module &M);

  void instrumentBBVAnalysis(Module &M);
  void instrumentPapiAnalysis(Module &M);
   
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_PHASEANALYSIS_H