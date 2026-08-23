#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

namespace cascade::codegen {

inline llvm::FunctionType *emit_signature(llvm::LLVMContext &ctx,
                                          uint8_t n_axes) {
  llvm::Type *ptr = llvm::PointerType::get(ctx, 0);
  llvm::Type *i64 = llvm::Type::getInt64Ty(ctx);
  std::vector<llvm::Type *> params = {ptr, ptr};
  params.insert(params.end(), n_axes, i64);
  return llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), params, false);
}

inline llvm::Function *emit_entry_block(llvm::Module &mod,
                                        llvm::FunctionType *sig,
                                        const std::string &name) {
  llvm::Function *fn =
      llvm::Function::Create(sig, llvm::Function::ExternalLinkage, name, mod);
  llvm::BasicBlock::Create(mod.getContext(), "entry", fn);
  return fn;
}

}
