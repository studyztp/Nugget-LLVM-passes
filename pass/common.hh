#include "llvm/Passes/PassBuilder.h"
// PassPlugin.h is in a different location depending on LLVM version
#if __has_include("llvm/Plugins/PassPlugin.h")
  #include "llvm/Plugins/PassPlugin.h"
#else
  #include "llvm/Passes/PassPlugin.h"
#endif

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Instructions.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <vector>

using namespace llvm;

static constexpr const char *BBIDKey = "bb.id";
