// Renderer: callstack
//
// Generic panel: the pile of frames with function name and arguments, the
// newest on top. Enabled for every recursive algorithm; the frames come from
// the tracer, which keeps them itself.
(function () {
  const SVGNS = 'http://www.w3.org/2000/svg';
  const el = (tag, attrs, parent) => {
    const n = document.createElementNS(SVGNS, tag);
    for (const k in attrs) n.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(n);
    return n;
  };

  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.callstack = function createCallStackRenderer() {
    let svg, gFrames, emptyLabel;
    let W, H, GAP, maxDepth = 1, baseY = 0, compact = false;

    const yOf = (i) => baseY - (i + 1) * (H + GAP);

    function mount(container, trace, opts) {
      opts = opts || {};
      compact = !!opts.compact;
      H = compact ? 20 : 26;
      GAP = 3;

      maxDepth = 1;
      let widest = 10;
      for (const s of opts.steps || []) {
        const cs = s.callStack || [];
        if (cs.length > maxDepth) maxDepth = cs.length;
        for (const f of cs) if (f.length > widest) widest = f.length;
      }
      W = Math.max(compact ? 140 : 190, widest * (compact ? 6.2 : 7.6) + 16);

      const totalH = maxDepth * (H + GAP) + 26;
      baseY = totalH - 10;

      container.innerHTML = '';
      svg = el('svg', { viewBox: `0 0 ${W + 16} ${totalH}`, width: W + 16, height: totalH }, container);
      gFrames = el('g', {}, svg);
      el('line', { x1: 6, y1: baseY + 2, x2: W + 10, y2: baseY + 2,
                   stroke: 'var(--line)', 'stroke-width': 2 }, svg);
      emptyLabel = el('text', { x: (W + 16) / 2, y: baseY - 8, 'text-anchor': 'middle',
                                fill: 'var(--ink-dim)', 'font-size': compact ? 10 : 12 }, svg);
    }

    // Frames are rebuilt each step rather than diffed: the pile is at most a
    // handful of rows, and a fresh build cannot drift out of sync with the step.
    function render(step) {
      const frames = step.callStack || [];
      gFrames.innerHTML = '';
      emptyLabel.textContent = frames.length ? '' : 'stack vuoto';

      frames.forEach((label, i) => {
        const top = i === frames.length - 1;
        const g = el('g', { class: 'cell' + (top ? ' sel' : ''), transform: `translate(8,${yOf(i)})` }, gFrames);
        el('rect', { width: W, height: H, rx: 4 }, g);
        const t = el('text', { class: 'v', x: 8, y: H / 2 + 4 }, g);
        t.textContent = label;
        t.setAttribute('font-size', compact ? 10 : 12);
        t.setAttribute('text-anchor', 'start');
      });
    }

    function legend() {
      return [{ color: 'var(--c-selected)', label: 'frame in esecuzione' }];
    }

    return { id: 'callstack', mount, render, legend };
  };
})();
