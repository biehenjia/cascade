#pragma once

#include <cstdint>
#include <vector>

#include <llvm/IR/Module.h>

#include "codegen/dispatch.hpp"
#include "codegen/looping.hpp"
#include "codegen/policy.hpp"
#include "codegen/registers.hpp"
#include "core/arena.hpp"
#include "plan/plan.hpp"

namespace cascade::codegen {

inline void generate_nested(RegisterFile &regs, llvm::Module &mod,
                            const LanePolicy &policy, const Arena &arena,
                            const ShiftPlan &plan, uint32_t root,
                            uint8_t n_axes) {
  emit_reset(regs, mod, policy, plan, 0);

  std::vector<LoopHandle> latches;
  for (uint8_t i = 0; i + 1 < n_axes; ++i) {
    latches.push_back(begin_loop(regs, regs.bounds[i]));
    emit_prefetch(regs, mod, policy, plan, i);
    emit_reset(regs, mod, policy, plan, i + 1);
  }

  latches.push_back(
      begin_loop(regs, regs.bounds[n_axes - 1], int64_t(policy.width)));
  emit_prefetch(regs, mod, policy, plan, uint8_t(n_axes - 1));
  llvm::Value *root_val = emit_value(regs, mod, policy, arena, plan, root);
  regs.store_result(root_val);

  for (unsigned step = 0; step < policy.width; ++step) {
    emit_refresh(regs, mod, policy, plan, uint8_t(n_axes - 1));
    emit_shift(regs, plan, uint8_t(n_axes - 1));
  }
  end_loop(regs, latches.back());
  latches.pop_back();

  for (int i = int(n_axes) - 2; i >= 0; --i) {
    emit_refresh(regs, mod, policy, plan, uint8_t(i));
    emit_shift(regs, plan, uint8_t(i));
    end_loop(regs, latches.back());
    latches.pop_back();
  }
}

}
