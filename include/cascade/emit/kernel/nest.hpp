#pragma once
#include <cascade/emit/kernel/context.hpp>
#include <cascade/emit/kernel/result.hpp>
#include <cascade/emit/kernel/updates.hpp>
#include <cascade/llvm/looping.hpp>
#include <cascade/plan/tape.hpp>

namespace cascade::detail {
template <class T>
void emit_nested(KernelContext<T>& ctx) {
    auto emit_axis = [&](auto&& self, unsigned level) -> void {
        const bool innermost = level + 1 == ctx.bounds.size();
        codegen::loop(
            ctx.builder, ctx.bounds[level],
            [&](llvm::Value* idx) {
                ctx.indices.push_back(idx);
                if (innermost) {
                    emit_result(ctx);
                } else {
                    const AxisPlan& plan = ctx.tape.axes[level + 2];
                    emit_resets(ctx, plan);
                    emit_fetches(ctx, plan);
                    if (level + 2 == ctx.bounds.size())
                        emit_lane_offsets(ctx, plan);
                    self(self, level + 1);
                }
                for (unsigned step = 0; step < (innermost ? ctx.policy.width : 1); ++step)
                    emit_shifts(ctx, ctx.tape.axes[level + 1]);
                ctx.indices.pop_back();
            },
            innermost ? ctx.policy.width : 1);
    };
    emit_axis(emit_axis, 0);
}
} // namespace cascade::detail
