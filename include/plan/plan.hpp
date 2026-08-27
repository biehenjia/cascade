#pragma once

// The shift scheduler lives in cr-core -- shared, neutral ground between this
// repo and chains++. It is pure topology (no Scalar, no LeafT), so the same
// plan drives chains++'s interpreter and this repo's LLVM lowering. This is a
// thin re-export so codegen/ can keep spelling these names unqualified in
// namespace cascade, unchanged.
#include <cr/plan.hpp>

#include "core/arena.hpp"
#include "core/node.hpp"

namespace cascade {

using OpKind = cr::OpKind;
using Op = cr::Op;
using ResetProgram = cr::ResetProgram;
using ShiftPlan = cr::ShiftPlan;

inline constexpr uint32_t kNoOffset = cr::kNoOffset;

namespace plan_detail {
using cr::plan_detail::finalize_baseline;
}

using cr::build_shift_plan;

}
