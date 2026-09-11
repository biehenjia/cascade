#pragma once
#include <cascade/emit/access.hpp>

namespace cascade::detail {
template <class T>
void emit_result(KernelContext<T>& ctx) {
    llvm::Value* flat = ctx.indices[0];
    for (unsigned k = 1; k < ctx.bounds.size(); ++k)
        flat = ctx.builder.CreateAdd(ctx.builder.CreateMul(flat, ctx.bounds[k]), ctx.indices[k]);
    auto* remaining = ctx.builder.CreateSub(ctx.bounds.back(), ctx.indices.back());
    ctx.policy.store_result(ctx.builder, ctx.out, flat, dispatch_access(ctx, ctx.root), remaining);
}
} // namespace cascade::detail
