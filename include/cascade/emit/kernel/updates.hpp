#pragma once
#include <cascade/emit/access.hpp>
#include <llvm/IR/Intrinsics.h>

namespace cascade::detail {
template <class T>
void emit_shift(KernelContext<T>& ctx, const Shift& shift, llvm::Value* mask = nullptr) {
    llvm::IRBuilder<>& b = ctx.builder;
    codegen::Registers& regs = ctx.regs;
    auto write = [&](SlotId slot, llvm::Value* value) {
        if (mask)
            value = b.CreateSelect(mask, value, regs.read(slot));
        regs.write(slot, value);
    };
    switch (shift.comb) {
        case Comb::Sum:
            write(shift.dst, b.CreateFAdd(regs.read(shift.dst), regs.read(shift.rhs)));
            return;
        case Comb::Prod:
            write(shift.dst, b.CreateFMul(regs.read(shift.dst), regs.read(shift.rhs)));
            return;
        case Comb::Rot: {
            llvm::Type* ty = ctx.policy.slot_type;
            llvm::Value* sin = regs.read(shift.dst);
            llvm::Value* cos = regs.read(shift.dst + 1);
            llvm::Value* sin_rhs = regs.read(shift.rhs);
            llvm::Value* cos_rhs = regs.read(shift.rhs + 1);
            write(shift.dst, b.CreateIntrinsic(ty, llvm::Intrinsic::fma, {sin, cos_rhs, b.CreateFMul(cos, sin_rhs)}));
            write(shift.dst + 1, b.CreateIntrinsic(ty, llvm::Intrinsic::fma,
                                                   {cos, cos_rhs, b.CreateFNeg(b.CreateFMul(sin, sin_rhs))}));
            return;
        }
    }
}
template <class T>
void emit_resets(KernelContext<T>& ctx, const AxisPlan& plan) {
    for (const SlotId slot : plan.resets) {
        auto* value = ctx.initial_values[slot];
        if (ctx.policy.width > 1)
            value = ctx.builder.CreateVectorSplat(ctx.policy.width,
                                                  ctx.builder.CreateExtractElement(value, ctx.builder.getInt32(0)));
        ctx.regs.write(slot, value);
    }
}
template <class T>
void emit_fetches(KernelContext<T>& ctx, const AxisPlan& plan) {
    for (const Fetch& fetch : plan.fetches)
        ctx.regs.write(fetch.dst, dispatch_access(ctx, fetch.src));
}
template <class T>
void emit_shifts(KernelContext<T>& ctx, const AxisPlan& plan) {
    for (const Shift& shift : plan.shifts)
        emit_shift(ctx, shift);
}

// After resets/fetches, advance lane j by j steps from iteration zero.
template <class T>
void emit_lane_offsets(KernelContext<T>& ctx, const AxisPlan& plan) {
    for (unsigned step = 1; step < ctx.policy.width; ++step) {
        std::vector<llvm::Constant*> lanes;
        lanes.reserve(ctx.policy.width);
        for (unsigned lane = 0; lane < ctx.policy.width; ++lane)
            lanes.push_back(ctx.builder.getInt1(lane >= step));
        auto* mask = llvm::ConstantVector::get(lanes);
        for (const auto& shift : plan.shifts)
            emit_shift(ctx, shift, mask);
    }
}
} // namespace cascade::detail
