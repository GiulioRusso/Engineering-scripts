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

function check(ok, msg) {
  checks++;
  if (!ok) { failures++; console.error('  FAIL ' + msg); }
}

const isSorted = (a) => a.every((v, i) => i === 0 || a[i - 1] <= v);
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
  check(!t.displaySource.some((l) => l.trim().startsWith('T.')),
        `${id}: righe di instrumentazione rimaste in displaySource`);
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

console.log(`\n${checks} controlli, ${failures} falliti`);
process.exit(failures ? 1 : 0);
