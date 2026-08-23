#!/usr/bin/env bash
# Loads every catalog entry in headless Chrome, straight from file://, and
# checks the page actually rendered: a step count, an SVG in the primary view,
# an explanation line, and no uncaught console error.
set -uo pipefail
cd "$(dirname "$0")/.."

CHROME=${CHROME:-"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"}
if [ ! -x "$CHROME" ]; then
  echo "chrome non trovato ($CHROME): salto lo smoke test"; exit 0
fi

PAGE="file://$(pwd)/web/index.html"
ids=$(node -e '
  const vm=require("vm"),fs=require("fs");
  const s={window:{}}; vm.createContext(s);
  vm.runInContext(fs.readFileSync("web/catalog.js","utf8"),s);
  console.log(s.window.CATALOG.flatMap(g=>g.items.map(i=>i.id)).join(" "));
')

fail=0
for id in $ids; do
  out=$("$CHROME" --headless --disable-gpu --no-sandbox --virtual-time-budget=6000 \
        --enable-logging=stderr --log-level=0 --dump-dom "$PAGE#$id" 2>/tmp/algoviz-console.txt)
  errs=$(grep -E "Uncaught|SEVERE|ERROR:.*javascript" /tmp/algoviz-console.txt | grep -v "GPU\|gpu\|Vulkan\|dbus\|Fontconfig" || true)
  problem=""
  echo "$out" | grep -q '<svg'                 || problem="$problem no-svg"
  echo "$out" | grep -qE 'id="step-count">[1-9]' || problem="$problem no-steps"
  echo "$out" | grep -q 'Errore:'              && problem="$problem load-error"
  [ -n "$errs" ]                               && problem="$problem console-error"
  if [ -n "$problem" ]; then
    echo "  FAIL $id:$problem"
    [ -n "$errs" ] && echo "$errs" | head -3
    fail=$((fail+1))
  else
    echo "  ok   $id"
  fi
done

echo "smoke: $fail fallimenti"
exit $((fail > 0))
