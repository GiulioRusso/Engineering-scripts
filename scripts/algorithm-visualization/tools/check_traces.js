// Consistency checks over the generated traces.
//
// "The code compiles" is not the bar: a trace can compile, load and still lie
// about what the algorithm did. These checks compare the recorded steps against
// what must be true of the algorithm.
'use strict';
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const root = path.join(__dirname, '..');
const sandbox = { window: { TRACES: {}, CATALOG: null } };
vm.createContext(sandbox);

function load(file) {
  vm.runInContext(fs.readFileSync(file, 'utf8'), sandbox, { filename: file });
}

load(path.join(root, 'web/catalog.js'));
for (const f of fs.readdirSync(path.join(root, 'traces')).sort()) {
  if (f.endsWith('.js')) load(path.join(root, 'traces', f));
}

const TRACES = sandbox.window.TRACES;
const CATALOG = sandbox.window.CATALOG;
let failures = 0;
let checks = 0;
let distanceComparisons = 0;

function check(ok, msg) {
  checks++;
  if (!ok) { failures++; console.error('  FAIL ' + msg); }
}

const isSorted = (a) => a.every((v, i) => i === 0 || a[i - 1] <= v);

// An independent shortest-path implementation, written here from the trace's own
// graph rather than from the C++. If the two disagree, one of them is wrong —
// which is the entire point of checking against a second implementation.
function referenceDistances(edges, source) {
  const nodes = new Set();
  const list = [];
  for (const e of edges) {
    nodes.add(e.from);
    nodes.add(e.to);
    const w = parseInt(e.label, 10);
    if (Number.isNaN(w)) continue;
    list.push([e.from, e.to, w]);
    if (!e.directed) list.push([e.to, e.from, w]);
  }
  const dist = {};
  for (const n of nodes) dist[n] = Infinity;
  dist[source] = 0;
  // Bellman-Ford: handles the negative weights that Dijkstra could not.
  for (let i = 0; i < nodes.size - 1; i++) {
    for (const [u, v, w] of list) {
      if (dist[u] + w < dist[v]) dist[v] = dist[u] + w;
    }
  }
  return dist;
}

const asNumber = (v) => (v === 'inf' || v === '∞' ? Infinity : Number(v));

// id -> { table, source } for the traces whose final distances we can verify.
const DISTANCE_CHECKS = {
  dijkstra: { table: 'dist', sourceVar: 'sorgente' },
  bellman_ford: { table: null, sourceVar: 'sorgente' },
  a_star: { table: 'gScore', sourceVar: 'start' },
};
const SORTS = ['insertion_sort', 'selection_sort', 'bubble_sort', 'merge_sort', 'quick_sort'];

// Every catalog entry must have a trace, and vice versa.
const catalogIds = CATALOG.flatMap((g) => g.items.map((i) => i.id));
for (const id of catalogIds) check(!!TRACES[id], `catalogo: manca la traccia ${id}`);
for (const id of Object.keys(TRACES)) {
  check(catalogIds.includes(id), `traccia ${id} non presente nel catalogo`);
}

