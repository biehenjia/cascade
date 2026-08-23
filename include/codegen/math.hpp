#pragma once

#include <initializer_list>

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>

#include "codegen/policy.hpp"

namespace cascade::codegen {

inline llvm::Value *splat(const LanePolicy &policy, double v) {
  llvm::Constant *c = llvm::ConstantFP::get(policy.scalar_type, v);
  if (policy.width == 1)
    return c;
  return llvm::ConstantVector::getSplat(
      llvm::ElementCount::getFixed(policy.width), c);
}

inline llvm::Value *call_intrinsic(llvm::IRBuilder<> &b, llvm::Module &mod,
                                   const LanePolicy &policy,
                                   llvm::Intrinsic::ID id,
                                   std::initializer_list<llvm::Value *> args) {
  llvm::Function *fn =
      llvm::Intrinsic::getOrInsertDeclaration(&mod, id, {policy.slot_type});
  return b.CreateCall(fn, args);
}

}
