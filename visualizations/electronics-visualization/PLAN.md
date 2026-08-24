# Electronics Visualizer — implementation plan

Build `visualizations/electronics-visualization/` as a sibling of
`visualizations/algorithm-visualization/`, reusing its architecture wholesale.
Content comes from `scripts/electronics/*.ipynb`, **excluding** `alu.ipynb`,
`memory.ipynb`, `msi_combinational.ipynb` and `registers.ipynb`.

Source notebooks in scope (6): `logic_gates`, `combinational_adders`, `latches`,
`flipflops`, `counters`, `fsm`.

Reference implementation to copy from and to read before starting:
`visualizations/algorithm-visualization/GUIDE.md` — the whole design rationale
applies here unchanged.

---

## 1. The one design decision

```
circuito.cpp  →  compiled and executed at build time  →  trace  →  player in the browser
```

The circuits stay **real C++ circuits** and are the single source of truth. The
browser player executes nothing: it replays a list of states a real run already
produced. That is what makes stepping **backwards** free (an index decrement),
which is the single most important property of a teaching tool.

Traces are committed. A fresh clone opens `web/index.html` by double-click, with
no compiler, no server, no `pip install`, no network.

**No generic netlist simulator.** Each `.cpp` hand-writes its circuit
evaluation, the way the notebooks do. The instrumented source is itself the
teaching material shown in the code panel; a netlist interpreter on screen would
teach the interpreter instead of the circuit.

**Coordinates are hand-placed in the C++**, lifted from the notebooks' existing
matplotlib coordinates. Never an auto-layout at runtime — a picture that
rearranges itself between steps is unfollowable.

---

## 2. Layout

```
visualizations/electronics-visualization/
├── PLAN.md                      # this file, delete when done
├── GUIDE.md                     # written in Phase 5
├── build.sh                     # copied verbatim
├── cpp/
│   ├── sim/
│   │   ├── logic.hpp            # the 7 gates + tri-state
│   │   ├── circuit.hpp          # schematic snapshot builder
│   │   └── waves.hpp            # waveform accumulator
│   ├── tracer/
│   │   └── tracer.hpp           # copied, +2 methods
│   ├── 01_gates/  02_adders/  03_latches/
│   ├── 04_flipflops/  05_counters/  06_fsm/
├── tools/
│   ├── check_traces.js          # rewritten checks, same harness
│   └── smoke.sh                 # copied verbatim
├── traces/                      # generated, COMMITTED
└── web/
    ├── index.html
    ├── shell.js                 # copied, 2 strings changed
    ├── catalog.js               # new content
    ├── style.css                # copied + ~30 lines
    └── renderers/               # 3 copied, 2 patched, 2 new
```

---

## 3. What is reused, patched, and new

### Copied verbatim
| From algorithm-visualization | Notes |
|---|---|
| `build.sh` | globs `cpp/*/*.cpp`; make it skip `cpp/sim/` as well as `cpp/tracer/` |
| `tools/smoke.sh` | headless Chrome load of every catalogue entry |
| `web/renderers/array.js` | counter bit vectors, shift registers |
| `web/renderers/table.js` | transition tables, delay comparisons, bit-flip counts |
| `web/renderers/callstack.js` | keep for free; may go unused |

### Copied with a small edit
| File | Edit |
|---|---|
| `cpp/tracer/tracer.hpp` | add `circuitState(raw)` and `waveState(raw)` to `StepBuilder`, mirroring the existing `graphState` / `matrixState` pair (single + named-panel overloads). ~10 lines. Emit them as `circuit` / `circuits` and `wave` / `waves` fields. |
| `web/shell.js` | two strings only: badge labels `'tempo '` → `'ritardo '` and `'spazio '` → `'gate '` (lines ~97–98); `localStorage` key `algoviz-theme` → `elecviz-theme`. Nothing else. |
| `web/style.css` | append ~30 lines: logic-level palette `--c-hi` (driven high), `--c-lo` (driven low), `--c-z` (high impedance), plus `.gatebox`, `.wire`, `.pin`, `.wavesig`, `.wavecursor`, `.wave-window`. Keep every existing semantic token — `--c-selected` still means "current", so intuition carries across the two visualizers. |

