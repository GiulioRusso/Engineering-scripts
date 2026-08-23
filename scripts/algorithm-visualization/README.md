# 🧭 Algorithm Visualizer

An **animated, step-by-step visualizer for algorithms and data structures written in C++**, covering the
whole programme of the *Algorithm and Data Structures* book — 46 entries, from array indexing to semaphores.
Open it, pick an algorithm, and step through a real execution — forwards *and backwards* — with the source
code, the variables and a plain-language explanation next to the picture.

<p align="center">
  <img src="../../doc/algorithm_visualizer.gif" width="85%" alt="Algorithm visualizer">
</p>

> **GIF placeholder** — not recorded yet. Capture a short run of the player (binary search and the broken
> lock-variable interleaving make good subjects) and drop it at `doc/algorithm_visualizer.gif`.

## ▶️ How to open it

Double-click **`web/index.html`**. That is the whole procedure — no server, no build step, no
`pip install`, no network. The traces are committed, so a fresh clone works offline, and the folder can be
published on GitHub Pages as-is.

A specific moment can be linked directly: `web/index.html?input=1&step=14#binary_search`.

## 🧱 Why HTML + JavaScript in a Python repository

Every other folder here is a Jupyter notebook, and this one deliberately is not. The reason is the audience:
a student should be able to open a link, or double-click a file, and get the animation — with no Python
environment, no `pip install`, and no kernel. That rules out notebooks and rules in a static page. The
deviation is intentional, not accidental.

(`scripts/computer-science/` covers the same book in notebook form, with matplotlib and ipywidgets. The two
are complementary: that one is for experimenting in a kernel, this one is for showing on a projector.)

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
trees, heaps, graphs, matrices, Gantt charts, concurrent processes. A single parameterised renderer for all
of them would be unmaintainable, so the project is a **shared shell** plus a **registry of independent
renderers**.

The shell (`web/shell.js`) owns everything common, once: trace loading, the current step index,
play/pause/step/scrub, the code panel with its line highlight, the variables panel, the explanation line,
counters, keyboard and theme.

A renderer implements one interface:

```js
{
  id: "array",
  mount(container, trace, opts),  // build the initial view
  render(step, prevStep, opts),   // update to the state of this step
  legend()                        // legend entries for this kind of view
}
```

Adding an algorithm on an already-supported structure costs **zero lines of JavaScript**: you write the
`.cpp` and add one line to the catalogue.

### Primary view and auxiliary panels

Many algorithms cannot be understood through one view. A heap has to be seen **at the same time** as a tree
and as an array — every step highlights the same element in both. Dijkstra needs the graph **plus** the
distance table. Ford-Fulkerson needs the flow network **plus** the residual graph. So a trace declares a
primary view and a list of auxiliary panels, each with its own renderer, rendered in reduced format:

```js
view: "tree",
panels: [
  { view: "array", title: "Rappresentazione ad array", source: "heap" },
  { view: "callstack", title: "Stack delle chiamate" }
]
```

### Renderers

| id | What it draws |
|---|---|
| `array` | Cells with value and index, arrow markers for the pointers (`i`, `j`, `minIndex`, `low`/`high`/`mid`), coloured regions, swap animated as a translation and shift as a wave. |
| `stack` | A column growing upwards, the `top` pointer, empty slots up to `capacity`, overflow and underflow. |
| `queue` | The circular array drawn as an actual ring, with `front`, `rear` and `count`. The modulo wrap-around is the teaching point, so on a ring it is simply going round. |
| `linkedlist` | `[data\|next]` nodes with explicit arrows. A new node appears outside the chain and enters as the pointers are rehooked one at a time; a deleted node lingers as an orphan. Singly, doubly and circular. |
| `tree` | Binary and n-ary trees by levels, with the current node, the descent path and the pruned subtree. |
| `graph` | Nodes at fixed hand-placed coordinates, weighted edges, arrowheads for direction, per-edge labels (weight, `flow/capacity`, residual). |
| `matrix` | An n×n grid with the current row, column and cell highlighted. |
| `table` | Evolving key/value tables — `dist[]`, `visited[]`, `gScore`/`fScore`, `parent[]`, `rank[]` — with changed cells flashing. |
| `gantt` | Process timeline with coloured blocks, time axis, current-time marker, and the arrival/burst/waiting/turnaround table with averages. |
| `threads` | One column of code per process, the current line highlighted only in the process executing at this step, plus the shared variables and the blocked queue. |
| `callstack` | Generic panel: the pile of frames with function name and arguments. |

