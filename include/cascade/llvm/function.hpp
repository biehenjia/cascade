#pragma once
#include <cascade/llvm/looping.hpp>
#include <cascade/llvm/registers.hpp>
#include <span>
#include <utility>

namespace cascade::codegen {

class Function {
public:
    explicit Function(llvm::Function* fn)
        : function_(fn), builder_(llvm::BasicBlock::Create(fn->getContext(), "entry", fn)) {}
    llvm::IRBuilder<>& builder() { return builder_; }
    llvm::Value* argument(unsigned index) { return function_->getArg(index); }
    Registers registers(llvm::Type* type, unsigned count) { return Registers(builder_, type, count); }
    template <class Body>
    void loop(llvm::Value* bound, Body&& body, unsigned step = 1) {
        codegen::loop(builder_, bound, std::forward<Body>(body), step);
    }
    template <class Body>
    void nest(std::span<llvm::Value* const> bounds, Body&& body, unsigned inner_step = 1) {
        codegen::nest(builder_, bounds, std::forward<Body>(body), inner_step);
    }
    void return_value(llvm::Value* value) { builder_.CreateRet(value); }
    void return_void() { builder_.CreateRetVoid(); }

private:
    llvm::Function* function_;
    llvm::IRBuilder<> builder_;
};
} // namespace cascade::codegen
