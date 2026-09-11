#pragma once
#include <cascade/plan/types.hpp>
#include <cr/domain/field.hpp>
#include <cr/ir/arena.hpp>
#include <optional>
#include <stdexcept>
#include <vector>

namespace cascade::detail {
// Evaluate axis origins in the value domain, leaving symbols unbound.
template <class T>
class InitialValues {
public:
    explicit InitialValues(const cr::Arena<T>& arena) : arena_(arena), cache_(arena.size()) {}
    T operator()(NodeId id) {
        auto& cached = cache_.at(id);
        if (!cached)
            cached = evaluate(id);
        return *cached;
    }

private:
    T evaluate(NodeId id) {
        using F = cr::Field<T>;
        const auto& n = arena_.node(id);
        if (n.kind == cr::Kind::Leaf)
            return arena_.value(n.ops);
        if (n.kind == cr::Kind::Ref)
            return (*this)(n.ops);
        auto ops = arena_.operands(id);
        auto arg = [&](unsigned k) { return (*this)(ops[k]); };
        auto projection = [&](unsigned k) {
            const auto rot = arena_.operands(ops[0]);
            const auto pair = arena_.operands(rot[0]);
            return (*this)(pair[k]);
        };
        switch (n.kind) {
            case cr::Kind::Sum:
            case cr::Kind::Prod:
                return arg(0);
            case cr::Kind::Sin:
                return projection(0);
            case cr::Kind::Cos:
                return projection(1);
            case cr::Kind::Tan:
                return F::div(projection(0), projection(1));
            case cr::Kind::Cot:
                return F::div(projection(1), projection(0));
            case cr::Kind::EAdd:
                return F::add(arg(0), arg(1));
            case cr::Kind::ESub:
                return F::sub(arg(0), arg(1));
            case cr::Kind::EMul:
                return F::mul(arg(0), arg(1));
            case cr::Kind::EDiv:
                return F::div(arg(0), arg(1));
            case cr::Kind::EPow:
                return F::pow(arg(0), arg(1));
            case cr::Kind::ELog:
                return F::log(arg(0));
            case cr::Kind::ESin:
                return F::sin(arg(0));
            case cr::Kind::ECos:
                return F::cos(arg(0));
            case cr::Kind::ETan:
                return F::tan(arg(0));
            case cr::Kind::ECot:
                return F::cot(arg(0));
            default:
                throw std::invalid_argument("initial value requires a scalar CR expression");
        }
    }
    const cr::Arena<T>& arena_;
    std::vector<std::optional<T>> cache_;
};
} // namespace cascade::detail
