#pragma once

#include <cstddef>
#include <cstdint>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "codegen/access.hpp"
#include "codegen/math.hpp"
#include "codegen/policy.hpp"
#include "codegen/registers.hpp"
#include "codegen/shifting.hpp"
#include "core/arena.hpp"
#include "core/node.hpp"
#include "plan/plan.hpp"

namespace cascade::codegen {

inline void emit_fetch(RegisterFile &regs, const Op &op) {
  regs.store(op.start, access_ir(regs, op.src_kind, op.src, op.length));
}

inline void emit_reset(RegisterFile &regs, const ShiftPlan &plan,
                       uint8_t axis) {
  if (axis >= plan.reset_programs.size())
    return;
  const ResetProgram &rp = plan.reset_programs[axis];
  for (auto &[start, len] : rp.baseline_ranges) {
    for (uint32_t i = 0; i < len; ++i)
      regs.store(start + i, regs.constants[start + i]);
  }
  for (const Op &fetch : rp.fetches)
    emit_fetch(regs, fetch);
}

inline void emit_refresh(RegisterFile &regs, const ShiftPlan &plan,
                         uint8_t axis) {
  if (axis >= plan.refresh_programs.size())
    return;
  for (const Op &fetch : plan.refresh_programs[axis])
    emit_fetch(regs, fetch);
}

inline void emit_shift(RegisterFile &regs, const ShiftPlan &plan,
                       uint8_t axis) {
  if (axis >= plan.programs.size())
    return;
  for (const Op &op : plan.programs[axis]) {
    switch (op.op) {
    case OpKind::Sum:
      emit_sum_shift(regs, op.start, op.length);
      break;
    case OpKind::Prod:
      emit_prod_shift(regs, op.start, op.length);
      break;
    case OpKind::Trig:
      emit_trig_shift(regs, op.start, op.length);
      break;
    case OpKind::Fetch:
      break;
    }
  }
}

inline llvm::Value *emit_value(RegisterFile &regs, llvm::Module &mod,
                               const LanePolicy &policy, const Arena &arena,
                               const ShiftPlan &plan, uint32_t node_id) {
  const Node &n = arena.node(node_id);
  if (is_leaf_kind(n.kind))
    return splat(policy, arena.leaf(node_id));
  if (is_connector_kind(n.kind))
    return regs.load(plan.node_to_offset[n.slot_a] + n.slot_b);
  if (is_chain_kind(n.kind))
    return access_ir(regs, n.kind, plan.node_to_offset[node_id], n.slot_b);

  auto ops = arena.operands(node_id);
  auto val = [&](std::size_t k) {
    return emit_value(regs, mod, policy, arena, plan, ops[k]);
  };

  switch (n.kind) {
  case Kind::EAdd: {
    llvm::Value *r = val(0);
    for (std::size_t k = 1; k < ops.size(); ++k)
      r = regs.builder.CreateFAdd(r, val(k));
    return r;
  }
  case Kind::EMul: {
    llvm::Value *r = val(0);
    for (std::size_t k = 1; k < ops.size(); ++k)
      r = regs.builder.CreateFMul(r, val(k));
    return r;
  }
  case Kind::EPow:
    return call_intrinsic(regs.builder, mod, policy, llvm::Intrinsic::pow,
                          {val(0), val(1)});
  case Kind::ELog: {
    llvm::Value *ln_a = call_intrinsic(regs.builder, mod, policy,
                                       llvm::Intrinsic::log, {val(0)});
    llvm::Value *ln_b = call_intrinsic(regs.builder, mod, policy,
                                       llvm::Intrinsic::log, {val(1)});
    return regs.builder.CreateFDiv(ln_a, ln_b);
  }
  case Kind::ESin:
    return call_intrinsic(regs.builder, mod, policy, llvm::Intrinsic::sin,
                          {val(0)});
  case Kind::ECos:
    return call_intrinsic(regs.builder, mod, policy, llvm::Intrinsic::cos,
                          {val(0)});
  case Kind::ETan: {
    llvm::Value *s = call_intrinsic(regs.builder, mod, policy,
                                    llvm::Intrinsic::sin, {val(0)});
    llvm::Value *c = call_intrinsic(regs.builder, mod, policy,
                                    llvm::Intrinsic::cos, {val(0)});
    return regs.builder.CreateFDiv(s, c);
  }
  case Kind::ECot: {
    llvm::Value *s = call_intrinsic(regs.builder, mod, policy,
                                    llvm::Intrinsic::sin, {val(0)});
    llvm::Value *c = call_intrinsic(regs.builder, mod, policy,
                                    llvm::Intrinsic::cos, {val(0)});
    return regs.builder.CreateFDiv(c, s);
  }
  default:
    return nullptr;
  }
}

}
