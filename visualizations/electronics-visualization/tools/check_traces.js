// Consistency checks over the generated traces.
//
// "The code compiles" is not the bar: a trace can compile, load and still lie
// about what the circuit did. These checks compare the recorded steps against
// what must be true of the circuit. Extended per chapter as entries are added
// (see PLAN.md §6); right now it covers what chapter 2 needs.
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

// Independent gate evaluation, written here rather than reused from the C++ -
// if the two disagree, one of them is wrong, which is the entire point.
const GATE = {
  NOT:  (a) => 1 - a,
  AND:  (a, b) => a & b,
  OR:   (a, b) => a | b,
  NAND: (a, b) => 1 - (a & b),
  NOR:  (a, b) => 1 - (a | b),
  XOR:  (a, b) => a ^ b,
  XNOR: (a, b) => 1 - (a ^ b),
  TRI:  (d, en) => (en ? d : -1),
};

// Gate ids reachable from themselves via directed gate-to-gate wires: a
// cross-coupled latch core. A gate mid-settle can legitimately show a stale
// output relative to its OTHER input's just-updated value - that staleness
// is what a gate delay is - so the strict per-step law below only applies to
// such a gate when it is the one just recomputed (state === "active"); a
// feedforward (acyclic) gate has no excuse to ever be stale and is always
// checked in full.
function gatesInCycle(C) {
  const adj = {};
  for (const g of C.gates || []) adj[g.id] = [];
  for (const w of C.wires || []) {
    const dot = w.to.indexOf('.');
    if (dot === -1) continue;
    const destGate = w.to.slice(0, dot);
    if (adj[w.from] !== undefined && adj[destGate] !== undefined) adj[w.from].push(destGate);
  }
  const inCycle = new Set();
  for (const start of Object.keys(adj)) {
    const seen = new Set();
    const stack = [...adj[start]];
    while (stack.length) {
      const n = stack.pop();
      if (n === start) { inCycle.add(start); break; }
      if (seen.has(n)) continue;
      seen.add(n);
      stack.push(...(adj[n] || []));
    }
  }
  return inCycle;
}

// A gate's expected output given its own current input wires, or null if it
// isn't a checkable primitive (composite block, or a port left unwired).
function expectedOut(C, g) {
  const fn = GATE[g.type];
  if (!fn) return null;
  const wireTo = (port) => (C.wires || []).find((w) => w.to === g.id + '.' + port);
  const wa = wireTo('a'), wb = wireTo('b');
  if (fn.length === 1) return wa ? fn(wa.bit) : null;
  return wa && wb ? fn(wa.bit, wb.bit) : null;
}

// A step's primary circuit plus every named panel circuit (e.g. the second
// schematic in sr_nor_vs_nand) - every schematic drawn anywhere gets checked,
// not just the primary view.
function circuitsOf(step) {
  return [step.circuit, ...Object.values(step.circuits || {})].filter(Boolean);
}

// Every gate's `out` must equal its type evaluated on the bits carried by the
// wires feeding its `a` and `b` ports. This is the check the whole schematic
// renderer's honesty rests on: it never lies about what it draws because the
// bit it draws is checked against an independent evaluation of the gate.
function checkCircuit(id, input, step, i) {
  for (const C of circuitsOf(step)) {
    const inCycle = gatesInCycle(C);
    for (const g of C.gates || []) {
      if (inCycle.has(g.id) && g.state !== 'active') continue;
      const expected = expectedOut(C, g);
      if (expected === null) continue;
      check(g.out === expected, `${id}/${input}#${i}: gate ${g.id} (${g.type}) out=${g.out}, expected ${expected}`);
    }
  }
}

// By the LAST step of an input, a feedback circuit must have fully converged:
// every gate, including the cyclic ones checkCircuit above relaxes mid-settle,
// must equal its own law. Reused by every entry with cross-coupled gates.
function checkSettled(id, input, steps) {
  if (!steps.length) return;
  for (const C of circuitsOf(steps[steps.length - 1])) {
    for (const g of C.gates || []) {
      const expected = expectedOut(C, g);
      if (expected === null) continue;
      check(g.out === expected,
        `${id}/${input}: not fully settled by the last step - gate ${g.id} (${g.type}) out=${g.out}, expected ${expected}`);
    }
  }
}