for (const id of Object.keys(TRACES).sort()) {
  const t = TRACES[id];
  console.log(`- ${id}`);

  check(Array.isArray(t.inputs) && t.inputs.length > 0, `${id}: nessun input`);
  check(Array.isArray(t.displaySource) && t.displaySource.length > 0, `${id}: displaySource vuoto`);
  // The source panel must show the algorithm and nothing else: any tracer call
  // left in it is noise the student has to read past.
  const INSTRUMENTATION = /(^|[^\w])T\.(at|dump|input|panel|watch|layout|view|clear|complexity)\(|\.emit\(\);|emitTables\(/;
  const leaks = t.displaySource.filter((l) => INSTRUMENTATION.test(l));
  check(leaks.length === 0,
        `${id}: ${leaks.length} righe di instrumentazione rimaste in displaySource, es. ${JSON.stringify(leaks[0])}`);
  check(!!t.complexity && t.complexity.time !== '?', `${id}: complessità non dichiarata`);

  t.inputs.forEach((input, ii) => {
    const where = `${id}/"${input.label}"`;
    check(input.steps.length > 0, `${where}: nessun passo`);

    input.steps.forEach((s, si) => {
      check(typeof s.note === 'string' && s.note.trim().length > 0,
            `${where} passo ${si}: note vuota`);
      // The line must resolve to a visible source line.
      let idx = t.lineMap[s.line];
      if (idx === undefined) {
        for (let l = s.line; l > 0 && idx === undefined; l--) idx = t.lineMap[l];
      }
      check(idx !== undefined && idx >= 0 && idx < t.displaySource.length,
            `${where} passo ${si}: line ${s.line} non risolve a una riga visibile`);

      // A swap step shows the array *before* the swap; the next step must
      // differ in exactly the two swapped indices.
      const a = s.action;
      if (a && a.kind === 'swap' && si + 1 < input.steps.length) {
        // Whatever the primary array is called: 'a' for the sorts, 'heap' for
        // the heap, 'data' for the stack and queue.
        const key = s.arrays && Object.keys(s.arrays)[0];
        const cur = key ? s.arrays[key] : null;
        const nxt = key && input.steps[si + 1].arrays ? input.steps[si + 1].arrays[key] : null;
        if (cur && nxt && cur.length === nxt.length) {
          const diff = cur.map((v, k) => (v === nxt[k] ? -1 : k)).filter((k) => k >= 0);
          // Swapping equal values (duplicates, or i === j) is a legitimate no-op
          // and must leave the array untouched.
          const noop = cur[a.a] === cur[a.b];
          check(noop ? diff.length === 0
                     : diff.length === 2 && diff.includes(a.a) && diff.includes(a.b),
                `${where} passo ${si}: swap(${a.a},${a.b}) ma l'array cambia in [${diff}]`);
          check(cur[a.a] === nxt[a.b] && cur[a.b] === nxt[a.a],
                `${where} passo ${si}: i valori scambiati non corrispondono`);
        }
      }
    });

    // BFS and DFS must visit every node exactly once: a duplicate means the
    // visited[] guard is in the wrong place.
    if (['graph_bfs', 'graph_dfs', 'tree_bfs'].includes(id)) {
      const seen = [];
      const dup = [];
      for (const s of input.steps) {
        const a = s.action;
        if (a && a.kind === 'visit') {
          if (seen.includes(a.node)) dup.push(a.node);
          else seen.push(a.node);
        }
      }
      check(dup.length === 0, `${where}: nodi visitati più di una volta: [${dup}]`);
      check(seen.length > 0, `${where}: nessun nodo visitato`);
    }

    const dc = DISTANCE_CHECKS[id];
    if (dc) {
      const first = input.steps[0];
      const last = input.steps[input.steps.length - 1];
      const negativeCycle = input.steps.some(
        (s) => s.action && s.action.kind === 'error' && s.action.what === 'ciclo_negativo');
      const source = first.vars && first.vars[dc.sourceVar];
      const edges = (first.graph && first.graph.edges) || [];
      // With a negative cycle no shortest path exists, so there is nothing to
      // compare against; the detection itself is checked separately.
      if (!negativeCycle && source && edges.length && last.tables) {
        const tableName = dc.table || Object.keys(last.tables).pop();
        const got = last.tables[tableName];
        const want = referenceDistances(edges, source);
        for (const node in got) {
          if (want[node] === undefined) continue;
          distanceComparisons++;
          check(asNumber(got[node]) === want[node],
                `${where}: dist[${node}] = ${got[node]}, l'implementazione di riferimento dice ${want[node]}`);
        }
      }
      if (id === 'bellman_ford' && input.label.startsWith('ciclo negativo')) {
        check(negativeCycle, `${where}: il ciclo negativo non è stato rilevato`);
      }
    }

    if (SORTS.includes(id)) {
      const first = input.steps.find((s) => s.arrays && s.arrays.a);
      const last = [...input.steps].reverse().find((s) => s.arrays && s.arrays.a);
      check(!!last && isSorted(last.arrays.a), `${where}: l'array finale non è ordinato`);
      check(!!first && first.arrays.a.length === last.arrays.a.length,
            `${where}: la lunghezza dell'array cambia durante la traccia`);
      const bag = (x) => [...x].sort((p, q) => p - q).join(',');
      check(bag(first.arrays.a) === bag(last.arrays.a),
            `${where}: gli elementi finali non sono una permutazione di quelli iniziali`);
    }
  });
}

console.log(`\n${checks} controlli (di cui ${distanceComparisons} distanze verificate contro ` +
            `un'implementazione indipendente), ${failures} falliti`);
process.exit(failures ? 1 : 0);
