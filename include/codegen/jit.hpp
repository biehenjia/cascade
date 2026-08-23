#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetSelect.h>

#include "codegen/emit.hpp"

namespace cascade::codegen {

inline void ensure_native_target_initialized() {
  static bool done = []() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    return true;
  }();
  (void)done;
}

inline void optimize(llvm::Module &mod) {
  llvm::LoopAnalysisManager lam;
  llvm::FunctionAnalysisManager fam;
  llvm::CGSCCAnalysisManager cgam;
  llvm::ModuleAnalysisManager mam;

  llvm::PassBuilder pb;
  pb.registerModuleAnalyses(mam);
  pb.registerCGSCCAnalyses(cgam);
  pb.registerFunctionAnalyses(fam);
  pb.registerLoopAnalyses(lam);
  pb.crossRegisterProxies(lam, fam, cgam, mam);

  llvm::ModulePassManager mpm;
  llvm::cantFail(pb.parsePassPipeline(
      mpm, "function(mem2reg,instcombine,simplifycfg,reassociate,loop-mssa("
           "licm),loop-vectorize,slp-vectorizer)"));
  mpm.run(mod, mam);
}

template <std::size_t N, std::size_t... Is>
auto kernel_fn_type(std::index_sequence<Is...>)
    -> void (*)(double *, double *, decltype((void(Is)), int64_t{})...);

template <std::size_t N>
using KernelFnPtr = decltype(kernel_fn_type<N>(std::make_index_sequence<N>{}));

struct JitEngine {
  std::unique_ptr<llvm::orc::LLJIT> jit;

  explicit JitEngine(EmittedModule &&em) {
    ensure_native_target_initialized();
    optimize(*em.module);
    jit = llvm::cantFail(llvm::orc::LLJITBuilder().create());
    llvm::orc::ThreadSafeModule tsm(std::move(em.module), std::move(em.ctx));
    llvm::cantFail(jit->addIRModule(std::move(tsm)));
  }

  template <std::size_t N> KernelFnPtr<N> get(const std::string &name) {
    auto sym = llvm::cantFail(jit->lookup(name));
    return reinterpret_cast<KernelFnPtr<N>>(sym.getValue());
  }
};

}