// Every "out"-kind wave signal must equal an independent gate evaluation of
// the "data"-kind signals at the current time index, keyed by the out
// signal's own name (the .cpp names it after the gate type it carries).
function checkWave(id, input, step, i) {
  const W = step.wave;
  if (!W) return;
  const t = W.t;
  const dataSigs = (W.signals || []).filter((s) => s.kind !== 'out');
  const outSigs = (W.signals || []).filter((s) => s.kind === 'out');
  for (const s of W.signals || []) {
    check(t >= 0 && t < s.values.length, `${id}/${input}#${i}: wave t=${t} out of range for signal "${s.name}"`);
  }
  for (const o of outSigs) {
    const fn = GATE[o.name];
    if (!fn) continue;
    const args = dataSigs.map((s) => s.values[t]);
    const expected = fn(...args);
    check(o.values[t] === expected,
      `${id}/${input}#${i}: wave ${o.name}[${t}]=${o.values[t]}, expected ${expected}`);
  }
}

// ripple_carry draws its full adders as one "FA" box each (see checkCircuit's
// skip above), so its correctness is checked independently here instead: the
// final S bit vector plus the final carry-out must reconstruct A + B exactly.
function checkRipple(id, input, steps) {
  if (id !== 'ripple_carry' || !steps.length) return;
  const last = steps[steps.length - 1];
  const bitsA = (last.arrays || {}).A, bitsB = (last.arrays || {}).B, bitsS = (last.arrays || {}).S;
  if (!bitsA || !bitsB || !bitsS) return;
  const toInt = (bits) => bits.reduce((acc, b, i) => acc + b * (1 << i), 0);
  const a = toInt(bitsA), b = toInt(bitsB), s = toInt(bitsS);
  const cout = last.vars.Cout;
  const actualSum = cout * (1 << bitsS.length) + s;
  check(actualSum === a + b, `${id}/${input}: ripple sum ${actualSum} (Cout*2^n + S) != A+B ${a + b}`);
}

// D flip-flop family (waveform view): the output must equal D sampled only
// at CLK's rising edges, held constant otherwise - re-derived independently
// from the trace's own D/CLK arrays, taken from the LAST step (which carries
// the complete, fully-revealed waveform).
function checkEdgeSample(id, input, steps, dName, clkName, qName) {
  if (!steps.length) return;
  const W = steps[steps.length - 1].wave;
  if (!W) return;
  const sig = (name) => (W.signals || []).find((s) => s.name === name);
  const D = sig(dName), CLK = sig(clkName), Q = sig(qName);
  if (!D || !CLK || !Q) return;
  // A master-slave sampler's master tracks D continuously through the whole
  // low phase and freezes on the LAST low-phase instant, t-1 - not on the
  // edge instant t itself. Using D[t] here would demand the impossible
  // (zero setup time); D[t-1] is what the master-slave construction actually
  // guarantees, and it is the same value setup time exists to protect.
  let expected = 0;
  for (let t = 0; t < CLK.values.length; t++) {
    if (t > 0 && CLK.values[t - 1] === 0 && CLK.values[t] === 1) expected = D.values[t - 1];
    check(Q.values[t] === expected,
      `${id}/${input}: ${qName}[${t}]=${Q.values[t]}, expected ${expected} (D sampled just before the CLK rising edge)`);
  }
}

// t_flipflop_divider: each stage is an independent re-derivation of the
// master-slave toggle from the previous stage's own (already-verified) output
// - not a fuzzy "roughly half the transitions" heuristic, an exact rebuild.
function checkToggleChain(id, input, steps) {
  if (id !== 't_flipflop_divider' || !steps.length) return;
  const W = steps[steps.length - 1].wave;
  if (!W) return;
  const sig = (name) => (W.signals || []).find((s) => s.name === name);
  const toggle = (clk) => {
    const out = [];
    let Q = 0, masterQ = 0;
    for (const c of clk) { if (c === 0) masterQ = 1 - Q; else Q = masterQ; out.push(Q); }
    return out;
  };
  const CLK = sig('CLK');
  if (!CLK) return;
  let prev = CLK.values;
  for (const name of ['Q1 (/2)', 'Q2 (/4)', 'Q3 (/8)']) {
    const s = sig(name);
    if (!s) continue;
    const expected = toggle(prev);
    check(JSON.stringify(s.values) === JSON.stringify(expected),
      `${id}/${input}: ${name} does not match an independent re-derivation of the toggle chain`);
    prev = expected;
  }
}

