#!/usr/bin/env bash
# Loads every catalog entry in headless Chrome, straight from file://, and
# checks the page actually rendered: a step count, a view, and no uncaught
# console error.
#
# Deliberately serial: one Chrome per algorithm, one at a time. Running several
# at once needs --user-data-dir to keep the profiles apart, and that flag makes
# headless Chrome hang here. Takes a couple of minutes; run it in the background.
set -uo pipefail
cd "$(dirname "$0")/.."

CHROME=${CHROME:-"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"}
if [ ! -x "$CHROME" ]; then
  echo "chrome non trovato ($CHROME): salto lo smoke test"; exit 0
fi

PAGE="file://$(pwd)/web/index.html"
LOG=$(mktemp)
trap 'rm -f "$LOG"' EXIT

ids=$(node -e '
  const vm=require("vm"),fs=require("fs");
  const s={window:{}}; vm.createContext(s);
  vm.runInContext(fs.readFileSync("web/catalog.js","utf8"),s);
  console.log(s.window.CATALOG.flatMap(g=>g.items.map(i=>i.id)).join(" "));
')

fail=0
for id in $ids; do
  dom=$("$CHROME" --headless --disable-gpu --no-sandbox --no-first-run \
        --no-default-browser-check --virtual-time-budget=3500 \
        --enable-logging=stderr --log-level=0 --dump-dom "$PAGE#$id" 2>"$LOG")
  errs=$(grep -E "Uncaught|SEVERE" "$LOG" | grep -v "GPU\|gpu\|Vulkan\|dbus\|Fontconfig" || true)
  problem=""
  echo "$dom" | grep -qE '<svg|class="(matrixbox|tablebox)' || problem="$problem no-view"
  echo "$dom" | grep -qE 'id="step-count">[1-9]'            || problem="$problem no-steps"
  echo "$dom" | grep -q 'Errore:'                           && problem="$problem load-error"
  [ -n "$errs" ]                                            && problem="$problem console-error"
  if [ -n "$problem" ]; then
    echo "  FAIL $id:$problem"
    [ -n "$errs" ] && echo "$errs" | head -3
    fail=$((fail + 1))
  else
    echo "  ok   $id"
  fi
done

echo "smoke: $fail fallimenti"
[ "$fail" -eq 0 ]
