#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "codegen/emit.hpp"
#include "codegen/jit.hpp"
#include "core/arena.hpp"
#include "plan/plan.hpp"

namespace cascade::codegen {

inline unsigned shrink_width(int64_t bound, unsigned max_width) {
  unsigned w = max_width;
  while (w > 1 && bound % int64_t(w) != 0)
    w /= 2;
  return w;
}

template <std::size_t N> struct Kernel {
  JitEngine engine;
  KernelFnPtr<N> fn;
  uint32_t tape_size;
  unsigned width;

  template <typename... Bounds>
  void operator()(double *result, double *tape, Bounds... bounds) const {
    static_assert(sizeof...(Bounds) == N,
                  "bounds count must match the kernel's axis count");
    [[maybe_unused]] const int64_t bs[N] = {static_cast<int64_t>(bounds)...};

    assert(bs[N - 1] % int64_t(width) == 0 &&
           "innermost bound must be a multiple of the kernel's vector width");
    fn(result, tape, static_cast<int64_t>(bounds)...);
  }
};

template <std::size_t N>
Kernel<N> compile(const Arena &arena, uint32_t root, unsigned width = 1,
                  const std::string &name = "kernel") {
  ShiftPlan plan = build_shift_plan(arena, root);
  EmittedModule em = emit(arena, plan, root, uint8_t(N), width, name);
  JitEngine engine(std::move(em));
  KernelFnPtr<N> fn = engine.get<N>(name);

  return Kernel<N>{std::move(engine), fn, plan.tape_size, width};
}

}