**Graph coordinates are placed by hand in the C++** and carried into the trace. Never a force-directed layout
at runtime: it is unstable, and an algorithm is impossible to follow when the picture rearranges itself
between steps.

## 🗂️ Layout

```
scripts/algorithm-visualization/
├── README.md
├── build.sh                     # compiles everything and regenerates every trace
├── cpp/
│   ├── tracer/
│   │   ├── tracer.hpp           # header-only, no dependencies
│   │   ├── views.hpp            # snapshot helpers: lists, trees, heaps, graphs, matrices
│   │   ├── graphs.hpp           # the example graphs, with hand-placed coordinates
│   │   ├── scheduling.hpp       # the book's process data + the Gantt snapshot
│   │   ├── threads.hpp          # the process model for the synchronization chapter
│   │   └── datasets.hpp         # the predefined array inputs
│   ├── 01_array/  02_sorting/  03_stack_queue/  04_linked_list/
│   ├── 05_tree/   06_bst/      07_heap/         08_graph/
│   ├── 09_shortest_path/       10_flow_mst/     11_scheduling/
│   └── 12_synchronization/
├── tools/
│   ├── check_traces.js          # consistency checks over the generated traces
│   └── smoke.sh                 # loads every catalogue entry in headless Chrome
├── traces/                      # generated, but COMMITTED
└── web/
    ├── index.html
    ├── shell.js                 # the player
    ├── catalog.js               # metadata only, for the sidebar
    ├── style.css
    └── renderers/               # one file per renderer
```

Traces are committed on purpose: anyone who clones the repo must be able to open `web/index.html` and see
everything **without a C++ compiler**. `build.sh` is only for people who change the algorithms. Only the
compiled binaries (`.bin/`) are git-ignored.

Traces are `.js` files that assign into `window.TRACES["<id>"]`, loaded with plain `<script>` tags. This is
deliberate: `fetch()`-ing JSON would break the page when opened from `file://` because of CORS, and
double-click use is a requirement. `catalog.js` holds metadata only; the selected algorithm's trace is
injected on demand, so 46 traces are never loaded at once.

## ✍️ Adding a new algorithm (5 steps)

1. Create `cpp/<chapter>/<id>.cpp` and instrument it with `tracer.hpp` — copy the shape of
   `cpp/02_sorting/selection_sort.cpp`, which is the smallest complete example.
2. In `main()`, declare the view, the complexity and any panels:
   `T.view("array").complexity("O(n²)", "O(1)");`
3. Emit one step per interesting moment, **always with a `note`**, and end with `T.dump("traces/<id>.js");`
4. Add one line to `web/catalog.js` under the right chapter.
5. Run `./build.sh` — it compiles, regenerates, syntax-checks and runs the consistency checks.

A step looks like this, and the whole chain is stripped from the source shown in the player:

```cpp
T.at(__LINE__, __func__)
 .vars({{"i", i}, {"j", j}, {"minIndex", minIndex}})
 .marks({{"i", i}, {"j", j}, {"minIndex", minIndex}})
 .region("sorted", 0, i - 1)
 .compare(j, minIndex)
 .note("Confronto a[j] con il minimo corrente.")
 .emit();
```

