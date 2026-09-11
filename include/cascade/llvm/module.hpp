#pragma once
#include <cascade/llvm/function.hpp>
#include <cassert>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <memory>
#include <utility>

namespace cascade::codegen {

class Module {
public:
    explicit Module(llvm::StringRef name = "kernel")
        : context_(std::make_unique<llvm::LLVMContext>()), module_(std::make_unique<llvm::Module>(name, *context_)) {}
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;
    llvm::LLVMContext& context() { return *context_; }
    llvm::Module& ir() { return *module_; }
    Function function(llvm::StringRef name, llvm::FunctionType* signature) {
        assert(&signature->getContext() == context_.get());
        assert(!module_->getNamedValue(name) && "duplicate function name");
        return Function(llvm::Function::Create(signature, llvm::Function::ExternalLinkage, name, module_.get()));
    }
    // Consumes the module. No emission handles may be used after this handoff.
    llvm::orc::ThreadSafeModule take() && {
        return llvm::orc::ThreadSafeModule(std::move(module_), std::move(context_));
    }

private:
    // Destruction order matters: module before context.
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;
};
} // namespace cascade::codegen
