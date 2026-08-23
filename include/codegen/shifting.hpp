#pragma once

#include <cstdint>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "codegen/registers.hpp"

namespace cascade::codegen {

inline void emit_sum_shift(RegisterFile &regs, uint32_t start,
                           uint32_t length) {
  for (uint32_t i = 0; i + 1 < length; ++i) {
    regs.store(start + i, regs.builder.CreateFAdd(regs.load(start + i),
                                                  regs.load(start + i + 1)));
  }
}

inline void emit_prod_shift(RegisterFile &regs, uint32_t start,
                            uint32_t length) {
  for (uint32_t i = 0; i + 1 < length; ++i) {
    regs.store(start + i, regs.builder.CreateFMul(regs.load(start + i),
                                                  regs.load(start + i + 1)));
  }
}

inline void emit_trig_shift(RegisterFile &regs, uint32_t start,
                            uint32_t length) {
  const uint32_t half = length / 2;
  for (uint32_t i = 0; i + 1 < half; ++i) {
    llvm::Value *sin_i = regs.load(start + i);
    llvm::Value *cos_i = regs.load(start + half + i);
    llvm::Value *sin_ip1 = regs.load(start + i + 1);
    llvm::Value *cos_ip1 = regs.load(start + half + i + 1);
    llvm::Value *new_sin =
        regs.builder.CreateFAdd(regs.builder.CreateFMul(sin_i, cos_ip1),
                                regs.builder.CreateFMul(cos_i, sin_ip1));
    llvm::Value *new_cos =
        regs.builder.CreateFSub(regs.builder.CreateFMul(cos_i, cos_ip1),
                                regs.builder.CreateFMul(sin_i, sin_ip1));
    regs.store(start + i, new_sin);
    regs.store(start + half + i, new_cos);
  }
}

}
