// Renderer: graph
//
// Nodes at fixed, hand-placed coordinates carried in the trace — never a
// force-directed layout computed at runtime, which would rearrange the picture
// between steps and make the algorithm impossible to follow. Edges carry a
// customisable label (weight, flow/capacity, residual) and an arrowhead when
// directed.
(function () {
  const SVGNS = 'http://www.w3.org/2000/svg';
  const el = (tag, attrs, parent) => {
    const n = document.createElementNS(SVGNS, tag);
    for (const k in attrs) n.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(n);
    return n;
  };

  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.graph = function createGraphRenderer() {
    let svg, gEdges, gNodes, seqText, pos = {}, R, compact, seekByEdge = null, seek = null;

    const graphOf = (step, opts) =>
      opts && opts.source ? (step.graphs || {})[opts.source] : step.graph;
    const edgeKey = (e) => e.from + '->' + e.to;

    function mount(container, trace, opts) {
      opts = opts || {};
      compact = !!opts.compact;
      R = compact ? 13 : 20;
      seek = opts.seek || null;

      const layout = opts.layout || {};
      const ids = Object.keys(layout);
      const xs = ids.map((k) => layout[k].x);
      const ys = ids.map((k) => layout[k].y);
      const minX = Math.min(...xs, 0), maxX = Math.max(...xs, 0);
      const minY = Math.min(...ys, 0), maxY = Math.max(...ys, 0);
      const pad = R + (compact ? 14 : 22);
      const width = (maxX - minX) + pad * 2;
      const height = (maxY - minY) + pad * 2;

      pos = {};
      for (const id of ids) pos[id] = { x: layout[id].x - minX + pad, y: layout[id].y - minY + pad };

      // Hovering an edge jumps to the step that edge belongs to. That is what
      // makes the adjacency view explorable rather than only playable.
      seekByEdge = {};
      (opts.steps || []).forEach((s, i) => {
        const a = s.action;
        if (a && a.from !== undefined && a.to !== undefined && seekByEdge[a.from + '->' + a.to] === undefined) {
          seekByEdge[a.from + '->' + a.to] = i;
        }
      });

      container.innerHTML = '';
      svg = el('svg', { viewBox: `0 0 ${width} ${height}`, width, height }, container);
      const defs = el('defs', {}, svg);
      [['g-arrow', 'var(--ink-dim)'], ['g-arrow-hot', 'var(--c-selected)'],
       ['g-arrow-ok', 'var(--c-sorted)'], ['g-arrow-off', 'var(--c-excluded)']].forEach(([id, color]) => {
        const m = el('marker', { id, viewBox: '0 0 10 10', refX: 10, refY: 5,
                                 markerWidth: 6, markerHeight: 6, orient: 'auto-start-reverse' }, defs);
        el('path', { d: 'M 0 0 L 10 5 L 0 10 z', fill: color }, m);
      });
      gEdges = el('g', {}, svg);
      gNodes = el('g', {}, svg);

      // The output sequence (the book's "Visited" row) sits under the graph.
      const hasSeq = (opts.steps || []).some((s) => s.sequence);
      if (hasSeq) {
        svg.setAttribute('viewBox', `0 0 ${width} ${height + (compact ? 18 : 24)}`);
        svg.setAttribute('height', height + (compact ? 18 : 24));
        seqText = el('text', { x: 6, y: height + (compact ? 12 : 17), 'text-anchor': 'start',
                               fill: 'var(--ink)', 'font-size': compact ? 11 : 13,
                               'font-family': 'var(--mono)' }, svg);
      } else {
        seqText = null;
      }
    }

    function render(step, prevStep, opts) {
      const G = graphOf(step, opts);
      gEdges.innerHTML = '';
      gNodes.innerHTML = '';
      if (seqText) seqText.textContent = step.sequence || '';
      if (!G) return;

      // Two nodes joined in both directions would otherwise draw one line with
      // both weights piled on it; shift each side off the axis so both are legible.
      const reciprocal = new Set();
      for (const e of G.edges || []) {
        if ((G.edges || []).some((o) => o.from === e.to && o.to === e.from)) {
          reciprocal.add(e.from + '->' + e.to);
        }
      }

      for (const e of G.edges || []) {
        // A self-loop (e.from === e.to) has zero length as a straight line;
        // an FSM staying in the same state on some input is common enough
        // that it needs its own arc, drawn as a small loop above the node.
        if (e.from === e.to) {
          const p = pos[e.from];
          if (!p) continue;
          const loopR = compact ? 13 : 18;
          const x0 = p.x - R * 0.6, y0 = p.y - R * 0.85;
          const x1 = p.x + R * 0.6, y1 = p.y - R * 0.85;
          const cx1 = p.x - loopR, cy1 = p.y - R - loopR * 1.6;
          const cx2 = p.x + loopR, cy2 = p.y - R - loopR * 1.6;
          const path = el('path', { class: 'gedge selfloop ' + (e.state || ''),
            d: `M ${x0} ${y0} C ${cx1} ${cy1} ${cx2} ${cy2} ${x1} ${y1}`, fill: 'none' }, gEdges);
          if (e.directed) {
            path.setAttribute('marker-end',
              e.state === 'path' || e.state === 'current' ? 'url(#g-arrow-hot)'
              : e.state === 'accepted' ? 'url(#g-arrow-ok)'
              : e.state === 'rejected' ? 'url(#g-arrow-off)' : 'url(#g-arrow)');
          }
          if (seek && seekByEdge && seekByEdge[edgeKey(e)] !== undefined) {
            path.classList.add('seekable');
            path.addEventListener('mouseenter', () => seek(seekByEdge[edgeKey(e)]));
          }
          if (e.label) {
            const t = el('text', { class: 'glabel ' + (e.state || ''),
              x: p.x, y: p.y - R - loopR * 1.6 - 4, 'text-anchor': 'middle' }, gEdges);
            t.textContent = e.label;
            if (compact) t.setAttribute('font-size', '9');
          }
          continue;
        }

        const a = pos[e.from], b = pos[e.to];
        if (!a || !b) continue;
        const dx = b.x - a.x, dy = b.y - a.y;
        const len = Math.hypot(dx, dy) || 1;
        const ux = dx / len, uy = dy / len;
        const sep = reciprocal.has(e.from + '->' + e.to) ? (compact ? 4 : 6) : 0;
        const ox = -uy * sep, oy = ux * sep;
        const x1 = a.x + ux * R + ox, y1 = a.y + uy * R + oy;
        const x2 = b.x - ux * R + ox, y2 = b.y - uy * R + oy;

        const line = el('line', { class: 'gedge ' + (e.state || ''), x1, y1, x2, y2 }, gEdges);
        if (e.directed) {
          line.setAttribute('marker-end',
            e.state === 'path' || e.state === 'current' ? 'url(#g-arrow-hot)'
            : e.state === 'accepted' ? 'url(#g-arrow-ok)'
            : e.state === 'rejected' ? 'url(#g-arrow-off)' : 'url(#g-arrow)');
        }
        if (seek && seekByEdge && seekByEdge[edgeKey(e)] !== undefined) {
          line.classList.add('seekable');
          line.addEventListener('mouseenter', () => seek(seekByEdge[edgeKey(e)]));
        }

        if (e.label) {
          const mx = (x1 + x2) / 2, my = (y1 + y2) / 2;
          const nx = -uy, ny = ux;                       // offset off the line
          const off = compact ? 8 : 11;
          const t = el('text', { class: 'glabel ' + (e.state || ''),
                                 x: mx + nx * off, y: my + ny * off + 4,
                                 'text-anchor': 'middle' }, gEdges);
          t.textContent = e.label;
          if (compact) t.setAttribute('font-size', '9');
        }
      }

      for (const id in G.nodes || {}) {
        const p = pos[id];
        if (!p) continue;
        const g = el('g', { class: 'gnode ' + (G.nodes[id].state || ''),
                            transform: `translate(${p.x},${p.y})` }, gNodes);
        el('circle', { r: R }, g);
        const t = el('text', { y: 5, 'text-anchor': 'middle' }, g);
        t.textContent = id;
        if (compact) t.setAttribute('font-size', '10');
        if (G.nodes[id].label) {
          const l = el('text', { class: 'glabel-node', y: -R - 6, 'text-anchor': 'middle' }, g);
          l.textContent = G.nodes[id].label;
          if (compact) l.setAttribute('font-size', '8');
        }
      }
    }

    function legend() {
      return [
        { color: 'var(--c-neutral)',  label: 'unvisited' },
        { color: 'var(--c-frontier)', label: 'in the frontier' },
        { color: 'var(--c-visited)',  label: 'visited' },
        { color: 'var(--c-selected)', label: 'current node or edge' },
        { color: 'var(--c-sorted)',   label: 'on the path / accepted' },
      ];
    }

    return { id: 'graph', mount, render, legend };
  };
})();
