#pragma once

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

namespace cascade::codegen {

struct LanePolicy {
  llvm::Type *scalar_type;
  unsigned width;
  llvm::Type *slot_type;

  LanePolicy(llvm::Type *scalar, unsigned w)
      : scalar_type(scalar), width(w),
        slot_type(w > 1 ? static_cast<llvm::Type *>(
                              llvm::VectorType::get(scalar, w, false))
                        : scalar) {}

  void store(llvm::IRBuilder<> &b, llvm::Value *out_ptr, llvm::Value *idx,
             llvm::Value *val) const {

    llvm::Value *addr = b.CreateGEP(scalar_type, out_ptr, idx);
    if (width == 1) {
      b.CreateStore(val, addr);
      return;
    }

    const llvm::DataLayout &dl =
        b.GetInsertBlock()->getModule()->getDataLayout();
    b.CreateAlignedStore(val, addr, dl.getABITypeAlign(scalar_type));
  }
};

}
