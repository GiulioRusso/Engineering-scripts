// Renderer: queue
//
// The circular array drawn as an actual ring. `front`, `rear` and `count` are
// arrows around it. Drawing it as a ring is the whole point: the modulo
// wrap-around is the thing students get wrong, and on a ring it is not an
// arithmetic trick, it is just going round.
(function () {
  const SVGNS = 'http://www.w3.org/2000/svg';
  const el = (tag, attrs, parent) => {
    const n = document.createElementNS(SVGNS, tag);
    for (const k in attrs) n.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(n);
    return n;
  };

  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.queue = function createQueueRenderer() {
    let svg, cells = [], ptrs = {}, centerCount, centerFormula;
    let capacity = 0, R = 0, S = 0, cx = 0, cy = 0, compact = false;

    // Slot 0 at the top, then clockwise: the ring reads like a clock.
    const angle = (i) => (i / capacity) * 2 * Math.PI - Math.PI / 2;
    const px = (i, rad) => cx + Math.cos(angle(i)) * rad;
    const py = (i, rad) => cy + Math.sin(angle(i)) * rad;

    function pick(step, opts) {
      if (!step.arrays) return null;
      if (opts && opts.source) return step.arrays[opts.source] || null;
      const key = Object.keys(step.arrays)[0];
      return step.arrays[key] || null;
    }
    const num = (step, name) => (step.vars && step.vars[name] !== undefined ? +step.vars[name] : undefined);

    function mount(container, trace, opts) {
      opts = opts || {};
      compact = !!opts.compact;
      S = compact ? 30 : 44;

      capacity = 0;
      for (const s of opts.steps || []) {
        const a = pick(s, opts);
        if (a && a.length > capacity) capacity = a.length;
      }
      if (!capacity) capacity = 1;

      R = Math.max(compact ? 60 : 96, (capacity * (S + 14)) / (2 * Math.PI));
      const pad = S / 2 + (compact ? 30 : 42);
      const size = 2 * (R + pad);
      cx = cy = size / 2;

      container.innerHTML = '';
      svg = el('svg', { viewBox: `0 0 ${size} ${size}`, width: size, height: size }, container);
      el('circle', { cx, cy, r: R, fill: 'none', stroke: 'var(--line)', 'stroke-width': 1,
                     'stroke-dasharray': '3 4' }, svg);

      cells = [];
      for (let i = 0; i < capacity; i++) {
        const g = el('g', { class: 'cell',
                            transform: `translate(${px(i, R) - S / 2},${py(i, R) - S / 2})` }, svg);
        el('rect', { width: S, height: S, rx: 5 }, g);
        const v = el('text', { class: 'v', x: S / 2, y: S / 2 + 7, 'text-anchor': 'middle' }, g);
        if (compact) v.setAttribute('font-size', '11');
        // The slot number lives inside its own cell: putting it on the inner
        // radius crowded the centre once the ring got small.
        const idx = el('text', { class: 'i', x: 3, y: 10, 'text-anchor': 'start' }, g);
        idx.textContent = i;
        idx.setAttribute('font-size', compact ? 8 : 10);
        cells.push({ g, value: v });
      }

      ptrs = {};
      ['front', 'rear'].forEach((name) => {
        const g = el('g', { class: 'mark', opacity: 0 }, svg);
        const t = el('text', { x: 0, y: 0, 'text-anchor': 'middle' }, g);
        t.textContent = name;
        t.setAttribute('font-size', compact ? 9 : 11);
        if (name === 'rear') t.setAttribute('fill', 'var(--c-selected)');
        ptrs[name] = { g, text: t };
      });

      centerCount = el('text', { x: cx, y: cy - 2, 'text-anchor': 'middle',
                                 fill: 'var(--ink)', 'font-size': compact ? 13 : 17,
                                 'font-weight': '700' }, svg);
      centerFormula = el('text', { x: cx, y: cy + (compact ? 13 : 17), 'text-anchor': 'middle',
                                   fill: 'var(--ink-dim)', 'font-size': compact ? 9 : 11 }, svg);
    }

    function render(step, prevStep, opts) {
      const arr = pick(step, opts);
      const front = num(step, 'front');
      const rear = num(step, 'rear');
      const count = num(step, 'count');
      const act = step.action || {};

      const filled = new Set();
      if (count > 0 && front !== undefined) {
        for (let t = 0; t < count; t++) filled.add((front + t) % capacity);
      }

      cells.forEach((c, i) => {
        c.g.setAttribute('class', 'cell');
        const has = filled.has(i);
        c.value.textContent = has && arr ? arr[i] : '';
        if (!has) c.g.classList.add('empty');
      });

      if (act.kind === 'enqueue' && rear !== undefined && filled.has(rear)) cells[rear].g.classList.add('sel');
      if (act.kind === 'dequeue' && front !== undefined) {
        const gone = ((front - 1) % capacity + capacity) % capacity;
        cells[gone].g.setAttribute('class', 'cell cmp');
      }
      if (act.kind === 'error') cells.forEach((c) => c.g.classList.add('err'));

      const place = (name, idx) => {
        const p = ptrs[name];
        if (idx === undefined || idx < 0 || count === 0) { p.g.setAttribute('opacity', 0); return; }
        p.g.setAttribute('opacity', 1);
        const rad = R + S / 2 + (compact ? 16 : 24);
        p.text.setAttribute('x', px(idx, rad));
        p.text.setAttribute('y', py(idx, rad) + 4);
        p.text.textContent = name + '=' + idx;
      };
      place('front', front);
      place('rear', rear);

      centerCount.textContent = count === undefined ? '' : 'count = ' + count;
      centerFormula.textContent = 'capacity = ' + capacity;
    }

    function legend() {
      return [
        { color: 'var(--c-selected)', label: 'appena inserito (rear)' },
        { color: 'var(--c-compare)',  label: 'appena estratto (front)' },
        { color: 'var(--c-error)',    label: 'coda piena / vuota' },
      ];
    }

    return { id: 'queue', mount, render, legend };
  };
})();