The `__LINE__` guarantees the highlighted line is the real line of the source file even after the file is
edited, so the code panel cannot drift out of sync. Instrumentation lines are stripped from the displayed
source; a step whose line is hidden highlights the **nearest visible line above**, which is why the tracer
call goes immediately *after* the statement it should light up. Support code that cannot be written as a
`T.` chain goes between `// tracer-hide-begin` and `// tracer-hide-end`, and `check_traces.js` fails the
build if any instrumentation survives into the displayed source.

## 🎨 Adding a new renderer

1. Create `web/renderers/<id>.js` and register a factory on `window.RENDERERS.<id>`.
2. Implement `mount`, `render` and `legend`. `mount` receives `opts.steps` (the current input's steps),
   `opts.compact` (true when used as an auxiliary panel), `opts.source` (which named state to read),
   `opts.layout` (graph coordinates) and `opts.seek` (to jump to a step).
3. Use the **shared semantic palette** from `style.css` (`--c-compare`, `--c-selected`, `--c-swap`,
   `--c-sorted`, `--c-visited`, `--c-frontier`, `--c-excluded`, `--c-error`). "Being compared" must look the
   same in an array and in a graph: that is what lets a student carry the intuition from one chapter to the
   next.
4. Add the `<script>` tag to `web/index.html`.

## 🔁 Regenerating the traces

```bash
cd scripts/algorithm-visualization
./build.sh                # everything
./build.sh kadane         # one algorithm
```

Requires `g++` with C++17. Everything compiles with `-Wall -Wextra -std=c++17` and no warnings. `node` is
optional; when present, `build.sh` also syntax-checks every generated file and runs `tools/check_traces.js`.

## ✅ Verification

`./build.sh` runs the first two; `tools/smoke.sh` is separate because it takes a couple of minutes.

- **Compilation** — `-Wall -Wextra -std=c++17`, no warnings.
- **`tools/check_traces.js`** — ~7000 assertions: every trace is valid JS with non-empty `steps`; every step
  has a non-empty `note`; every `line` resolves to a visible source line; no instrumentation survives in the
  displayed source; every `swap` step is followed by an array differing in exactly the two swapped indices;
  sorted output is a permutation of the input and is actually sorted; BFS and DFS visit each node exactly
  once; the final distances of Dijkstra, Bellman-Ford and A* match an **independent implementation** written
  from the trace's own graph, and Bellman-Ford's negative cycle is detected; the non-idle Gantt length equals
  the sum of the bursts and every turnaround equals waiting + burst; the scheduling comparison view agrees
  with the four traced schedulers; the "broken" synchronization interleaving really does put two processes in
  the critical section, and no other interleaving ever exceeds its declared limit.
- **`tools/smoke.sh`** — loads every catalogue entry in headless Chrome from `file://` and fails on a console
  error, a missing view or an empty step count.

## ⌨️ Controls

| Key | Action |
|---|---|
| `←` `→` | step backwards / forwards |
| `Spazio` | play / pause |
| `R` | reset |
| `Home` / `End` | first / last step |
| `1`–`9` | jump to a chapter |

Reset · step back · play/pause · step forward · scrub · speed · chapter → algorithm → input selectors are all
in the control bar. The speed slider drives both the interval between steps and the duration of the CSS
transitions. Light and dark themes, no external libraries, no CDN, fully offline.

## 🎛️ Predefined inputs

The player is static and cannot recompile, so `build.sh` generates a trace per input and the player offers
them in a menu: random / nearly-sorted / reversed / with duplicates for the sorts, key-absent for binary
search, overflow and underflow and queue wrap-around for the data structures, one input per case for
`bst_delete`, a negative edge *and* a negative cycle for Bellman-Ford, already-sorted for quick sort (to show
the O(n²) degeneration) and for bubble sort (to show the early exit). Arrays are 8–10 elements and graphs
5–7 nodes: readable on a projector.

Arbitrary input at runtime would require compiling the C++ to WebAssembly. That is a **future extension**,
not an accidental limitation.

## 📚 Catalogue

| Chapter | id | Name | View | Time | Space |
|---|---|---|---|---|---|
| Array | `array_access` | Accessing | array | O(1) | O(1) |
| Array | `array_insert` | Insert | array | O(n) | O(1) |
| Array | `array_delete` | Delete | array | O(n) | O(1) |
| Array | `linear_search` | Linear Search | array | O(n) | O(1) |
| Array | `binary_search` | Binary Search | array | O(log n) | O(1) |
| Ordinamento | `insertion_sort` | Insertion Sort | array | O(n²) | O(1) |
| Ordinamento | `selection_sort` | Selection Sort | array | O(n²) | O(1) |
| Ordinamento | `bubble_sort` | Bubble Sort | array | O(n²) | O(1) |
| Ordinamento | `merge_sort` | Merge Sort | array | O(n log n) | O(n) |
| Ordinamento | `quick_sort` | Quick Sort | array | O(n log n) | O(log n) |
| Altro | `kadane` | Kadane | array | O(n) | O(1) |
| Stack e Queue | `stack` | Stack | stack | O(1) | O(n) |
| Stack e Queue | `queue` | Queue circolare | queue | O(1) | O(n) |
| Linked List | `list_access` | Accessing | linkedlist | O(n) | O(1) |
| Linked List | `list_insert` | Insert | linkedlist | O(1)–O(n) | O(1) |
| Linked List | `list_delete` | Delete | linkedlist | O(1)–O(n) | O(1) |
| Linked List | `list_search` | Search | linkedlist | O(n) | O(1) |
| Linked List | `list_types` | Singly / Doubly / Circular | linkedlist | — | O(n) |
| Tree | `tree_basics` | Tree n-ario | tree | O(n) | O(h) |
| Tree | `tree_bfs` | Breadth-First | tree | O(n) | O(w) |
| Tree | `tree_preorder` | Pre-order | tree | O(n) | O(h) |
| Tree | `tree_inorder` | In-order | tree | O(n) | O(h) |
| Tree | `tree_postorder` | Post-order | tree | O(n) | O(h) |
| Binary Search Tree | `bst_search` | Search | tree | O(h) | O(1) |
| Binary Search Tree | `bst_insert` | Insertion | tree | O(h) | O(h) |
| Binary Search Tree | `bst_delete` | Delete (3 casi) | tree | O(h) | O(h) |
| Heap | `heap_heapify` | Heapify | tree | O(log n) | O(log n) |
| Heap | `heap_insert` | Insert | tree | O(log n) | O(1) |
| Heap | `heap_delete` | Delete | tree | O(log n) | O(log n) |
| Grafi | `graph_repr` | Matrice e lista | graph | O(1) / O(grado) | O(V²) / O(V+E) |
| Grafi | `graph_bfs` | Breadth-First Search | graph | O(V + E) | O(V) |
| Grafi | `graph_dfs` | Depth-First Search | graph | O(V + E) | O(V) |
| Cammini minimi | `dijkstra` | Dijkstra | graph | O(V²) | O(V) |
| Cammini minimi | `floyd_warshall` | Floyd-Warshall | matrix | O(V³) | O(V²) |
| Cammini minimi | `bellman_ford` | Bellman-Ford | graph | O(V·E) | O(V) |
| Cammini minimi | `a_star` | A* | graph | O(E) | O(V) |
| Flusso e MST | `ford_fulkerson` | Ford-Fulkerson | graph | O(E · f) | O(V²) |
| Flusso e MST | `kruskal` | Kruskal | graph | O(E log E) | O(V) |
| Scheduling | `fcfs` | First Come First Served | gantt | O(n log n) | O(n) |
| Scheduling | `sjf` | Shortest Job First | gantt | O(n²) | O(n) |
| Scheduling | `priority_scheduling` | Priority Scheduling | gantt | O(n²) | O(n) |
| Scheduling | `round_robin` | Round Robin | gantt | O(n) | O(n) |
| Scheduling | `scheduling_compare` | Confronto fra i quattro | table | — | — |
| Sincronizzazione | `lock_variable` | Lock variable | threads | — | O(1) |
| Sincronizzazione | `petersons_solution` | Peterson's Solution | threads | — | O(1) |
| Sincronizzazione | `semaphores` | Semafori | threads | O(1) | O(n) |

## 📝 Deviations from the book

Documented so a student comparing the two knows exactly where they differ.

**Things the book names but does not develop, implemented here:**

- `list_access` — `getAt(i)`, sequential hop-by-hop indexing. The book defines the cost but writes no code.
- `list_insert` / `list_delete` — the book implements head and tail insertion and delete-by-value only;
  **positional** insert and delete are added, because the pointer-rehooking order is the lesson.
- `list_types` — singly, doubly and circular are defined in the book but not implemented; here all three run
  the same operations side by side.
- `heap_heapify` — the book implements a min-heap; the **max-heap** variant is offered as a second input.
  The comparison direction is a parameter (`hasPriority`) rather than two duplicated functions.
- **The whole scheduling chapter** — the book has only process tables and Gantt figures, no algorithms and no
  code. All four are written here, on the book's own data (P0 0/5, P1 1/3, P2 2/8, P3 3/6; priorities
  1, 2, 5, 3; quantum 3) so the computed results match its figures. A fifth entry compares the four.

