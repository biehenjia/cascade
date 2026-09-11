#pragma once
#include <cascade/emit/kernel/context.hpp>
#include <cassert>
#include <cr/ir/arena.hpp>
#include <cr/ir/node.hpp>
#include <cstdint>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
namespace cascade::detail {
template <class T>
llvm::Value* dispatch_access(KernelContext<T>& ctx, NodeId id) {
    llvm::IRBuilder<>& b = ctx.builder;
    codegen::Registers& regs = ctx.regs;
    const cr::Arena<T>& arena = ctx.arena;
    const cr::Node& n = arena.node(id);
    llvm::Type* ty = ctx.policy.slot_type;
    const std::span<const NodeId> ops = arena.operands(id);
    auto rec = [&](NodeId x) { return dispatch_access(ctx, x); };
    auto head = [&](NodeId chain) { return ctx.tape.head[chain]; };
    auto un = [&](llvm::Intrinsic::ID iid, NodeId x) { return b.CreateIntrinsic(ty, iid, {rec(x)}); };
    switch (n.kind) {
        case cr::Kind::Leaf:
            return ctx.initial_values.at(ctx.tape.leaves.at(id));
        case cr::Kind::Sum:
        case cr::Kind::Prod:
            return regs.read(head(id));
        case cr::Kind::Sin:
            return regs.read(head(ops[0]));
        case cr::Kind::Cos:
            return regs.read(head(ops[0]) + 1);
        case cr::Kind::Tan:
            return b.CreateFDiv(regs.read(head(ops[0])), regs.read(head(ops[0]) + 1));
        case cr::Kind::Cot:
            return b.CreateFDiv(regs.read(head(ops[0]) + 1), regs.read(head(ops[0])));
        case cr::Kind::EAdd:
            return b.CreateFAdd(rec(ops[0]), rec(ops[1]));
        case cr::Kind::ESub:
            return b.CreateFSub(rec(ops[0]), rec(ops[1]));
        case cr::Kind::EMul:
            return b.CreateFMul(rec(ops[0]), rec(ops[1]));
        case cr::Kind::EDiv:
            return b.CreateFDiv(rec(ops[0]), rec(ops[1]));
        case cr::Kind::EPow:
            return b.CreateIntrinsic(ty, llvm::Intrinsic::pow, {rec(ops[0]), rec(ops[1])});
        case cr::Kind::ELog:
            return un(llvm::Intrinsic::log, ops[0]);
        case cr::Kind::ESin:
            return un(llvm::Intrinsic::sin, ops[0]);
        case cr::Kind::ECos:
            return un(llvm::Intrinsic::cos, ops[0]);
        case cr::Kind::ETan:
            return un(llvm::Intrinsic::tan, ops[0]);
        case cr::Kind::ECot:
            return b.CreateFDiv(llvm::ConstantFP::get(ty, 1.0), un(llvm::Intrinsic::tan, ops[0]));
        case cr::Kind::Ref:
            return rec(n.ops);
        default:
            assert(false && "dispatch_access: unexpected kind");
            return llvm::ConstantFP::get(ty, 0.0);
    }
}
} // namespace cascade::detail
