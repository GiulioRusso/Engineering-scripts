// Renderer: linkedlist
//
// Nodes drawn as [data|next] with explicit arrows. A new node appears *outside*
// the chain and enters it as the pointers are rehooked, one at a time and in
// the right order; a deleted node lingers for a moment as an orphan before it
// disappears. Handles singly, doubly and circular lists.
(function () {
  const SVGNS = 'http://www.w3.org/2000/svg';
  const el = (tag, attrs, parent) => {
    const n = document.createElementNS(SVGNS, tag);
    for (const k in attrs) n.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(n);
    return n;
  };

  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.linkedlist = function createLinkedListRenderer() {
    let svg, gEdges, gNodes, gLabels;
    const nodes = new Map();          // id -> { g, data, box }
    let DW, PW, NW, NH, XGAP, PAD, chainY, looseY, compact;
    let width = 0;

    const xOf = (order) => PAD + order * (NW + XGAP);

    function mount(container, trace, opts) {
      opts = opts || {};
      compact = !!opts.compact;
      DW = compact ? 34 : 46;
      PW = compact ? 20 : 26;
      NW = DW + PW;
      NH = compact ? 26 : 34;
      XGAP = compact ? 26 : 36;
      PAD = 10;

      let maxLen = 1;
      for (const s of opts.steps || []) {
        const L = opts.source ? (s.lists || {})[opts.source] : s.list;
        if (L && L.nodes.length > maxLen) maxLen = L.nodes.length;
      }

      looseY = 6;
      chainY = looseY + NH + (compact ? 26 : 34);
      const totalH = chainY + NH + (compact ? 40 : 52);
      width = PAD * 2 + maxLen * (NW + XGAP) + (compact ? 40 : 54);

      container.innerHTML = '';
      svg = el('svg', { viewBox: `0 0 ${width} ${totalH}`, width, height: totalH }, container);

      const defs = el('defs', {}, svg);
      const mk = (id, color) => {
        const m = el('marker', { id, viewBox: '0 0 10 10', refX: 9, refY: 5,
                                 markerWidth: 6, markerHeight: 6, orient: 'auto-start-reverse' }, defs);
        el('path', { d: 'M 0 0 L 10 5 L 0 10 z', fill: color }, m);
      };
      mk('ll-arrow', 'var(--ink-dim)');
      mk('ll-arrow-hot', 'var(--c-selected)');
      mk('ll-arrow-dead', 'var(--c-error)');

      gEdges = el('g', {}, svg);
      gNodes = el('g', {}, svg);
      gLabels = el('g', {}, svg);
      nodes.clear();
    }

    function nodeFor(id) {
      let n = nodes.get(id);
      if (n) return n;
      const g = el('g', { class: 'lnode' }, gNodes);
      el('rect', { class: 'box', width: NW, height: NH, rx: 5 }, g);
      el('line', { class: 'sep', x1: DW, y1: 0, x2: DW, y2: NH }, g);
      const data = el('text', { class: 'v', x: DW / 2, y: NH / 2 + 5, 'text-anchor': 'middle' }, g);
      const ptr = el('text', { class: 'p', x: DW + PW / 2, y: NH / 2 + 4, 'text-anchor': 'middle' }, g);
      ptr.textContent = '•';
      if (compact) { data.setAttribute('font-size', '11'); }
      n = { g, data };
      nodes.set(id, n);
      return n;
    }

    function render(step, prevStep, opts) {
      opts = opts || {};
      // A panel names its list; the primary view uses the unnamed one.
      const L = opts && opts.source ? (step.lists || {})[opts.source] : step.list;
      if (!L) return;
      const act = step.action || {};

      const chain = L.nodes.filter((n) => n.state === 'chain');
      const loose = L.nodes.filter((n) => n.state !== 'chain');
      const place = new Map();   // id -> {x, y}

      chain.forEach((n, i) => place.set(n.id, { x: xOf(i), y: chainY }));
      loose.forEach((n, i) => {
        const at = n.at !== undefined && n.at >= 0 ? n.at : chain.length + i;
        place.set(n.id, { x: xOf(Math.min(at, Math.max(0, chain.length))), y: looseY });
      });

      // Drop nodes that no longer exist; keep the rest so they animate.
      for (const [id, n] of nodes) {
        if (!place.has(id)) { n.g.remove(); nodes.delete(id); }
      }

      for (const n of L.nodes) {
        const view = nodeFor(n.id);
        const p = place.get(n.id);
        view.g.setAttribute('transform', `translate(${p.x},${p.y})`);
        view.data.textContent = n.data;
        let cls = 'lnode ' + n.state;
        if (n.id === L.cursor) cls += ' cursor';
        view.g.setAttribute('class', cls);
      }

      // Edges are cheap to rebuild and cannot drift out of sync that way.
      gEdges.innerHTML = '';
      gLabels.innerHTML = '';

      const hot = (from, to) =>
        (act.kind === 'link' || act.kind === 'unlink') &&
        String(act.from) === String(from) && String(act.to) === String(to);

      for (const n of L.nodes) {
        const from = place.get(n.id);
        const target = n.next ? place.get(n.next) : null;
        if (!n.next) {
          if (n.state === 'chain') {
            const t = el('text', { x: from.x + NW + 14, y: from.y + NH / 2 + 4,
                                   'text-anchor': 'start', fill: 'var(--ink-dim)',
                                   'font-size': compact ? 9 : 11 }, gLabels);
            t.textContent = 'NULL';
            arrow(from.x + DW + PW / 2, from.y + NH / 2, from.x + NW + 10, from.y + NH / 2, '');
          }
          continue;
        }
        if (!target) continue;
        const cls = hot(n.id, n.next) ? 'hot' : (n.state === 'orphan' ? 'dead' : '');
        curve(from, target, cls);
      }

      // Backward pointers: only a doubly linked list has them, and drawing them
      // under the chain keeps them from fighting with the next arrows.
      if (L.kind === 'doubly') {
        for (const n of L.nodes) {
          if (!n.prev) continue;
          const from = place.get(n.id);
          const to = place.get(n.prev);
          if (!from || !to) continue;
          const y = from.y + NH + (compact ? 10 : 14);
          el('path', { class: 'lledge back',
                       d: `M ${from.x + 6} ${from.y + NH} L ${from.x + 6} ${y} L ${to.x + NW - 6} ${y} L ${to.x + NW - 6} ${to.y + NH}`,
                       'marker-end': 'url(#ll-arrow)' }, gEdges);
        }
      }

      if (L.head && place.has(L.head)) {
        const p = place.get(L.head);
        const t = el('text', { x: p.x + DW / 2, y: p.y - 8, 'text-anchor': 'middle',
                               fill: 'var(--accent)', 'font-size': compact ? 10 : 12,
                               'font-weight': '700' }, gLabels);
        t.textContent = 'head';
      }
      if (L.cursor && place.has(L.cursor)) {
        const p = place.get(L.cursor);
        const t = el('text', { x: p.x + DW / 2, y: p.y + NH + 16, 'text-anchor': 'middle',
                               fill: 'var(--c-selected)', 'font-size': compact ? 10 : 12,
                               'font-weight': '700' }, gLabels);
        t.textContent = L.cursorLabel || 'current';
      }
    }

    function arrow(x1, y1, x2, y2, cls) {
      const p = el('path', { class: 'lledge ' + cls, d: `M ${x1} ${y1} L ${x2} ${y2}` }, gEdges);
      p.setAttribute('marker-end', markerFor(cls));
      return p;
    }

    const markerFor = (cls) =>
      cls === 'hot' ? 'url(#ll-arrow-hot)' : cls === 'dead' ? 'url(#ll-arrow-dead)' : 'url(#ll-arrow)';

    function curve(from, to, cls) {
      const x1 = from.x + DW + PW / 2;
      const y1 = from.y + NH / 2;
      const x2 = to.x - 4;
      const y2 = to.y + NH / 2;
      let d;
      if (Math.abs(y1 - y2) < 1 && x2 > x1) {
        d = `M ${x1} ${y1} L ${x2} ${y2}`;                       // straight along the chain
      } else if (x2 < x1) {
        const dip = to.y + NH + (compact ? 22 : 30);              // wrap-around, drawn underneath
        d = `M ${x1} ${y1 + NH / 2 - 2} C ${x1} ${dip}, ${x2} ${dip}, ${to.x + DW / 2} ${to.y + NH + 2}`;
      } else {
        d = `M ${x1} ${y1} C ${x1 + 24} ${y1}, ${x2 - 24} ${y2}, ${x2} ${y2}`;
      }
      const p = el('path', { class: 'lledge ' + cls, d }, gEdges);
      p.setAttribute('marker-end', markerFor(cls));
      if (cls === 'hot') p.classList.add('pulse');
      return p;
    }

    function legend() {
      return [
        { color: 'var(--c-selected)', label: 'current node / rehooked pointer' },
        { color: 'var(--c-frontier)', label: 'new node, outside the chain' },
        { color: 'var(--c-error)',    label: 'orphaned node, before delete' },
      ];
    }

    return { id: 'linkedlist', mount, render, legend };
  };
})();
