#pragma once
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/Alignment.h>
#include <stdexcept>
#include <vector>

namespace cascade {

struct LanePolicy {
    llvm::Type* scalar_type;
    llvm::Type* slot_type;
    unsigned width;
    static LanePolicy scalar(llvm::Type* t) { return {t, t, 1}; }
    static LanePolicy vector(llvm::Type* t, unsigned w) {
        if (w == 0)
            throw std::invalid_argument("lane width must be positive");
        if (w == 1)
            return scalar(t);
        return {t, llvm::FixedVectorType::get(t, w), w};
    }
    void store_result(llvm::IRBuilder<>& b, llvm::Value* out, llvm::Value* idx, llvm::Value* val,
                      llvm::Value* remaining) const {
        llvm::Value* p = b.CreateGEP(scalar_type, out, idx, "result.addr");
        if (width == 1) {
            b.CreateStore(val, p);
            return;
        }
        std::vector<llvm::Constant*> offsets;
        offsets.reserve(width);
        for (unsigned lane = 0; lane < width; ++lane)
            offsets.push_back(b.getInt64(lane));
        auto* mask = b.CreateICmpULT(llvm::ConstantVector::get(offsets), b.CreateVectorSplat(width, remaining));
        b.CreateMaskedStore(val, p, llvm::Align(scalar_type->getScalarSizeInBits() / 8), mask);
    }
};
} // namespace cascade