**Where the book contradicts itself, or where a choice had to be made:**

- `dijkstra` — the book's prose says priority queue, its code uses a **linear scan**. The code is what is
  implemented here, hence O(V²), and the note says so on screen.
- `kadane` — uses the reset-to-zero formulation so the moment `currentSum` goes negative is a visible event;
  `maxSum` starts at `INT_MIN` so an all-negative input still gives the right answer.
- `array_access` — a fictitious base address (`0x1000`) and `sizeof(int)`; the out-of-range case uses an
  explicit bounds check plus `std::out_of_range`, because the point is that the arithmetic would happily
  compute an address into somebody else's memory.
- `a_star` — a third input with `h = 0` was added: it is exactly Dijkstra, and it expands 5 nodes where the
  Manhattan heuristic expands 3. On this graph Manhattan and Euclidean happen to expand the same nodes, so
  without the `h = 0` case the effect of the heuristic would not be visible at all.
- `graph_repr` — the brief asks for hover interactivity; it is step-driven *and* hoverable: hovering an edge
  jumps to the step that added it, so the matrix cell and list entry light up together.
- **Graphs and trees** — the book's own figures were not available in machine-readable form, so the example
  graphs in `graphs.hpp` are small hand-laid-out graphs of the same size and character. The tree examples use
  the standard 9-node BST (8, 3, 10, 1, 6, 14, 4, 7, 13), which supports the book's "search for 4".
- **Synchronization** — no real threads. Each process is a state machine advanced one step at a time by an
  explicit scheduler, which is the only way to show a concurrency bug reliably instead of hoping it shows up.
  `lock_variable` splits `counter++` into read and write so the **lost update** is visible, not just the
  mutual-exclusion violation.

Variable names follow the book (`minIndex`, `key`, `low`/`high`/`mid`, `currentSum`/`maxSum`, `flag[]`/`turn`,
…) so that what is on screen matches what is on the page.

The UI text and the step explanations are in Italian, matching how the material is taught; this README and
the code are in English, matching the rest of the repository.

## 🔭 Possible extensions

The entries that appear only in the book's cheat-sheet are not in the catalogue, because the book does not
develop them — they are the natural next additions: hash tables, AVL and red-black trees, B-trees, radix
sort, Prim, topological sort. Compiling the algorithms to WebAssembly would additionally allow arbitrary
input at runtime.
