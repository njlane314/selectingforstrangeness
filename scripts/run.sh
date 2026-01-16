#!/usr/bin/env bash
set -euo pipefail

mode="+"
if [[ "${1:-}" == "--interp"  ]]; then mode="";  shift; fi
if [[ "${1:-}" == "--rebuild" ]]; then mode="++"; shift; fi

macro_rel="${1:?Usage: $0 [--interp|--rebuild] macros/foo.C [args...]}"
shift || true

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$project_root/build"

macro_path="$macro_rel"
if [[ "$macro_path" != /* ]]; then
  macro_path="$project_root/$macro_path"
fi

if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
  cmake -S "$project_root" -B "$build_dir" -DCMAKE_BUILD_TYPE=RelWithDebInfo
fi
cmake --build "$build_dir" -j

export ROOT_INCLUDE_PATH="$project_root/include:${ROOT_INCLUDE_PATH:-}"
export LD_LIBRARY_PATH="$build_dir:${LD_LIBRARY_PATH:-}"
if [[ "$(uname -s)" == "Darwin" ]]; then
  export DYLD_LIBRARY_PATH="$build_dir:${DYLD_LIBRARY_PATH:-}"
fi

cpp_args=()
for a in "$@"; do
  if [[ "$a" =~ ^-?[0-9]+$ ]] \
  || [[ "$a" =~ ^-?[0-9]*\.[0-9]+([eE][-+]?[0-9]+)?$ ]] \
  || [[ "$a" =~ ^-?[0-9]+([eE][-+]?[0-9]+)?$ ]] \
  || [[ "$a" == "true" || "$a" == "false" || "$a" == "nullptr" ]]; then
    cpp_args+=("$a")
  else
    esc=${a//\\/\\\\}
    esc=${esc//\"/\\\"}
    cpp_args+=("\"$esc\"")
  fi
done

if (( ${#cpp_args[@]} )); then
  arglist="$(IFS=,; echo "${cpp_args[*]}")"
  root -l -b -q "${macro_path}${mode}(${arglist})"
else
  root -l -b -q "${macro_path}${mode}"
fi