### Patched, so no new renderer is needed
| File | Patch | Buys |
|---|---|---|
| `web/renderers/matrix.js` | accept optional `M.rowLabels`, falling back to `M.labels[r]` (it currently assumes a square matrix). ~3 lines. | **truth tables** render as a matrix, with `hlRow` marking the active input combination. No `truthtable` renderer. |
| `web/renderers/graph.js` | when `e.from === e.to`, draw a self-loop arc above the node instead of a degenerate zero-length line. ~12 lines. | **FSM state diagrams** render as a graph. No `statediagram` renderer. |

### New — exactly two
**`web/renderers/schematic.js`** (~200 lines). Gate boxes, elbow wires, pins.
Wire colour and width follow the logic level, the same convention the notebooks
already use (`wcol` / `wlw`). The gate currently being evaluated gets
`--c-selected`.

```js
{
  gates: [{ id: "g1", type: "XOR", x: 3, y: 2, out: 1, state: "active" }],
  wires: [{ from: "A", to: "g1.a", bit: 1, state: "" }],
  pins:  [{ name: "A", x: 0, y: 2.4, bit: 1, side: "left" }]
}
```
`from`/`to` are either a pin name or `"<gateId>.<port>"`. Ports: `a`, `b`, `out`.
Implements `mount` / `render` / `legend`; honours `opts.compact` and
`opts.source` like every other renderer.

**`web/renderers/waveform.js`** (~150 lines). N stepped signals over time, a
current-time cursor, rising-edge tick marks, and an optional shaded window for
setup/hold.

```js
{
  signals: [{ name: "CLK", values: [0,1,0,1], kind: "clk" }],
  t: 7,
  markers: [{ t: 4, label: "edge" }],
  window: { from: 6, to: 8, label: "t_su" }
}
```
`kind`: `clk` | `data` | `out` — only affects styling. Same interface contract.

Register both in `window.RENDERERS` and add the `<script>` tags to
`web/index.html`.

---

## 4. C++ simulation layer — `cpp/sim/`

Three small headers, the analogue of `cpp/tracer/views.hpp`. They only turn the
current state into the JSON the matching renderer expects; they never change how
a circuit is written.

- **`logic.hpp`** — `NOT AND OR NAND NOR XOR XNOR` plus tri-state, as free
  functions on `int`. Same definitions as `logic_gates.ipynb`. ~20 lines.
- **`circuit.hpp`** — a `Schematic` that a `.cpp` declares once
  (`S.gate("g1","XOR",3,2); S.wire("A","g1.a"); S.pin("A",0,2.4,"left");`) and
  then re-serialises per step with the current bit values (`S.json()`).
- **`waves.hpp`** — a `Waves` accumulator: `W.add("CLK","clk")`, `W.push("CLK",1)`,
  `W.json(t)`.

Feedback: cross-coupled circuits (the SR latch) settle iteratively. Emit **one
step per settle iteration**. That the latch settles at all is the lesson, not an
implementation detail to hide.

Everything compiles with `-Wall -Wextra -std=c++17` and no warnings.

---

## 5. Catalogue — 31 entries, 6 chapters

Badges are repurposed, since time/space complexity is meaningless here:
`complexity.time` carries the propagation delay (`"3 t_p"`), `complexity.space`
the gate count (`"5 gate"`). Sequential entries use `"level-sensitive"` /
`"edge-triggered"` in the delay slot where that is the real answer.

