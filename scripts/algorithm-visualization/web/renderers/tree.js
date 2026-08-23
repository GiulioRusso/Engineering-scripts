// Renderer: tree
//
// Binary and n-ary trees drawn by levels. Highlights the current node, the path
// taken down from the root, and the pruned subtree — which is where the O(h) of
// a search on a BST becomes visible instead of merely asserted.
(function () {
  const SVGNS = 'http://www.w3.org/2000/svg';
  const el = (tag, attrs, parent) => {
    const n = document.createElementNS(SVGNS, tag);
    for (const k in attrs) n.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(n);
    return n;
  };

  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.tree = function createTreeRenderer() {
    let svg, gEdges, gNodes, seqText;
    let R, XS, YS, PAD, TOP, compact;
    let width = 0, height = 0;

    const treeOf = (step, opts) =>
      opts && opts.source ? (step.trees || {})[opts.source] : step.tree;

    // x is a leaf slot, so siblings never collide and a missing binary child
    // still consumes its slot: that is what keeps left and right readable.
    function layout(T) {
      const byId = {};
      for (const n of T.nodes) byId[n.id] = n;
      const pos = {};
      let slot = 0;
      let maxDepth = 0;

      function walk(id, depth) {
        if (!id) { return slot++; }            // empty binary child: reserve the slot
        const n = byId[id];
        if (!n) return slot++;
        maxDepth = Math.max(maxDepth, depth);
        const kids = n.children || [];
        if (!kids.length) {
          const x = slot++;
          pos[id] = { x, depth };
          return x;
        }
        const xs = kids.map((k) => walk(k, depth + 1));
        const x = (Math.min(...xs) + Math.max(...xs)) / 2;
        pos[id] = { x, depth };
        return x;
      }
      walk(T.root, 0);
      return { pos, byId, slots: Math.max(1, slot), depth: maxDepth };
    }

    function mount(container, trace, opts) {
      opts = opts || {};
      compact = !!opts.compact;
      R = compact ? 13 : 19;
      XS = compact ? 30 : 46;
      YS = compact ? 42 : 62;
      PAD = R + 8;

      let slots = 1, depth = 0, hasSeq = false, hasLabels = false;
      for (const s of opts.steps || []) {
        const T = treeOf(s, opts);
        if (T && T.nodes && T.nodes.length) {
          const L = layout(T);
          slots = Math.max(slots, L.slots);
          depth = Math.max(depth, L.depth);
          if (T.nodes.some((n) => n.label)) hasLabels = true;
        }
        if (s.sequence) hasSeq = true;
      }

      // Node labels sit above the circle, so the root needs headroom or they clip.
      TOP = R + (hasLabels ? 20 : 8);
      width = PAD * 2 + slots * XS;
      height = TOP + PAD + depth * YS + (hasSeq ? (compact ? 20 : 26) : 0);

      container.innerHTML = '';
      svg = el('svg', { viewBox: `0 0 ${width} ${height}`, width, height }, container);
      gEdges = el('g', {}, svg);
      gNodes = el('g', {}, svg);
      seqText = el('text', { x: PAD, y: height - 6, 'text-anchor': 'start',
                             fill: 'var(--ink)', 'font-size': compact ? 11 : 13 }, svg);
      seqText.setAttribute('font-family', 'var(--mono)');
    }

    function render(step, prevStep, opts) {
      const T = treeOf(step, opts);
      gEdges.innerHTML = '';
      gNodes.innerHTML = '';
      seqText.textContent = step.sequence || '';
      if (!T || !T.nodes || !T.nodes.length) return;

      const L = layout(T);
      const px = (id) => PAD + L.pos[id].x * XS + XS / 2 - (XS / 2);
      const py = (id) => TOP + L.pos[id].depth * YS;

      for (const n of T.nodes) {
        if (!L.pos[n.id]) continue;
        for (const kid of n.children || []) {
          if (!kid || !L.pos[kid]) continue;
          const cls = (L.byId[kid].state === 'pruned' || n.state === 'pruned') ? 'pruned'
                    : (L.byId[kid].state === 'path' || L.byId[kid].state === 'current') ? 'path' : '';
          el('line', { class: 'tedge ' + cls,
                       x1: px(n.id), y1: py(n.id) + R, x2: px(kid), y2: py(kid) - R }, gEdges);
        }
      }

      for (const n of T.nodes) {
        if (!L.pos[n.id]) continue;
        const g = el('g', { class: 'tnode ' + (n.state || ''),
                            transform: `translate(${px(n.id)},${py(n.id)})` }, gNodes);
        el('circle', { r: R }, g);
        const t = el('text', { y: 5, 'text-anchor': 'middle' }, g);
        t.textContent = n.value;
        if (compact) t.setAttribute('font-size', '10');
        if (n.label) {
          const l = el('text', { class: 'tlabel', y: -R - 6, 'text-anchor': 'middle' }, g);
          l.textContent = n.label;
          if (compact) l.setAttribute('font-size', '8');
        }
      }
    }

    function legend() {
      return [
        { color: 'var(--c-selected)', label: 'nodo corrente' },
        { color: 'var(--c-frontier)', label: 'percorso di discesa' },
        { color: 'var(--c-visited)',  label: 'gia visitato' },
        { color: 'var(--c-sorted)',   label: 'trovato / inserito' },
        { color: 'var(--c-excluded)', label: 'sottoalbero potato' },
      ];
    }

    return { id: 'tree', mount, render, legend };
  };
})();
