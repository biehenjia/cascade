#pragma once
#include <cascade/plan/types.hpp>
#include <cstdint>
#include <vector>
namespace cascade {
inline constexpr SlotId npos = ~SlotId{0};
// A shift combine; Rot uses adjacent sin/cos slots.
enum class Comb : std::uint8_t { Sum, Prod, Rot };
inline constexpr SlotId width(Comb c) {
    return c == Comb::Rot ? 2u : 1u;
}
struct Fetch {
    SlotId dst;
    NodeId src;
};
struct Shift {
    SlotId dst;
    SlotId rhs;
    Comb comb;
};
// Everything an axis needs at one loop boundary. Resets precede fetches;
// shifts run after evaluation. Actions execute in vector order.
struct AxisPlan {
    std::vector<SlotId> resets;
    std::vector<Fetch> fetches;
    std::vector<Shift> shifts;
};
// A fully planned lowering program. head[id] is the first slot of a chain,
// npos for every other node; axes are indexed by CR variable.
template <class T>
struct Tape {
    SlotId n_slots = 0;
    std::vector<SlotId> head;
    std::vector<AxisPlan> axes;
    // Initial register values in the arena's domain (e.g. symbolic expressions).
    // Slot-major entries: slot * lanes + lane. Simplification preserves order.
    std::vector<T> initial_values;
    // Immutable expression leaves also enter through the runtime tape.
    std::vector<SlotId> leaves;
    unsigned lanes = 1;
};
} // namespace cascade
