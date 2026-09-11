#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

build_dir="${CASCADE_BUILD_DIR:-build}"
tidy_args=(-p "$build_dir")
case "${1:-}" in
    --fix) tidy_args+=(--fix) ;;
    "") ;;
    *) echo "usage: $0 [--fix]" >&2; exit 2 ;;
esac

if [ ! -f "$build_dir/compile_commands.json" ]; then
    echo "configure CMake in $build_dir before running clang-tidy" >&2
    exit 1
fi

files=()
while IFS= read -r -d '' file; do
    files+=("$file")
done < <(find tests tools -type f -name '*.cpp' -print0)

if [ "${#files[@]}" -eq 0 ]; then
    echo "no source files found"
    exit 0
fi

nix develop . --command clang-tidy "${tidy_args[@]}" "${files[@]}"
