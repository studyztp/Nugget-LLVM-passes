#include "IRInfoDumpPass.hh"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/DataLayout.h"
#include <sstream>

static std::string typeToString(Type *T) {
    std::string s;
    raw_string_ostream rso(s);
    T->print(rso);
    return rso.str();
}

static std::string instrToString(const Instruction &I) {
    std::string s;
    raw_string_ostream rso(s);
    I.print(rso);
    return rso.str();
}

static std::string escapeJson(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

static int64_t getBbId(const BasicBlock &BB) {
    const Instruction *T = BB.getTerminator();
    if (!T) return -1;
    MDNode *md = T->getMetadata(kBbIdKey);
    if (!md) return -1;
    MDString *s = dyn_cast<MDString>(md->getOperand(0));
    if (!s) return -1;
    return std::stoll(s->getString().str());
}

PreservedAnalyses IRInfoDumpPass::run(Module &M, ModuleAnalysisManager &) {
    std::string output_file = GetOptionValue(options_, "output_json");
    DEBUG_PRINT("IRInfoDumpPass: output_json=" << output_file);

    const DataLayout &DL = M.getDataLayout();

    std::error_code EC;
    raw_fd_ostream out(output_file, EC, sys::fs::OF_Text);
    if (EC) {
        report_fatal_error(Twine("Cannot open output file: ") + output_file +
                           " (" + EC.message() + ")");
    }

    out << "{\n";

    // ---- Global variables ----
    out << "  \"globals\": [\n";
    bool first_global = true;
    for (const GlobalVariable &GV : M.globals()) {
        if (GV.isDeclaration()) continue;
        if (!first_global) out << ",\n";
        first_global = false;

        Type *elem_ty = GV.getValueType();
        uint64_t byte_size = DL.getTypeAllocSize(elem_ty);

        out << "    {"
            << "\"name\": \"" << escapeJson(GV.getName().str()) << "\""
            << ", \"type\": \"" << escapeJson(typeToString(elem_ty)) << "\""
            << ", \"byte_size\": " << byte_size
            << ", \"is_constant\": " << (GV.isConstant() ? "true" : "false")
            << ", \"linkage\": \"" << escapeJson(std::string(
                   GV.hasInternalLinkage() ? "internal" :
                   GV.hasPrivateLinkage()  ? "private"  :
                   GV.hasExternalLinkage() ? "external" : "other")) << "\""
            << "}";
    }
    out << "\n  ],\n";

    // ---- Functions and their allocas / BBs ----
    out << "  \"functions\": [\n";
    bool first_func = true;
    for (Function &F : M) {
        if (F.isDeclaration()) continue;
        if (std::find(nugget_functions.begin(), nugget_functions.end(),
                      F.getName().str()) != nugget_functions.end())
            continue;

        if (!first_func) out << ",\n";
        first_func = false;

        out << "    {\n";
        out << "      \"name\": \"" << escapeJson(F.getName().str()) << "\",\n";

        // Collect allocas
        out << "      \"allocas\": [\n";
        bool first_alloca = true;

        // Build a map from alloca -> source variable name via dbg.declare
        std::map<const Value *, std::string> alloca_to_src_name;
        for (const BasicBlock &BB : F) {
            for (const Instruction &I : BB) {
                if (const auto *DDI = dyn_cast<DbgDeclareInst>(&I)) {
                    if (Value *addr = DDI->getAddress()) {
                        DILocalVariable *var = DDI->getVariable();
                        if (var)
                            alloca_to_src_name[addr] = var->getName().str();
                    }
                }
            }
        }

        for (const BasicBlock &BB : F) {
            for (const Instruction &I : BB) {
                if (const auto *AI = dyn_cast<AllocaInst>(&I)) {
                    if (!first_alloca) out << ",\n";
                    first_alloca = false;

                    Type *alloc_ty = AI->getAllocatedType();
                    uint64_t byte_size = DL.getTypeAllocSize(alloc_ty);

                    std::string ir_name = AI->hasName() ?
                        AI->getName().str() : "<unnamed>";
                    std::string src_name = "";
                    auto it = alloca_to_src_name.find(AI);
                    if (it != alloca_to_src_name.end())
                        src_name = it->second;

                    out << "        {"
                        << "\"ir_name\": \"" << escapeJson(ir_name) << "\""
                        << ", \"src_name\": \"" << escapeJson(src_name) << "\""
                        << ", \"type\": \""
                        << escapeJson(typeToString(alloc_ty)) << "\""
                        << ", \"byte_size\": " << byte_size
                        << "}";
                }
            }
        }
        out << "\n      ],\n";

        // Collect basic blocks
        out << "      \"basic_blocks\": [\n";
        bool first_bb = true;
        for (const BasicBlock &BB : F) {
            if (!first_bb) out << ",\n";
            first_bb = false;

            int64_t bb_id = getBbId(BB);

            out << "        {\n";
            out << "          \"bb_id\": " << bb_id << ",\n";
            out << "          \"ir_name\": \""
                << escapeJson(BB.getName().str()) << "\",\n";

            // Predecessors (as bb_ids)
            out << "          \"predecessors\": [";
            bool first_pred = true;
            for (const BasicBlock *Pred : predecessors(&BB)) {
                if (!first_pred) out << ", ";
                first_pred = false;
                out << getBbId(*Pred);
            }
            out << "],\n";

            // Successors (as bb_ids)
            out << "          \"successors\": [";
            bool first_succ = true;
            for (const BasicBlock *Succ : successors(BB.getTerminator())) {
                if (!first_succ) out << ", ";
                first_succ = false;
                out << getBbId(*Succ);
            }
            out << "],\n";

            // Instructions
            out << "          \"instructions\": [\n";
            bool first_inst = true;
            for (const Instruction &I : BB) {
                if (!first_inst) out << ",\n";
                first_inst = false;
                out << "            \""
                    << escapeJson(instrToString(I)) << "\"";
            }
            out << "\n          ]\n";
            out << "        }";
        }
        out << "\n      ]\n";
        out << "    }";
    }
    out << "\n  ]\n";
    out << "}\n";

    out.flush();
    errs() << "IRInfoDumpPass: wrote " << output_file << "\n";
    return PreservedAnalyses::all();
}
