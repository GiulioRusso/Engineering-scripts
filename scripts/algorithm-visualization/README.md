# 🧭 Algorithm Visualizer

An **animated, step-by-step visualizer for algorithms and data structures written in C++**, covering the
programme of the *Algorithm and Data Structures* book. Open it, pick an algorithm, and step through a real
execution — forwards *and backwards* — with the source code, the variables and a plain-language explanation
next to the picture.

<p align="center">
  <img src="../../doc/algorithm_visualizer.gif" width="85%" alt="Algorithm visualizer">
</p>

> **Placeholder:** the GIF above is not committed yet. Record a short capture of the player and drop it at
> `doc/algorithm_visualizer.gif`.

## ▶️ How to open it

Double-click **`web/index.html`**. That is the whole procedure — no server, no build, no `pip install`,
no network. The traces are committed, so a fresh clone works offline and the folder can be published on
GitHub Pages as-is.

## 🧱 Why HTML + JavaScript in a Python repository

Every other folder here is a Jupyter notebook, and this one deliberately is not. The reason is the audience:
a student should be able to open a link, or double-click a file, and get the animation — with no Python
environment, no `pip install`, and no kernel. That rules out notebooks and rules in a static page.
The deviation is intentional, not accidental.

## 🧠 Architecture: trace, not re-execution

This is the one design decision the whole project rests on.

```
algoritmo.cpp  →  compiled and executed at build time  →  trace  →  player in the browser
```

The algorithms stay **real C++ algorithms** and are the single source of truth. They are never rewritten in
JavaScript. Each interesting step is recorded as a snapshot, and the browser player **executes nothing**: it
replays a list of states that a real run already produced.

The payoff is concrete. With a materialised trace, **stepping backwards is free** — it is an index
decrement. That is the single most important feature of a teaching tool, because it is the moment a student
says *"wait, go back"*. With live execution, stepping back is nearly impossible, and for recursive
algorithms it is impossible outright.

### Shell + renderer registry

The book covers structures that have nothing in common — arrays, stacks, circular queues, linked lists,
trees, heaps, graphs, matrices, Gantt charts, concurrent threads. A single parameterised renderer for all of
them would be unmaintainable, so the project is a **shared shell** plus a **registry of independent
renderers**.

The shell (`web/shell.js`) owns everything common, once: trace loading, the current step index,
play/pause/step/scrub, the code panel with its line highlight, the variables panel, the explanation line,
counters, keyboard, theme.

A renderer implements one interface:

```js
{
  id: "array",
  mount(container, trace, opts),  // build the initial SVG
  render(step, prevStep, opts),   // update to the state of this step
  legend()                        // legend entries for this kind of view
}
```

Adding an algorithm on an already-supported structure costs **zero lines of JavaScript**: you write the
`.cpp` and nothing else.

### Primary view and auxiliary panels

Many algorithms cannot be understood through one view. A heap has to be seen **at the same time** as a tree
and as an array. Dijkstra has to be seen as a graph **plus** a distance table. So a trace declares a primary
view and a list of auxiliary panels, each with its own renderer, rendered in reduced format:

```js
view: "tree",
panels: [
  { view: "array", title: "Rappresentazione ad array", source: "heapArray" },
  { view: "callstack", title: "Stack delle chiamate" }
]
```

## 🗂️ Layout

```
scripts/algorithm-visualization/
├── README.md
├── build.sh              # compiles everything and regenerates every trace
├── cpp/
│   ├── tracer/           # tracer.hpp (header-only) + datasets.hpp
│   ├── 01_array/
│   └── 02_sorting/
├── tools/
│   ├── check_traces.js   # consistency checks over the generated traces
│   └── smoke.sh          # loads every catalog entry in headless Chrome
├── traces/               # generated, but COMMITTED
└── web/
    ├── index.html
    ├── shell.js
    ├── catalog.js        # metadata only, for the sidebar
    ├── style.css
    └── renderers/        # one file per renderer
```

Traces are committed on purpose: anyone who clones the repo must be able to open `web/index.html` and see
everything **without a C++ compiler**. `build.sh` is only for people who change the algorithms. Only the
compiled binaries (`.bin/`) are git-ignored.

Traces are `.js` files that assign into `window.TRACES["<id>"]`, loaded with plain `<script>` tags. This is
deliberate: `fetch()`-ing JSON would break the page when opened from `file://` because of CORS, and
double-click use is a requirement. `catalog.js` holds metadata only; the selected algorithm's trace is
injected on demand, so dozens of traces are never loaded at once.

## ✍️ Adding a new algorithm (5 steps)

