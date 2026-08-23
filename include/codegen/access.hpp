#pragma once

#include <cstdint>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "codegen/registers.hpp"
#include "core/node.hpp"

namespace cascade::codegen {

inline llvm::Value *access_ir(RegisterFile &regs, Kind kind, uint32_t start,
                              uint32_t length) {
  switch (kind) {
  case Kind::Sum:
  case Kind::Prod:
  case Kind::Sin:
    return regs.load(start);
  case Kind::Cos:
    return regs.load(start + length / 2);
  case Kind::Tan:
    return regs.builder.CreateFDiv(regs.load(start),
                                   regs.load(start + length / 2));
  case Kind::Cot:
    return regs.builder.CreateFDiv(regs.load(start + length / 2),
                                   regs.load(start));
  default:
    return nullptr;
  }
}

}
