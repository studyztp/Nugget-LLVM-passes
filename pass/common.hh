#ifndef _COMMON_HH_
#define _COMMON_HH_

// Debug print macro - only prints if DEBUG is defined during compilation
#ifdef DEBUG
#define DEBUG_PRINT(msg) errs() << "DEBUG: " << msg << "\n"
#else
#define DEBUG_PRINT(msg)
#endif

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

#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <vector>

using namespace llvm;

// Metadata key for IR basic block ID
static constexpr const char *kBbIdKey = "bb.id";

// Option structure to hold pass parameters
struct Options {
  std::string option_name;
  std::string option_value;
  bool is_set() const {
    return !option_value.empty();
  }
};

// Function to parse options from parameter string
static Expected<std::vector<Options>> ParseOptions(StringRef Params, const std::vector<Options> TargetOptions) {
  // Since the maximum options are less than 5, we don't need to optimize the paring.
  
  SmallVector<StringRef, 8> Items;
  Params.split(Items, ';', /*MaxSplit=*/-1, /*KeepEmpty=*/false);

  std::vector<Options> ReturnOptions = TargetOptions;

  for (StringRef It : Items) {
    It = It.trim();
    if (It.empty())
      continue;

    auto KV = It.split('=');
    StringRef K = KV.first.trim();
    StringRef V = KV.second.trim();

    if (K.empty() || V.empty()) {
      return make_error<StringError>("invalid option: " + It, inconvertibleErrorCode());
    }
    
    bool Found = false;
    for (auto &Opt : ReturnOptions) {
      DEBUG_PRINT("comparing K='" << K.str() << "' with Opt.option_name='" << Opt.option_name << "'");
      if (Opt.option_name == K.str()) {
        Opt.option_value = V.str();
        DEBUG_PRINT("Match found! Set " << Opt.option_name << " = " << Opt.option_value);
        Found = true;
        break;
      }
    }
    if (!Found) {
      return make_error<StringError>("unknown option: " + K.str(), inconvertibleErrorCode());
    }
  }

  // Check all required options are set
  for (const auto &Opt : ReturnOptions) {
    DEBUG_PRINT("Checking if set: " << Opt.option_name << " = '" << Opt.option_value << "' is_set=" << (Opt.is_set() ? "true" : "false"));
    if (!Opt.is_set()) {
      return make_error<StringError>("missing required option: " + Opt.option_name, inconvertibleErrorCode());
    }
  }

  DEBUG_PRINT("parseOptions returning successfully");
  return ReturnOptions;
}

// Function to match parameterized pass name and extract options
static Expected<std::vector<Options>> MatchParamPass(StringRef Name, StringRef Base, const std::vector<Options> TargetOptions) {
  DEBUG_PRINT("MatchParamPass: Name='" << Name << "' Base='" << Base << "'");
  if (Name == Base) {
    // No parameters
    for (const auto &Opt : TargetOptions) {
      if (!Opt.is_set()) {
        return make_error<StringError>("missing required option: " + Opt.option_name, inconvertibleErrorCode());
      }
    }
    return std::vector<Options>(TargetOptions);
  }
  if (!Name.starts_with(Base)) {
    return make_error<StringError>("name not matched", inconvertibleErrorCode());
  }
  if (Name.size() <= Base.size() + 2) { // must fit "<>"
    return make_error<StringError>("malformed parameterized pass name", inconvertibleErrorCode());
  }
  if (Name[Base.size()] != '<' || !Name.ends_with(">")) {
    return make_error<StringError>("malformed parameterized pass name", inconvertibleErrorCode());
  }
  StringRef Params = Name.drop_front(Base.size() + 1).drop_back(1);
  DEBUG_PRINT("extracted Params='" << Params << "'");
  return ParseOptions(Params, TargetOptions);
}

#endif // _COMMON_HH_
