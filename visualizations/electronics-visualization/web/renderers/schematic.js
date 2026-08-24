// Renderer: schematic
//
// Gate boxes, elbow-free straight wires, pins. Wire colour and pin colour
// follow the logic level (the same convention the source notebooks used:
// red = driven high, grey = driven low). Coordinates come straight from the
// C++ (Schematic::json in cpp/sim/circuit.hpp) — never computed here.
(function () {
  const SVGNS = 'http://www.w3.org/2000/svg';
  const el = (tag, attrs, parent) => {
    const n = document.createElementNS(SVGNS, tag);
    for (const k in attrs) n.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(n);
    return n;
  };

  window.RENDERERS = window.RENDERERS || {};

  const GRID = 60;            // px per schematic unit at full size
  const GW = 1.0, GH = 0.8;   // gate box footprint, in schematic units

  window.RENDERERS.schematic = function createSchematicRenderer() {
    let svg, gWires, gGates, gPins, compact, scale;

    const circuitOf = (step, opts) =>
      opts && opts.source ? (step.circuits || {})[opts.source] : step.circuit;

    // Port position in schematic units, relative to a gate's centre (x, y).
    function port(gate, name) {
      const hw = GW / 2, hh = GH / 2;
      if (name === 'out') return { x: gate.x + hw, y: gate.y };
      if (name === 'a')   return { x: gate.x - hw, y: gate.y - hh * 0.5 };
      if (name === 'b')   return { x: gate.x - hw, y: gate.y + hh * 0.5 };
      return { x: gate.x, y: gate.y };
    }

    // A wire endpoint is either a pin name or "<gateId>.<port>".
    function endpoint(C, ref) {
      const dot = ref.indexOf('.');
      if (dot === -1) {
        const p = (C.pins || []).find((p) => p.name === ref);
        return p ? { x: p.x, y: p.y } : null;
      }
      const gid = ref.slice(0, dot), pname = ref.slice(dot + 1);
      const g = (C.gates || []).find((g) => g.id === gid);
      return g ? port(g, pname) : null;
    }

    function mount(container, trace, opts) {
      opts = opts || {};
      compact = !!opts.compact;
      scale = compact ? GRID * 0.62 : GRID;
      container.innerHTML = '';
      svg = el('svg', {}, container);
      gWires = el('g', {}, svg);
      gGates = el('g', {}, svg);
      gPins = el('g', {}, svg);
    }

    // bit is 0, 1, or -1 for high impedance (a tri-state buffer's disabled output).
    const cls = (bit) => (bit === 1 ? 'hi' : bit === 0 ? 'lo' : 'z');

    function render(step, prevStep, opts) {
      const C = circuitOf(step, opts);
      gWires.innerHTML = '';
      gGates.innerHTML = '';
      gPins.innerHTML = '';
      if (!C) return;

      const xs = [0], ys = [0];
      (C.gates || []).forEach((g) => { xs.push(g.x - GW / 2, g.x + GW / 2); ys.push(g.y - GH / 2, g.y + GH / 2); });
      (C.pins || []).forEach((p) => { xs.push(p.x); ys.push(p.y); });
      const minX = Math.min(...xs), maxX = Math.max(...xs, 1);
      const minY = Math.min(...ys), maxY = Math.max(...ys, 1);
      // Wide enough to fit a "NAME=-1" pin label without clipping. Not scaled
      // down for compact panels: the label is the same character count either
      // way, just a smaller font, so it needs relatively more margin room.
      const pad = 1.0;
      const width = (maxX - minX + pad * 2) * scale;
      const height = (maxY - minY + pad * 2) * scale;
      svg.setAttribute('viewBox', `0 0 ${width} ${height}`);
      svg.setAttribute('width', width);
      svg.setAttribute('height', height);

      const px = (x) => (x - minX + pad) * scale;
      const py = (y) => (y - minY + pad) * scale;

      for (const w of C.wires || []) {
        const a = endpoint(C, w.from), b = endpoint(C, w.to);
        if (!a || !b) continue;
        el('line', { class: 'wire ' + cls(w.bit),
          x1: px(a.x), y1: py(a.y), x2: px(b.x), y2: py(b.y) }, gWires);
      }

      for (const g of C.gates || []) {
        const grp = el('g', { class: 'gatebox' + (g.state === 'active' ? ' active' : ''),
          transform: `translate(${px(g.x - GW / 2)},${py(g.y - GH / 2)})` }, gGates);
        el('rect', { width: GW * scale, height: GH * scale, rx: 4 }, grp);
        const t = el('text', { x: GW * scale / 2, y: GH * scale / 2 + 4, 'text-anchor': 'middle' }, grp);
        t.textContent = g.type;
      }

      for (const p of C.pins || []) {
        const grp = el('g', { class: 'pin' }, gPins);
        el('circle', { class: cls(p.bit), cx: px(p.x), cy: py(p.y), r: compact ? 3.5 : 5 }, grp);
        const t = el('text', { class: cls(p.bit), x: px(p.x) + (p.side === 'left' ? -8 : 8),
          y: py(p.y) + 4, 'text-anchor': p.side === 'left' ? 'end' : 'start' }, grp);
        t.textContent = p.name + '=' + p.bit;
      }
    }

    function legend() {
      return [
        { color: 'var(--c-hi)', label: 'high level (1)' },
        { color: 'var(--c-lo)', label: 'low level (0)' },
        { color: 'var(--c-selected)', label: 'gate evaluated this step' },
      ];
    }

    return { id: 'schematic', mount, render, legend };
  };
})();
