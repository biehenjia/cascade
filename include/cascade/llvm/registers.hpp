#pragma once
#include <cassert>
#include <llvm/IR/IRBuilder.h>
#include <vector>

namespace cascade::codegen {

// Reads are snapshots; write each slot on every incoming path before reading.
// Private entry-block allocas allow LLVM to promote state to SSA.
class Registers {
public:
    Registers(llvm::IRBuilder<>& builder, llvm::Type* type, unsigned count)
        : builder_(builder), type_(type), function_(builder.GetInsertBlock()->getParent()) {
        llvm::IRBuilder<> entry(&function_->getEntryBlock(), function_->getEntryBlock().getFirstInsertionPt());
        for (unsigned i = 0; i < count; ++i)
            slots_.push_back(entry.CreateAlloca(type, nullptr, "r" + llvm::Twine(i)));
    }
    Registers(const Registers&) = delete;
    Registers& operator=(const Registers&) = delete;
    llvm::Value* read(unsigned slot) {
        check(slot);
        return builder_.CreateLoad(type_, slots_[slot], "r" + llvm::Twine(slot) + ".value");
    }
    void write(unsigned slot, llvm::Value* value) {
        check(slot);
        assert(value->getType() == type_ && "register type mismatch");
        builder_.CreateStore(value, slots_[slot]);
    }

private:
    void check(unsigned slot) const {
        assert(slot < slots_.size() && "invalid register");
        assert(builder_.GetInsertBlock()->getParent() == function_);
    }
    llvm::IRBuilder<>& builder_;
    llvm::Type* type_;
    llvm::Function* function_;
    std::vector<llvm::AllocaInst*> slots_;
};
} // namespace cascade::codegen