// jk_flipflop / sr_flipflop (schematic view): the composite "MS" blocks carry
// no per-gate law (see checkCircuit's skip of unknown gate types), so their
// master/slave transitions are re-derived here instead, directly from the
// per-step vars the .cpp already records.
function checkMasterSlaveVars(id, input, steps, updateFn) {
  let Q = 0, masterQ = 0;
  for (const s of steps) {
    if (s.vars.CLK === 0) {
      const expected = updateFn(s.vars, Q);
      check(s.vars.masterQ === expected,
        `${id}/${input}: masterQ=${s.vars.masterQ}, expected ${expected}`);
      masterQ = s.vars.masterQ;
    } else {
      check(s.vars.Q === masterQ, `${id}/${input}: Q=${s.vars.Q}, expected masterQ=${masterQ} copied by the slave`);
      Q = s.vars.Q;
    }
  }
}

// sync_counter: count must advance by exactly +1 mod 16 on every step (no
// ripple in a synchronous design, so every step IS a clean, valid count).
function checkSyncCounter(id, input, steps) {
  if (id !== 'sync_counter') return;
  for (let i = 1; i < steps.length; i++) {
    const prev = steps[i - 1].vars.count, cur = steps[i].vars.count;
    check(cur === (prev + 1) % 16, `${id}/${input}#${i}: count went ${prev} -> ${cur}, expected +1 mod 16`);
  }
}

// mod_n_counter: count must never equal N (reset fires the instant the
// natural binary successor would have reached it) and must never exceed N-1.
function checkModN(id, input, steps) {
  if (id !== 'mod_n_counter' || !steps.length) return;
  const N = steps[0].vars.target;
  for (const s of steps) {
    check(s.vars.count >= 0 && s.vars.count < N,
      `${id}/${input}: count=${s.vars.count} is outside [0, ${N})`);
  }
}

// ring_johnson: a ring counter must visit exactly NBITS distinct one-hot
// states before repeating; a Johnson counter must visit exactly 2*NBITS.
function checkRingJohnson(id, input, steps) {
  if (id !== 'ring_johnson' || !steps.length) return;
  const nbits = (steps[0].arrays || {}).reg ? steps[0].arrays.reg.length : 0;
  if (!nbits) return;
  const seen = new Set();
  for (const s of steps) seen.add((s.arrays.reg || []).join(''));
  const expected = input.startsWith('Johnson') ? nbits * 2 : nbits;
  check(seen.size === expected, `${id}/${input}: visited ${seen.size} distinct states, expected ${expected}`);
}

// gray_counter: every step past the first must change exactly one bit versus
// the previous one - the entire point of a Gray code.
function checkGrayOneBit(id, input, steps) {
  if (id !== 'gray_counter') return;
  for (let i = 1; i < steps.length; i++) {
    check(steps[i].vars.bits_changed_gray === 1,
      `${id}/${input}#${i}: Gray step changed ${steps[i].vars.bits_changed_gray} bits, expected exactly 1`);
  }
}

// ripple_counter: several sub-ticks after each CLK rising edge the ripple
// must have fully settled to next = (prev + 1) mod 16 - re-derived from the
// trace's own CLK and count arrays on the final (fully-revealed) step.
function checkRippleSettles(id, input, steps) {
  if (id !== 'ripple_counter' || !steps.length) return;
  const W = steps[steps.length - 1].wave;
  if (!W) return;
  const sig = (name) => (W.signals || []).find((s) => s.name === name);
  const CLK = sig('CLK');
  const bits = ['bit0', 'bit1', 'bit2', 'bit3'].map(sig);
  if (!CLK || bits.some((b) => !b)) return;
  const countAt = (t) => bits.reduce((acc, b, i) => acc + b.values[t] * (1 << i), 0);
  const SETTLE = 8;   // sub-ticks: comfortably more than NBITS * DELAY
  let lastCount = countAt(0);
  for (let t = 1; t < CLK.values.length; t++) {
    if (CLK.values[t - 1] === 0 && CLK.values[t] === 1 && t + SETTLE < CLK.values.length) {
      const settled = countAt(t + SETTLE);
      check(settled === (lastCount + 1) % 16,
        `${id}/${input}: ${SETTLE} sub-ticks after the edge at t=${t}, count=${settled}, expected ${(lastCount + 1) % 16}`);
      lastCount = settled;
    }
  }
}

