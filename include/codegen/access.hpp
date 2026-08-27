#pragma once

#include <cstddef>
#include <initializer_list>
#include <cstdint>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "codegen/math.hpp"
#include "codegen/policy.hpp"
#include "codegen/registers.hpp"
#include "core/node.hpp"

namespace cascade::codegen {

// Reads a node's current value out of its own tape range. Uniform over chain
// and algebraic kinds: an algebraic node owns one slot per operand (see
// plan.hpp), so reading one is a combinator over its slots rather than a walk
// of the tree. Mirrors pycr's dispatch_access / access_cre_* table.
inline llvm::Value *access_ir(RegisterFile &regs, llvm::Module &mod,
                              const LanePolicy &policy, Kind kind,
                              uint32_t start, uint32_t length) {
  auto intr = [&](llvm::Intrinsic::ID id,
                  std::initializer_list<llvm::Value *> args) {
    return call_intrinsic(regs.builder, mod, policy, id, args);
  };

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
  case Kind::EAdd: {
    llvm::Value *r = regs.load(start);
    for (uint32_t k = 1; k < length; ++k)
      r = regs.builder.CreateFAdd(r, regs.load(start + k));
    return r;
  }
  case Kind::EMul: {
    llvm::Value *r = regs.load(start);
    for (uint32_t k = 1; k < length; ++k)
      r = regs.builder.CreateFMul(r, regs.load(start + k));
    return r;
  }
  case Kind::EPow:
    return intr(llvm::Intrinsic::pow, {regs.load(start), regs.load(start + 1)});
  case Kind::ELog: {
    llvm::Value *ln_a = intr(llvm::Intrinsic::log, {regs.load(start)});
    llvm::Value *ln_b = intr(llvm::Intrinsic::log, {regs.load(start + 1)});
    return regs.builder.CreateFDiv(ln_a, ln_b);
  }
  case Kind::ESin:
    return intr(llvm::Intrinsic::sin, {regs.load(start)});
  case Kind::ECos:
    return intr(llvm::Intrinsic::cos, {regs.load(start)});
  case Kind::ETan: {
    llvm::Value *s = intr(llvm::Intrinsic::sin, {regs.load(start)});
    llvm::Value *c = intr(llvm::Intrinsic::cos, {regs.load(start)});
    return regs.builder.CreateFDiv(s, c);
  }
  case Kind::ECot: {
    llvm::Value *s = intr(llvm::Intrinsic::sin, {regs.load(start)});
    llvm::Value *c = intr(llvm::Intrinsic::cos, {regs.load(start)});
    return regs.builder.CreateFDiv(c, s);
  }
  default:
    return nullptr;
  }
}

}
