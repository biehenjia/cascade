#pragma once
#include <cascade/emit/kernel/nest.hpp>
#include <cascade/emit/policy.hpp>
#include <cascade/llvm/function.hpp>
#include <cascade/plan/tape.hpp>
#include <cassert>
#include <cr/ir/arena.hpp>
#include <cr/ir/node.hpp>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <stdexcept>
#include <vector>
namespace cascade {
template <class T>
struct KernelInput {
    const Tape<T>& tape;
    const cr::Arena<T>& arena;
    NodeId root;
    const LanePolicy& policy;
};

template <class T>
llvm::Function* generate_nested(llvm::Module& mod, const KernelInput<T>& input, llvm::StringRef name = "kernel") {
    llvm::LLVMContext& ctx = mod.getContext();
    llvm::Type* i64 = llvm::Type::getInt64Ty(ctx);
    llvm::Type* ptr = llvm::PointerType::getUnqual(ctx);
    const unsigned axes = input.arena.node(input.root).var;
    if (input.policy.width != input.tape.lanes)
        throw std::invalid_argument("kernel lane width must match the planned tape");
    assert(axes >= 1 && "kernel needs at least one axis");
    std::vector<llvm::Type*> params{ptr, ptr};
    params.insert(params.end(), axes, i64);
    auto* fty = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), params, false);
    auto* fn = llvm::Function::Create(fty, llvm::Function::ExternalLinkage, name, &mod);
    fn->getArg(0)->setName("result");
    fn->getArg(1)->setName("tape");
    for (unsigned axis = 0; axis < axes; ++axis)
        fn->getArg(2 + axis)->setName("bound." + llvm::Twine(axis));
    codegen::Function function(fn);
    auto& b = function.builder();
    auto regs = function.registers(input.policy.slot_type, input.tape.n_slots);
    std::vector<llvm::Value*> initial_values;
    initial_values.reserve(input.tape.n_slots);
    for (SlotId slot = 0; slot < input.tape.n_slots; ++slot) {
        auto* address =
            b.CreateGEP(input.policy.scalar_type, fn->getArg(1), b.getInt64(std::uint64_t(slot) * input.policy.width),
                        "tape.r" + llvm::Twine(slot) + ".addr");
        auto* value = b.CreateAlignedLoad(input.policy.slot_type, address,
                                          llvm::Align(input.policy.scalar_type->getScalarSizeInBits() / 8),
                                          "r" + llvm::Twine(slot) + ".initial");
        initial_values.push_back(value);
        regs.write(slot, value);
    }
    std::vector<llvm::Value*> bounds;
    bounds.reserve(axes);
    for (unsigned k = 0; k < axes; ++k)
        bounds.push_back(fn->getArg(2 + k));
    detail::KernelContext<T> lower{
        b, regs, input.tape, input.arena, input.root, input.policy, fn->getArg(0), bounds, initial_values};
    detail::emit_nested(lower);
    function.return_void();
    return fn;
}
} // namespace cascade
