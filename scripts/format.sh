#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

format_args=(-i)
case "${1:-}" in
    --check) format_args=(--dry-run --Werror) ;;
    "") ;;
    *) echo "usage: $0 [--check]" >&2; exit 2 ;;
esac

files=()
while IFS= read -r -d '' file; do
    files+=("$file")
done < <(find include tests tools -type f \( -name '*.hpp' -o -name '*.cpp' -o -name '*.h' \) -print0)

if [ "${#files[@]}" -eq 0 ]; then
    echo "no source files found"
    exit 0
fi

nix develop . --command clang-format "${format_args[@]}" "${files[@]}"
