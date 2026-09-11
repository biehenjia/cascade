#pragma once
#include <algorithm>
#include <cascade/plan/initial.hpp>
#include <cascade/plan/lanes.hpp>
#include <cascade/plan/tape.hpp>
#include <cascade/plan/types.hpp>
#include <cassert>
#include <cr/intern/intern.hpp>
#include <cr/ir/arena.hpp>
#include <cr/ir/node.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>
namespace cascade {
// Share terminal and suffix slots, recording each suffix's shift once.
template <class T>
Tape<T> plan_tape(const cr::Interner<T>& in, NodeId root, unsigned lanes = 1) {
    const cr::Arena<T>& arena = in.arena;
    Tape<T> t{0, std::vector<SlotId>(arena.size(), npos), std::vector<AxisPlan>(arena.node(root).var + 1)};
    detail::InitialValues<T> initial(arena);
    auto allocate = [&](NodeId source, Comb comb) {
        const SlotId slot = t.n_slots;
        if (comb == Comb::Rot) {
            const auto pair = arena.operands(source);
            assert(arena.node(source).kind == cr::Kind::Pair && pair.size() == 2);
            t.initial_values.push_back(initial(pair[0]));
            t.initial_values.push_back(initial(pair[1]));
        } else {
            t.initial_values.push_back(initial(source));
        }
        t.n_slots += width(comb);
        return slot;
    };
    std::vector<Axis> least(arena.size(), 0);
    for (const NodeId id : arena.postorder(root)) {
        Axis axis = arena.node(id).var;
        for (const NodeId op : arena.operands(id)) {
            const Axis child = least[op];
            if (child >= 1 && (axis == 0 || child < axis))
                axis = child;
        }
        least[id] = axis;
    }
    std::unordered_map<NodeId, SlotId> terminal_slot;
    std::unordered_map<std::uint64_t, SlotId> suffix_slot;
    auto terminal = [&](NodeId key, Comb comb) {
        auto [it, added] = terminal_slot.try_emplace(key, t.n_slots);
        if (added)
            allocate(key, comb);
        return it->second;
    };
    auto suffix = [&](std::uint64_t key, NodeId source, SlotId next, Axis axis, Comb comb) {
        auto [it, added] = suffix_slot.try_emplace(key, t.n_slots);
        if (added) {
            allocate(source, comb);
            t.axes[axis].shifts.push_back({it->second, next, comb});
        }
        return it->second;
    };
    for (const NodeId id : arena.postorder(root)) {
        const cr::Node& n = arena.node(id);
        Comb comb;
        switch (n.kind) {
            case cr::Kind::Sum:
                comb = Comb::Sum;
                break;
            case cr::Kind::Prod:
                comb = Comb::Prod;
                break;
            case cr::Kind::Rot:
                comb = Comb::Rot;
                break;
            default:
                continue;
        }
        const std::span<const std::uint64_t> sfx = in.suffixes(id);
        const std::span<const NodeId> ops = arena.operands(id);
        assert(sfx.size() == n.len && "chain not interned through intern_node");
        const Axis axis = arena.node(id).var;
        SlotId head = terminal(ops.back(), comb);
        for (std::size_t k = sfx.size() - 1; k-- > 0;)
            head = suffix(sfx[k], ops[k], head, axis, comb);
        t.head[id] = head;
        AxisPlan& plan = t.axes[axis];
        for (std::size_t k = 0; k < ops.size(); ++k) {
            const NodeId op = ops[k];
            const SlotId slot = k + 1 == ops.size() ? terminal_slot.at(op) : suffix_slot.at(sfx[k]);
            if (least[op] >= 1 && least[op] < axis) {
                if (comb == Comb::Rot) {
                    const std::span<const NodeId> pair = arena.operands(op);
                    plan.fetches.push_back({slot, pair[0]});
                    plan.fetches.push_back({slot + 1, pair[1]});
                } else {
                    plan.fetches.push_back({slot, op});
                }
            } else if (k + 1 < ops.size()) {
                plan.resets.push_back(slot);
                if (comb == Comb::Rot)
                    plan.resets.push_back(slot + 1);
            }
        }
    }
    for (AxisPlan& plan : t.axes)
        std::reverse(plan.shifts.begin(), plan.shifts.end());
    t.leaves.assign(arena.size(), npos);
    // Initialization-only coefficients already have tape entries.
    std::vector<bool> visited(arena.size(), false);
    auto inputs = [&](auto&& self, NodeId id) -> void {
        if (visited[id])
            return;
        visited[id] = true;
        const auto& node = arena.node(id);
        if (node.kind == cr::Kind::Leaf) {
            t.leaves[id] = terminal(id, Comb::Sum);
            return;
        }
        switch (node.kind) {
            case cr::Kind::Sum:
            case cr::Kind::Prod:
            case cr::Kind::Rot:
            case cr::Kind::Sin:
            case cr::Kind::Cos:
            case cr::Kind::Tan:
            case cr::Kind::Cot:
                return;
            case cr::Kind::Ref:
                self(self, node.ops);
                return;
            default:
                for (NodeId op : arena.operands(id))
                    self(self, op);
        }
    };
    inputs(inputs, root);
    for (const auto& axis : t.axes)
        for (const auto& fetch : axis.fetches)
            inputs(inputs, fetch.src);
    detail::pack_lanes(t, lanes);
    return t;
}
} // namespace cascade
