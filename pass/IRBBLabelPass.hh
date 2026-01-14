#include "common.hh"

class IRBBLabelPass : public PassInfoMixin<IRBBLabelPass> {
  public:
    IRBBLabelPass() = default;
    ~IRBBLabelPass() = default;

    struct basicBlockInfo {
        // function information
        std::string functionName;
        uint64_t functionId;

        // basic block information
        std::string basicBlockName;
        uint64_t basicBlockInstCount;
        uint64_t basicBlockId;
    };

  private:
    std::vector<basicBlockInfo> bbInfoList;

  public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &);
};
