#include "PhaseAnalysisPass.hh"

bool PhaseAnalysisPass::instrumentAllIRBasicBlocks(Module &M, 
                  int64_t &total_basic_block_count, const uint64_t threshold) {
  
  Function* bb_hook_function = M.getFunction("nugget_bb_hook");
  if (!bb_hook_function) {
    errs() << "Function nugget_bb_hook not found\n";
    return false;
  }

  IRBuilder<> builder(M.getContext());
  int64_t bb_id = -1;
  total_basic_block_count = 0;
  for (Function &F : M) {
    if (F.isDeclaration()) continue;

    // Skip function if it is one of the nugget helper functions
    if (std::find(nugget_functions.begin(), nugget_functions.end(),
                  F.getName().str()) != nugget_functions.end()) {
        continue;
    }

    for (BasicBlock &BB : F) {
      // Get the bb_id metadata
      bb_id = -1;
      Instruction *T = BB.getTerminator();
      if (T) {
          MDNode* bb_id_md = T->getMetadata(kBbIdKey);
          if (!bb_id_md) {
              errs() << "Warning: BasicBlock " << BB.getName()
                    << " in function " << F.getName()
                    << " is missing !bb.id metadata.\n";
              continue;
          }
          
          // Cast operand to MDString and extract the value
          MDString *bb_id_str = dyn_cast<MDString>(bb_id_md->getOperand(0));
          if (!bb_id_str) {
              errs() << "Warning: Invalid bb.id metadata format\n";
              continue;
          }
          bb_id = std::stoll(bb_id_str->getString().str());
      } else {
        errs() << "Could not find terminator for function " << F.getName() 
                                            << " bb " << BB.getName() << "\n";
        continue;
      }
      assert(bb_id != -1 && "bb_id should have been set from metadata");
      builder.SetInsertPoint(T);

      builder.CreateCall(bb_hook_function, {
        ConstantInt::get(Type::getInt64Ty(M.getContext()), BB.size()),
        ConstantInt::get(Type::getInt64Ty(M.getContext()), bb_id),
        ConstantInt::get(Type::getInt64Ty(M.getContext()), threshold),
      });
      total_basic_block_count++;
    }
  }
  return true;
}

PreservedAnalyses PhaseAnalysisPass::run(Module &M, ModuleAnalysisManager &) {
  LLVMContext &C = M.getContext();

  // First, instrument every basic block in every function
  // Then, insert nugget_init_call at the nugget_roi_begin_ function
  int64_t total_basic_block_count = -1;

  uint64_t threshold = std::stoull(GetOptionValue(options_, 
                                                          "interval_length"));
  if (!instrumentAllIRBasicBlocks(M, total_basic_block_count, 
                                                        threshold)) {
    report_fatal_error("Error instrumenting basic blocks");
  }
  assert(total_basic_block_count >= 1 && 
                "There should be at least one basic block instrumented");
  Value* total_bb_count_arg = ConstantInt::get(
        Type::getInt64Ty(C), total_basic_block_count);
  if (!instrumentRoiBegin(M, {total_bb_count_arg})) {
    report_fatal_error("Error instrumenting nugget_roi_begin_");
  }
  return PreservedAnalyses::all();

}