#ifndef _PHASEBOUNDPASS_HH_
#define _PHASEBOUNDPASS_HH_

#include "common.hh"

const std::vector<Options> PhaseBoundPassOptions = {
    // The BB ID of the warmup marker basic block
    {"warmup_marker_bb_id", ""},
    // Number of executions of the warmup marker basic block before the warmup
    // point is reached
    {"warmup_marker_count", ""},
    // The BB ID of the start marker basic block
    {"start_marker_bb_id", ""},
    // Number of executions of the start marker basic block before the start
    // point is reached
    {"start_marker_count", ""},
    // The BB ID of the end marker basic block
    {"end_marker_bb_id", ""},
    // Number of executions of the end marker basic block before the end
    // point is reached
    {"end_marker_count", ""} 
};

class PhaseBoundPass : public PassInfoMixin<PhaseBoundPass> {
  public:
    PhaseBoundPass(std::vector<Options> Options)
    {
        options_ = Options;
    }
    ~PhaseBoundPass() = default;
  private:
    std::vector<Options> options_;
    bool instrumentMarkerBBs(Module &M,
            const uint64_t warmup_marker_bb_id,
            const uint64_t start_marker_bb_id,
            const uint64_t end_marker_bb_id);
  public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &);
};

#endif // _PHASEBOUNDPASS_HH_