// seq_detector_101: the output must be 1 exactly at the positions where the
// input read so far ends in "101" - checked with a regex over the trace's
// own recorded input bits, independent of the FSM's internal state labels.
function checkSeqDetector(id, input, steps) {
  if (id !== 'seq_detector_101' || !steps.length) return;
  const W = steps[steps.length - 1].wave;
  if (!W) return;
  const sig = (name) => (W.signals || []).find((s) => s.name === name);
  const IN = sig('in'), OUT = sig('out');
  if (!IN || !OUT) return;
  let seen = '';
  for (let t = 0; t < IN.values.length; t++) {
    seen += IN.values[t];
    const expected = seen.endsWith('101') ? 1 : 0;
    check(OUT.values[t] === expected,
      `${id}/${input}: out[${t}]=${OUT.values[t]}, expected ${expected} (input read so far: "${seen}")`);
  }
}

// moore_mealy_timing: the Mealy output must lead the Moore output by exactly
// one step, on the SAME input stream.
function checkMooreMealyLead(id, input, steps) {
  if (id !== 'moore_mealy_timing' || !steps.length) return;
  const W = steps[steps.length - 1].wave;
  if (!W) return;
  const sig = (name) => (W.signals || []).find((s) => s.name === name);
  const mealy = sig('out_mealy'), moore = sig('out_moore');
  if (!mealy || !moore) return;
  for (let t = 1; t < moore.values.length; t++) {
    check(moore.values[t] === mealy.values[t - 1],
      `${id}/${input}: out_moore[${t}]=${moore.values[t]}, expected out_mealy[${t - 1}]=${mealy.values[t - 1]}`);
  }
  check(moore.values[0] === 0, `${id}/${input}: out_moore[0]=${moore.values[0]}, expected 0 before any input`);
}

for (const id in TRACES) {
  const t = TRACES[id];
  check(Array.isArray(t.inputs) && t.inputs.length > 0, `${id}: no inputs`);
  for (const inp of t.inputs || []) {
    check(Array.isArray(inp.steps) && inp.steps.length > 0, `${id}/${inp.label}: no steps`);
    inp.steps.forEach((s, i) => {
      check(!!s.note, `${id}/${inp.label}#${i}: empty note`);
      const dispIdx = t.lineMap[s.line];
      check(dispIdx !== undefined, `${id}/${inp.label}#${i}: line ${s.line} not in lineMap`);
      if (dispIdx !== undefined) {
        check((t.displaySource[dispIdx] || '').trim() !== '',
          `${id}/${inp.label}#${i}: line ${s.line} resolves to a blank displaySource line (check for a blank line between the statement and its T.at() chain)`);
      }
      checkCircuit(id, inp.label, s, i);
      checkWave(id, inp.label, s, i);
    });
    checkRipple(id, inp.label, inp.steps);
    checkSettled(id, inp.label, inp.steps);
    checkToggleChain(id, inp.label, inp.steps);
    if (id === 'd_flipflop_master_slave') checkEdgeSample(id, inp.label, inp.steps, 'D', 'CLK', 'Q');
    if (id === 'latch_vs_flipflop') checkEdgeSample(id, inp.label, inp.steps, 'D', 'CLK', 'Q_ff');
    if (id === 'sr_flipflop') {
      checkMasterSlaveVars(id, inp.label, inp.steps,
        (v, Q) => (v.S ? 1 : (v.R ? 0 : Q)));
    }
    if (id === 'jk_flipflop') {
      checkMasterSlaveVars(id, inp.label, inp.steps,
        (v, Q) => (v.J & (1 - Q)) | ((1 - v.K) & Q));
    }
    checkSyncCounter(id, inp.label, inp.steps);
    checkModN(id, inp.label, inp.steps);
    checkRingJohnson(id, inp.label, inp.steps);
    checkGrayOneBit(id, inp.label, inp.steps);
    checkRippleSettles(id, inp.label, inp.steps);
    checkSeqDetector(id, inp.label, inp.steps);
    checkMooreMealyLead(id, inp.label, inp.steps);
  }
  // no instrumentation should have survived into the displayed source
  (t.displaySource || []).forEach((line, i) => {
    check(!/\bT\.at\(|\.emit\(\);|^\s*Tracer\s/.test(line),
      `${id}: instrumentation leaked into displaySource line ${i + 1}: "${line}"`);
  });
}

// every catalogue entry has a trace and vice versa
const catIds = new Set();
for (const g of CATALOG) for (const item of g.items) catIds.add(item.id);
for (const id of catIds) check(!!TRACES[id], `catalog entry "${id}" has no trace`);
for (const id in TRACES) check(catIds.has(id), `trace "${id}" is not in the catalog`);

console.log(`  ${checks} checks, ${failures} failure(s)`);
process.exit(failures ? 1 : 0);
