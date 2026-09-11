#pragma once
#include <llvm/ADT/SmallString.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <string>
namespace cascade::codegen {
inline void optimize(llvm::Module& mod, llvm::TargetMachine& tm,
                     llvm::OptimizationLevel level = llvm::OptimizationLevel::O3, bool vectorize = true) {
    mod.setDataLayout(tm.createDataLayout());
    mod.setTargetTriple(tm.getTargetTriple());
    llvm::PipelineTuningOptions pto;
    pto.LoopInterleaving = true;
    pto.LoopUnrolling = true;
    pto.LoopVectorization = vectorize;
    pto.SLPVectorization = vectorize;
    llvm::PassBuilder pb(&tm, pto);
    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cgam;
    llvm::ModuleAnalysisManager mam;
    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.crossRegisterProxies(lam, fam, cgam, mam);
    llvm::ModulePassManager mpm = level == llvm::OptimizationLevel::O0 ? pb.buildO0DefaultPipeline(level)
                                                                       : pb.buildPerModuleDefaultPipeline(level);
    mpm.run(mod, mam);
}
inline std::string emit_assembly(llvm::Module& mod, llvm::TargetMachine& tm) {
    llvm::SmallString<0> buf;
    llvm::raw_svector_ostream os(buf);
    llvm::legacy::PassManager pm;
    if (tm.addPassesToEmitFile(pm, os, nullptr, llvm::CodeGenFileType::AssemblyFile))
        return "<target cannot emit assembly>";
    pm.run(mod);
    return std::string(buf);
}
} // namespace cascade::codegen
