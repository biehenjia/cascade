#pragma once
#include <cascade/emit/policy.hpp>
#include <cascade/llvm/registers.hpp>
#include <cascade/plan/tape.hpp>
#include <cr/ir/arena.hpp>
#include <llvm/IR/IRBuilder.h>
#include <vector>

namespace cascade::detail {
template <class T>
struct KernelContext {
    llvm::IRBuilder<>& builder;
    codegen::Registers& regs;
    const Tape<T>& tape;
    const cr::Arena<T>& arena;
    NodeId root;
    const LanePolicy& policy;
    llvm::Value* out;
    const std::vector<llvm::Value*>& bounds;
    const std::vector<llvm::Value*>& initial_values;
    std::vector<llvm::Value*> indices;
};
} // namespace cascade::detail