| Chapter | id | Name | View | Panels |
|---|---|---|---|---|
| 1 · Porte logiche | `gates_truth` | Le sette porte | schematic | matrix (tabella di verità) |
| | `gates_timing` | Porte nel tempo | waveform | schematic |
| | `nand_universal` | NAND è universale | schematic | — |
| | `tri_state` | Buffer tri-state | schematic | waveform |
| 2 · Sommatori | `half_adder` | Half Adder | schematic | matrix |
| | `full_adder` | Full Adder | schematic | matrix |
| | `ripple_carry` | Ripple-Carry a 4 bit | schematic | array (A, B, S) |
| | `adder_delay_compare` | Ripple vs Carry-Look-Ahead | table | — |
| | `comparator_1bit` | Comparatore a 1 bit | schematic | matrix |
| | `parity_tree` | Generatore di parità | schematic | — |
| 3 · Latch | `sr_latch_nor` | SR Latch (NOR) | schematic | — |
| | `sr_latch_timing` | SR nel tempo | waveform | schematic |
| | `d_latch` | D Latch con Enable | schematic | — |
| | `d_latch_transparency` | Trasparenza | waveform | — |
| | `sr_nor_vs_nand` | Dualità NOR / NAND | schematic | schematic |
| | `latch_glitch` | Il glitch che sopravvive | waveform | — |
| 4 · Flip-flop | `sr_flipflop` | SR Flip-Flop | schematic | — |
| | `d_flipflop_master_slave` | D Master-Slave | schematic | waveform |
| | `latch_vs_flipflop` | Latch vs Flip-Flop | waveform | — |
| | `jk_flipflop` | JK Flip-Flop | schematic | matrix |
| | `t_flipflop_divider` | T come divisore di frequenza | waveform | — |
| | `setup_hold` | Setup e Hold | waveform | — |
| | `metastability` | Metastabilità | waveform | — |
| 5 · Contatori | `ripple_counter` | Contatore asincrono | schematic | waveform |
| | `sync_counter` | Contatore sincrono | schematic | array |
| | `mod_n_counter` | Contatore Mod-N | schematic | waveform |
| | `ring_johnson` | Ring e Johnson | array | waveform |
| | `gray_counter` | Codice Gray | array | table (bit che cambiano) |
| 6 · FSM | `moore_vs_mealy` | Moore vs Mealy | graph | graph |
| | `seq_detector_101` | Rilevatore '101' (Moore) | graph | waveform, table |
| | `moore_mealy_timing` | Stesso ingresso, uscite diverse | waveform | — |
| | `traffic_light` | Semaforo | graph | table |

### Predefined inputs
The player is static and cannot recompile, so `build.sh` generates one trace per
input and the player offers them in a menu. Per entry, at minimum:

- `gates_truth` — one input per gate (7).
- `half_adder` — the 4 combinations; `full_adder` — the 8.
- `sr_latch_nor` — set / hold / reset / **vietato** (S=R=1), the last one being
  the teaching point.
- `d_latch_transparency` — one enable pattern where the glitch passes, one where
  it does not.
- `ripple_carry` — an addition with no carry, one with a carry chain across all
  4 bits, and one that overflows.
- `mod_n_counter` — N = 6 and N = 10.
- `ring_johnson` — ring and Johnson on the same width.
- `metastability` — inside the window, on the boundary, violating it.

---

## 6. Verification — `tools/check_traces.js`

Same harness as the algorithm visualizer (a `vm` context, load `catalog.js` and
every trace, assert). Rewrite the assertions. "It compiles" is not the bar: a
trace can load and still lie about what the circuit did.

Structural checks, in rough order of importance:

1. **Every wire's bit equals an independent JS evaluation of its source gate**,
   given that gate's input wires' bits, on every step of every trace. The
   schematic cannot lie about what it draws. This is the check the whole design
   rests on.
2. Truth tables match independent JS booleans for the named gate.
3. `ripple_carry`'s final sum equals `a + b` computed in JS, and the carry-out
   equals the overflow bit.
4. `full_adder`, `comparator_1bit`, `parity_tree` match independent boolean
   expressions.
5. **A D flip-flop's output equals D sampled only at rising edges**, re-derived
   in JS from the trace's own CLK and D waveforms.
6. **A D latch's output tracks D at every instant where EN = 1** — together with
   (5), this is what proves the level-sensitive / edge-triggered distinction the
   chapter exists to teach.
7. `t_flipflop_divider`: each stage's output has exactly half the transitions of
   the previous one.
8. Counters: `sync_counter` visits `0 … 2^n − 1` in order; `mod_n_counter` wraps
   at N; `gray_counter` changes **exactly one bit** per step; `ring_johnson`
   produces N states for ring and 2N for Johnson.
