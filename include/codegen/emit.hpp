#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "codegen/generate.hpp"
#include "codegen/policy.hpp"
#include "codegen/prologue.hpp"
#include "codegen/registers.hpp"
#include "core/arena.hpp"
#include "plan/plan.hpp"

namespace cascade::codegen {

struct EmittedModule {
  std::unique_ptr<llvm::LLVMContext> ctx;
  std::unique_ptr<llvm::Module> module;
  std::string kernel_name;
};

inline EmittedModule emit(const Arena &arena, const ShiftPlan &plan,
                          uint32_t root, uint8_t n_axes, unsigned width,
                          const std::string &name = "kernel") {
  auto ctx = std::make_unique<llvm::LLVMContext>();
  auto mod = std::make_unique<llvm::Module>(name, *ctx);

  llvm::Type *dbl = llvm::Type::getDoubleTy(*ctx);
  LanePolicy policy(dbl, width);

  llvm::FunctionType *sig = emit_signature(*ctx, n_axes);
  llvm::Function *fn = emit_entry_block(*mod, sig, name);
  llvm::IRBuilder<> builder(&fn->getEntryBlock());

  RegisterFile regs(builder, policy, plan.tape_size);
  regs.bind(fn, n_axes);
  regs.prologue();

  generate_nested(regs, *mod, policy, arena, plan, root, n_axes);
  builder.CreateRetVoid();

  return EmittedModule{std::move(ctx), std::move(mod), name};
}

}
