#pragma once
#include <cascade/plan/tape.hpp>
#include <cr/domain/field.hpp>
#include <stdexcept>

namespace cascade::detail {
// Seed slot-major lanes by advancing the ordered scalar schedule.
template <class T>
void pack_lanes(Tape<T>& tape, unsigned lanes) {
    if (lanes == 0)
        throw std::invalid_argument("lane width must be positive");
    tape.lanes = lanes;
    if (lanes == 1)
        return;
    using F = cr::Field<T>;
    auto state = tape.initial_values;
    tape.initial_values.clear();
    for (const auto& value : state)
        for (unsigned lane = 0; lane < lanes; ++lane)
            tape.initial_values.push_back(value);
    for (unsigned lane = 1; lane < lanes; ++lane) {
        for (const auto& shift : tape.axes.back().shifts) {
            auto& dst = state[shift.dst];
            const auto rhs = state[shift.rhs];
            switch (shift.comb) {
                case Comb::Sum:
                    dst = F::add(dst, rhs);
                    break;
                case Comb::Prod:
                    dst = F::mul(dst, rhs);
                    break;
                case Comb::Rot: {
                    const auto sin = dst;
                    const auto cos = state[shift.dst + 1];
                    const auto cos_rhs = state[shift.rhs + 1];
                    dst = F::add(F::mul(sin, cos_rhs), F::mul(cos, rhs));
                    state[shift.dst + 1] = F::sub(F::mul(cos, cos_rhs), F::mul(sin, rhs));
                    break;
                }
            }
        }
        for (SlotId slot = 0; slot < tape.n_slots; ++slot)
            tape.initial_values[std::size_t(slot) * lanes + lane] = state[slot];
    }
}
} // namespace cascade::detail
