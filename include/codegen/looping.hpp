#pragma once

#include <cstdint>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

#include "codegen/registers.hpp"

namespace cascade::codegen {

struct LoopHandle {
  llvm::PHINode *idx;
  llvm::BasicBlock *header;
  llvm::BasicBlock *exit;
  int64_t stride;
};

inline LoopHandle begin_loop(RegisterFile &regs, llvm::Value *n,
                             int64_t stride = 1) {
  auto &b = regs.builder;
  llvm::Function *fn = b.GetInsertBlock()->getParent();
  llvm::BasicBlock *pre = b.GetInsertBlock();
  llvm::BasicBlock *h = llvm::BasicBlock::Create(b.getContext(), "loop.h", fn);
  llvm::BasicBlock *body =
      llvm::BasicBlock::Create(b.getContext(), "loop.b", fn);
  llvm::BasicBlock *x = llvm::BasicBlock::Create(b.getContext(), "loop.x", fn);

  b.CreateBr(h);
  b.SetInsertPoint(h);
  llvm::PHINode *idx = b.CreatePHI(b.getInt64Ty(), 2, "i");
  idx->addIncoming(b.getInt64(0), pre);
  b.CreateCondBr(b.CreateICmpSLT(idx, n), body, x);

  b.SetInsertPoint(body);
  regs.indices.push_back(idx);
  return LoopHandle{idx, h, x, stride};
}

inline void end_loop(RegisterFile &regs, LoopHandle handle) {
  auto &b = regs.builder;
  llvm::BasicBlock *latch = b.GetInsertBlock();
  llvm::Value *nxt = b.CreateAdd(
      handle.idx, llvm::ConstantInt::get(b.getInt64Ty(), handle.stride));
  handle.idx->addIncoming(nxt, latch);
  b.CreateBr(handle.header);
  b.SetInsertPoint(handle.exit);
  regs.indices.pop_back();
}

}
