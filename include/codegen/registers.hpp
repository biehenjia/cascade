#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "codegen/policy.hpp"

namespace cascade::codegen {

struct RegisterFile {
  llvm::IRBuilder<> &builder;
  const LanePolicy &policy;
  std::vector<llvm::Value *> slots;
  std::vector<llvm::Value *> constants;
  std::vector<llvm::Value *> indices;

  llvm::Value *result = nullptr;
  llvm::Value *tape = nullptr;
  std::vector<llvm::Value *> bounds;

  RegisterFile(llvm::IRBuilder<> &b, const LanePolicy &pol, uint32_t tape_size)
      : builder(b), policy(pol) {
    slots.reserve(tape_size);
    for (uint32_t i = 0; i < tape_size; ++i) {
      slots.push_back(builder.CreateAlloca(policy.slot_type, nullptr,
                                           ("r" + std::to_string(i)).c_str()));
    }
  }

  llvm::Value *load(uint32_t i) const {
    return builder.CreateLoad(policy.slot_type, slots[i]);
  }
  void store(uint32_t i, llvm::Value *v) { builder.CreateStore(v, slots[i]); }

  void bind(llvm::Function *fn, uint32_t n_axes) {
    auto it = fn->arg_begin();
    result = &*it++;
    tape = &*it++;
    bounds.clear();
    for (uint32_t i = 0; i < n_axes; ++i)
      bounds.push_back(&*it++);
  }

  void prologue() {
    constants.reserve(slots.size());
    for (uint32_t i = 0; i < slots.size(); ++i) {
      llvm::Value *addr =
          builder.CreateGEP(policy.slot_type, tape, builder.getInt64(i));
      llvm::Value *v = builder.CreateLoad(policy.slot_type, addr);
      constants.push_back(v);
      store(i, v);
    }
  }

  void store_result(llvm::Value *val) {
    policy.store(builder, result, linearize(), val);
  }

  llvm::Value *linearize() const {
    llvm::Value *acc = indices[0];
    for (std::size_t k = 1; k < indices.size(); ++k) {
      acc = builder.CreateAdd(builder.CreateMul(acc, bounds[k]), indices[k]);
    }
    return acc;
  }
};

}
