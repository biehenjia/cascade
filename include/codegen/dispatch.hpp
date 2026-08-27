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

inline void emit_fetch(RegisterFile &regs, llvm::Module &mod,
                       const LanePolicy &policy, const Op &op) {
  regs.store(op.start,
             access_ir(regs, mod, policy, op.src_kind, op.src, op.length));
}

inline void emit_reset(RegisterFile &regs, llvm::Module &mod,
                       const LanePolicy &policy, const ShiftPlan &plan,
                       uint8_t axis) {
  if (axis >= plan.reset_programs.size())
    return;
  const ResetProgram &rp = plan.reset_programs[axis];
  for (auto &[start, len] : rp.baseline_ranges) {
    for (uint32_t i = 0; i < len; ++i)
      regs.store(start + i, regs.constants[start + i]);
  }
  for (const Op &fetch : rp.fetches)
    emit_fetch(regs, mod, policy, fetch);
}

inline void emit_refresh(RegisterFile &regs, llvm::Module &mod,
                         const LanePolicy &policy, const ShiftPlan &plan,
                         uint8_t axis) {
  if (axis >= plan.refresh_programs.size())
    return;
  for (const Op &fetch : plan.refresh_programs[axis])
    emit_fetch(regs, mod, policy, fetch);
}

// Recompute algebraic nodes' operand slots from their children. Runs at the top
// of every iteration of `axis`, before the root is read and before any shift --
// so an algebraic node always reflects whatever its children hold now,
// including children on outer axes that advanced since last time.
inline void emit_prefetch(RegisterFile &regs, llvm::Module &mod,
                          const LanePolicy &policy, const ShiftPlan &plan,
                          uint8_t axis) {
  if (axis >= plan.prefetch_programs.size())
    return;
  for (const Op &fetch : plan.prefetch_programs[axis])
    emit_fetch(regs, mod, policy, fetch);
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

// Reading any node's current value: leaves splat, connectors alias, and
// everything else -- chain or algebraic -- is a combinator over its own tape
// slots. The old recursive E-kind switch is gone: algebraic nodes now own
// slots, so access_ir covers them (pycr's dispatch_access).
inline llvm::Value *emit_value(RegisterFile &regs, llvm::Module &mod,
                               const LanePolicy &policy, const Arena &arena,
                               const ShiftPlan &plan, uint32_t node_id) {
  const Node &n = arena.node(node_id);
  if (is_leaf_kind(n.kind))
    return splat(policy, arena.leaf(node_id));
  if (is_connector_kind(n.kind))
    return regs.load(plan.node_to_offset[n.slot_a] + n.slot_b);
  return access_ir(regs, mod, policy, n.kind, plan.node_to_offset[node_id],
                   n.slot_b);
}

}
