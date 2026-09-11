#pragma once
#include <cascade/llvm/module.hpp>
#include <cascade/llvm/pipeline.hpp>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>

namespace cascade::codegen {
namespace detail {
inline void init_native() {
    static const bool initialized = [] {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        return true;
    }();
    (void)initialized;
}
} // namespace detail

class Executable {
public:
    explicit Executable(std::unique_ptr<llvm::orc::LLJIT> jit) : jit_(std::move(jit)) {}
    llvm::Expected<llvm::orc::ExecutorAddr> lookup(llvm::StringRef name) { return jit_->lookup(name); }
    template <class Fn>
    llvm::Expected<Fn*> get(llvm::StringRef name) {
        auto address = lookup(name);
        if (!address)
            return address.takeError();
        return address->template toPtr<Fn*>();
    }

private:
    std::unique_ptr<llvm::orc::LLJIT> jit_;
};

inline llvm::Expected<Executable> compile(llvm::orc::ThreadSafeModule owned,
                                          llvm::OptimizationLevel level = llvm::OptimizationLevel::O3,
                                          bool vectorize = true) {
    detail::init_native();
    auto target = llvm::orc::JITTargetMachineBuilder::detectHost();
    if (!target)
        return target.takeError();
    auto machine = target->createTargetMachine();
    if (!machine)
        return machine.takeError();
    auto error = owned.withModuleDo([&](llvm::Module& ir) -> llvm::Error {
        std::string diagnostics;
        llvm::raw_string_ostream stream(diagnostics);
        if (llvm::verifyModule(ir, &stream))
            return llvm::createStringError(llvm::inconvertibleErrorCode(), diagnostics);
        optimize(ir, **machine, level, vectorize);
        if (llvm::verifyModule(ir, &stream))
            return llvm::createStringError(llvm::inconvertibleErrorCode(), diagnostics);
        return llvm::Error::success();
    });
    if (error)
        return std::move(error);
    auto jit = llvm::orc::LLJITBuilder().setJITTargetMachineBuilder(std::move(*target)).create();
    if (!jit)
        return jit.takeError();
    if (auto error = (*jit)->addIRModule(std::move(owned)))
        return std::move(error);
    return Executable(std::move(*jit));
}

inline llvm::Expected<Executable> compile(Module&& module, llvm::OptimizationLevel level = llvm::OptimizationLevel::O3,
                                          bool vectorize = true) {
    return compile(std::move(module).take(), level, vectorize);
}
} // namespace cascade::codegen