1. Create `cpp/<chapter>/<id>.cpp` and instrument it with `tracer.hpp` (copy the shape of
   `cpp/02_sorting/selection_sort.cpp`).
2. Declare the view and complexity in `main()`: `T.view("array").complexity("O(n²)", "O(1)");`
3. Emit one step per interesting moment, always with a `note`, and end with `T.dump("traces/<id>.js");`
4. Add the entry to `web/catalog.js` under the right chapter.
5. Run `./build.sh` — it compiles, regenerates, syntax-checks and runs the consistency checks.

The `__LINE__` in `T.at(__LINE__, __func__)` guarantees the highlighted line is the real line of the source
file even after the file is edited, so the code panel can never drift out of sync. Instrumentation lines are
stripped from the source shown in the player; a step whose line is hidden highlights the **nearest visible
line above**, which is why the tracer call goes immediately *after* the statement it should light up.

## 🎨 Adding a new renderer

1. Create `web/renderers/<id>.js` and register a factory on `window.RENDERERS.<id>`.
2. Implement `mount`, `render` and `legend`. `mount` receives `opts.steps` (the current input's steps),
   `opts.compact` (true when used as an auxiliary panel) and `opts.source`.
3. Use the **shared semantic palette** from `style.css` (`--c-compare`, `--c-selected`, `--c-swap`,
   `--c-sorted`, `--c-visited`, `--c-frontier`, `--c-excluded`, `--c-error`). "Being compared" must look the
   same in an array and in a graph: that is what lets a student carry the intuition from one chapter to the
   next.
4. Add the `<script>` tag to `web/index.html`.

## 🔁 Regenerating the traces

```bash
cd scripts/algorithm-visualization
./build.sh            # everything
./build.sh kadane     # one algorithm
```

Requires `g++` with C++17. Everything compiles with `-Wall -Wextra -std=c++17` and no warnings. `node` is
optional; when present, `build.sh` also syntax-checks every generated file and runs `tools/check_traces.js`.

## ✅ Verification

- `./build.sh` — compiles warning-free, regenerates and checks every trace.
- `tools/check_traces.js` — every step has a non-empty note; every `line` resolves to a visible source line;
  every `swap` step is followed by an array that differs in exactly the two swapped indices; sorted output is
  a permutation of the input and is actually sorted; catalog and traces agree.
- `tools/smoke.sh` — loads every catalog entry in headless Chrome from `file://` and fails on a console
  error, a missing SVG or an empty step count.

## ⌨️ Controls

| Key | Action |
|---|---|
| `←` `→` | step backwards / forwards |
| `Spazio` | play / pause |
| `R` | reset |
| `Home` / `End` | first / last step |
| `1`–`9` | jump to a chapter |

A step can also be deep-linked: `web/index.html?input=1&step=14#binary_search`.

## 🎛️ Predefined inputs

The player is static and cannot recompile, so `build.sh` generates a trace per input and the player offers
them in a menu. Arrays are 8–10 elements and graphs 5–7 nodes: readable on a projector.

Arbitrary input at runtime would require compiling the C++ to WebAssembly. That is a **future extension**,
not an accidental limitation.

## 📚 Catalogue

| Chapter | id | Name | Time | Space |
|---|---|---|---|---|
| Array | `array_access` | Accessing | O(1) | O(1) |
| Array | `array_insert` | Insert | O(n) | O(1) |
| Array | `array_delete` | Delete | O(n) | O(1) |
| Array | `linear_search` | Linear Search | O(n) | O(1) |
| Array | `binary_search` | Binary Search | O(log n) | O(1) |
| Sorting | `insertion_sort` | Insertion Sort | O(n²) | O(1) |
| Sorting | `selection_sort` | Selection Sort | O(n²) | O(1) |
| Sorting | `bubble_sort` | Bubble Sort | O(n²) | O(1) |
| Other | `kadane` | Kadane | O(n) | O(1) |

*(The remaining chapters — stack/queue, linked lists, trees, BST, heap, graphs, shortest paths, flow and MST,
scheduling, synchronization — are being added milestone by milestone.)*

## 📝 Deviations from the book

Documented so a student comparing the two knows exactly where they differ:

- **`kadane`** uses the reset-to-zero formulation, so the moment `currentSum` goes negative is a visible
  event; `maxSum` starts at `INT_MIN` so an all-negative input still gives the right answer.
- **Address arithmetic** in `array_access` uses a fictitious base address (`0x1000`) and `sizeof(int)`; the
  out-of-range case is shown with an explicit bounds check plus `std::out_of_range`, because the point is
  that the arithmetic would happily compute an address into somebody else's memory.

The UI text and the step explanations are in Italian, matching how the material is taught; this README and
the code are in English, matching the rest of the repository.
