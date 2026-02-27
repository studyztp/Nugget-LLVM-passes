#ifndef _IRINFODUMPPASS_HH_
#define _IRINFODUMPPASS_HH_

#include "common.hh"

// IRInfoDumpPass - Dumps IR-level structural information to JSON.
//
// For each function: lists all allocas with their name, type, and byte size.
// For the module: lists all global variables.
// For each basic block: lists BB ID, function, predecessors, successors,
//   and the textual IR instructions.
// Debug variable mapping: maps allocas to source variable names via
//   dbg.declare / dbg.value intrinsics.
//
// Usage:
//   opt -load-pass-plugin=NuggetPasses.so \
//       -passes="ir-info-dump-pass<output_json=ir_info.json>" \
//       input.ll -o /dev/null
//
// The pass requires IRBBLabelPass to have been run first (!bb.id metadata).

static const std::vector<Options> IRInfoDumpPassOptions = {
    {"output_json", "ir_info.json"}
};

class IRInfoDumpPass : public PassInfoMixin<IRInfoDumpPass> {
  public:
    IRInfoDumpPass(std::vector<Options> Options) {
        options_ = Options;
    }
    ~IRInfoDumpPass() = default;

  private:
    std::vector<Options> options_;

  public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &);
};

#endif // _IRINFODUMPPASS_HH_
