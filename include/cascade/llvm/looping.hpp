#pragma once
#include <cassert>
#include <cstddef>
#include <llvm/IR/IRBuilder.h>
#include <span>
#include <vector>

namespace cascade::codegen {


template <class Body>
void loop(llvm::IRBuilder<>& b, llvm::Value* bound, Body&& body, unsigned step = 1) {
    assert(step > 0 && bound->getType() == b.getInt64Ty());
    auto* pre = b.GetInsertBlock();
    assert(pre && !pre->getTerminator());
    auto* fn = pre->getParent();
    auto* body_block = llvm::BasicBlock::Create(b.getContext(), "loop.body", fn);
    auto* exit = llvm::BasicBlock::Create(b.getContext(), "loop.exit", fn);
    b.CreateCondBr(b.CreateICmpSGT(bound, b.getInt64(0)), body_block, exit);
    b.SetInsertPoint(body_block);
    auto* index = b.CreatePHI(b.getInt64Ty(), 2, "index");
    index->addIncoming(b.getInt64(0), pre);
    body(index);
    auto* latch = b.GetInsertBlock();
    assert(latch->getParent() == fn && !latch->getTerminator());
    auto* next = b.CreateAdd(index, b.getInt64(step));
    index->addIncoming(next, latch);
    b.CreateCondBr(b.CreateICmpULT(next, bound), body_block, exit);
    b.SetInsertPoint(exit);
}


template <class Body>
void nest(llvm::IRBuilder<>& b, std::span<llvm::Value* const> bounds, Body&& body, unsigned inner_step = 1) {
    assert(inner_step > 0);
    std::vector<llvm::Value*> indices;
    auto axis = [&](auto&& self, std::size_t level) -> void {
        if (level == bounds.size()) {
            body(std::span<llvm::Value* const>(indices));
            return;
        }
        loop(
            b, bounds[level],
            [&](llvm::Value* index) {
                indices.push_back(index);
                self(self, level + 1);
                indices.pop_back();
            },
            level + 1 == bounds.size() ? inner_step : 1);
    };
    axis(axis, 0);
}
} // namespace cascade::codegen
