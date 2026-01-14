#ifndef _IRBBLABELPASS_HH_
#define _IRBBLABELPASS_HH_
#include "common.hh"

static const std::vector<Options> IRBBLabelPassOptions = {
    {"output_csv", "bb_info.csv"} // default output file name
};

class IRBBLabelPass : public PassInfoMixin<IRBBLabelPass> {
  // This class implements a ModulePass that labels each IR basic block with a 
  // unique ID via metadata and collects information about each basic block.
  // The collected information is then output to a CSV file specified by the
  // "output_csv" option. The options are provided via parameterized pass name.
  // They are stored in the 'options_' member variable.
  public:
    IRBBLabelPass(std::vector<Options> Options) {
        options_ = Options;
    }
    ~IRBBLabelPass() = default;

    struct BasicBlockInfo {
        // function information
        std::string function_name;
        uint64_t function_id;

        // basic block information
        std::string basic_block_name;
        uint64_t basic_block_inst_count;
        uint64_t basic_block_id;
    };

  private:
    std::vector<BasicBlockInfo> bb_info_list_;
    std::vector<Options> options_;

  public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &);
};

#endif // _IRBBLABELPASS_HH_
