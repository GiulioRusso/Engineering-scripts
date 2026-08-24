#!/usr/bin/env bash
# Compiles every instrumented circuit and regenerates every trace.
#
# Only needed if you change the C++. The traces are committed, so cloning the
# repo and opening web/index.html is enough to see everything.
set -euo pipefail
cd "$(dirname "$0")"

CXX=${CXX:-g++}
CXXFLAGS="-Wall -Wextra -std=c++17 -O2"

mkdir -p .bin traces

only="${1:-}"
count=0
for src in cpp/*/*.cpp; do
  case "$src" in cpp/tracer/*|cpp/sim/*) continue;; esac
  name="$(basename "$src" .cpp)"
  if [ -n "$only" ] && [ "$name" != "$only" ]; then continue; fi
  printf '%-22s' "$name"
  # __FILE__ must stay relative to this directory: the tracer reads the source
  # back to build displaySource, and the binary runs from here too.
  "$CXX" $CXXFLAGS -o ".bin/$name" "$src"
  "./.bin/$name"
  count=$((count + 1))
done

echo "built $count circuit(s)"

if command -v node >/dev/null 2>&1; then
  echo "--- syntax check"
  for f in traces/*.js web/*.js web/renderers/*.js; do node --check "$f"; done
  echo "--- consistency check"
  node tools/check_traces.js
else
  echo "node not found: skipping trace checks"
fi