9. `seq_detector_101`: the output is 1 exactly at the positions where the input
   read so far ends in `101`, checked with a regex over the trace's own input
   bits. `moore_mealy_timing`: the Mealy output leads the Moore output by
   exactly one clock.
10. `sr_latch_nor` in the forbidden state records both outputs at 0, and the
    trace says so in its `note`.

Inherited generic checks, keep all of them:
- every trace is valid JS with a non-empty `steps` array;
- every step has a non-empty `note`;
- every `line` resolves to a visible source line;
- no instrumentation survives into `displaySource`;
- every catalogue entry has a trace and vice versa.

`./build.sh` runs compilation, `node --check` on every generated file, and
`check_traces.js`. `tools/smoke.sh` is separate (it takes minutes) and loads
every catalogue entry in headless Chrome from `file://`, failing on a console
error, a missing view or an empty step count.

---

## 7. Phases

Execute in order. Each phase ends green (`./build.sh` passes) before the next
starts.

**Phase 0 — scaffold, one entry end to end.**
Copy `tracer.hpp` (+2 methods), `shell.js` (2 strings), `style.css` (+palette),
`build.sh`, `smoke.sh`, `array.js`, `table.js`, `callstack.js`. Write
`cpp/sim/logic.hpp` and `cpp/sim/circuit.hpp`, the `schematic` renderer,
`web/index.html`, `web/catalog.js` with a single entry, and
`cpp/02_adders/half_adder.cpp`. Green build with one working entry proves the
whole pipeline.

**Phase 1 — the waveform renderer.**
`cpp/sim/waves.hpp`, `web/renderers/waveform.js`, and `gates_timing` as its
first consumer.

**Phase 2 — chapters 1 and 2.** The remaining gate and adder entries. All
combinational, so no feedback yet.

**Phase 3 — chapter 3, latches.** Introduces the settle loop for cross-coupled
feedback, one step per iteration.

**Phase 4 — chapter 4, flip-flops.** Clock edges, master-slave, setup/hold,
metastability. The heaviest chapter.

**Phase 5 — chapters 5 and 6.** Counters (reusing `array`) and FSMs (applying
the `graph.js` self-loop patch and the `matrix.js` `rowLabels` patch).

**Phase 6 — finish.** Complete `check_traces.js`, run `tools/smoke.sh`, write
`GUIDE.md` modelled on the algorithm visualizer's, add one bullet to the root
`README.md` next to the existing `visualizations/algorithm-visualization/` line,
and delete this `PLAN.md`.

---

## 8. Conventions

- **UI text and step notes in Italian**, matching how the material is taught.
  Code, comments and `GUIDE.md` in English, matching the rest of the repository.
- Signal names follow the notebooks (`S`, `R`, `Q`, `Qb`, `EN`, `CLK`, `D`, `J`,
  `K`, `T`, `Cin`, `Cout`), so what is on screen matches what is on the page.
- Every step carries a `note`. No exceptions — `check_traces.js` enforces it.
- The tracer chain goes immediately **after** the statement it should light up,
  because a step whose line is hidden highlights the nearest visible line above.
- Support code that cannot be written as a `T.` chain goes between
  `// tracer-hide-begin` and `// tracer-hide-end`.
- Adding an entry on an already-supported view costs **zero lines of
  JavaScript**: write the `.cpp`, add one line to `web/catalog.js`, run
  `./build.sh`.

## 9. Explicitly out of scope

- `alu.ipynb`, `memory.ipynb`, `msi_combinational.ipynb`, `registers.ipynb` —
  excluded by request. `memory.ipynb` is the largest of the notebooks and the
  natural first extension afterwards.
- Analog behaviour, SPICE, propagation delay as a continuous quantity. Delay is
  modelled as discrete gate-delay units only, which is exactly what
  `ripple_counter` needs to show delay accumulating.
- A generic netlist simulator (see §1).
- Compiling to WebAssembly for arbitrary input at runtime. A future extension,
  not an accidental limitation — same as in the algorithm visualizer.
