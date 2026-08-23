// Renderer: stack
//
// A vertical column that grows upwards, with the `top` pointer, the empty slots
// up to `capacity`, and the overflow / underflow states drawn explicitly:
// running out of room is part of what a fixed-size stack teaches.
(function () {
  const SVGNS = 'http://www.w3.org/2000/svg';
  const el = (tag, attrs, parent) => {
    const n = document.createElementNS(SVGNS, tag);
    for (const k in attrs) n.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(n);
    return n;
  };

  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.stack = function createStackRenderer() {
    let svg, slots = [], topArrow, topLabel, emptyLabel;
    let W, H, GAP, LEFT, capacity = 0, baseY = 0;

    // Slot 0 is the bottom of the column, so index i sits `i` rows up.
    const yOf = (i) => baseY - (i + 1) * (H + GAP);

    function pick(step, opts) {
      if (!step.arrays) return null;
      if (opts && opts.source) return step.arrays[opts.source] || null;
      const key = Object.keys(step.arrays)[0];
      return step.arrays[key] || null;
    }
    const topOf = (step) =>
      step.vars && step.vars.top !== undefined ? +step.vars.top
        : step.marks && step.marks.top !== undefined ? +step.marks.top : -1;

    function mount(container, trace, opts) {
      opts = opts || {};
      const compact = !!opts.compact;
      W = compact ? 52 : 76;
      H = compact ? 22 : 30;
      GAP = 3;
      LEFT = compact ? 44 : 60;

      capacity = 0;
      for (const s of opts.steps || []) {
        const a = pick(s, opts);
        if (a && a.length > capacity) capacity = a.length;
      }
      if (!capacity) capacity = 1;

      const totalH = capacity * (H + GAP) + 34;
      const totalW = LEFT + W + (compact ? 46 : 66);
      baseY = totalH - 20;

      container.innerHTML = '';
      svg = el('svg', { viewBox: `0 0 ${totalW} ${totalH}`, width: totalW, height: totalH }, container);

      slots = [];
      for (let i = 0; i < capacity; i++) {
        const g = el('g', { class: 'cell', transform: `translate(${LEFT},${yOf(i)})` }, svg);
        el('rect', { width: W, height: H, rx: 4 }, g);
        const v = el('text', { class: 'v', x: W / 2, y: H / 2 + 5, 'text-anchor': 'middle' }, g);
        const idx = el('text', { class: 'i', x: -8, y: H / 2 + 4, 'text-anchor': 'end' }, g);
        idx.textContent = i;
        if (compact) { v.setAttribute('font-size', '11'); idx.setAttribute('font-size', '9'); }
        slots.push({ g, value: v });
      }

      // Base line: the stack sits on something, and it is easier to read the
      // column as "growing" when it has a floor.
      el('line', { x1: LEFT - 6, y1: baseY + 2, x2: LEFT + W + 6, y2: baseY + 2,
                   stroke: 'var(--line)', 'stroke-width': 2 }, svg);

      const arrow = el('g', { class: 'mark', opacity: 0 }, svg);
      el('path', { d: `M ${LEFT + W + 22} ${H / 2 - 5} L ${LEFT + W + 22} ${H / 2 + 5} L ${LEFT + W + 10} ${H / 2} Z` }, arrow);
      topLabel = el('text', { x: LEFT + W + 26, y: H / 2 + 4, 'text-anchor': 'start' }, arrow);
      topLabel.textContent = 'top';
      if (compact) topLabel.setAttribute('font-size', '9');
      topArrow = arrow;

      emptyLabel = el('text', { x: LEFT + W / 2, y: baseY - 8, 'text-anchor': 'middle',
                                fill: 'var(--ink-dim)', 'font-size': compact ? 10 : 12 }, svg);
      emptyLabel.textContent = '';
    }

    function render(step, prevStep, opts) {
      const arr = pick(step, opts);
      const top = topOf(step);
      const act = step.action || {};

      svg.classList.add('no-anim');
      slots.forEach((s, i) => {
        s.g.setAttribute('transform', `translate(${LEFT},${yOf(i)})`);
        s.g.setAttribute('class', 'cell');
      });
      svg.getBoundingClientRect();
      svg.classList.remove('no-anim');

      slots.forEach((s, i) => {
        const filled = arr && i <= top;
        s.value.textContent = filled ? arr[i] : '';
        if (!filled) s.g.classList.add('empty');
        if (filled && i === top) s.g.classList.add('sel');
      });

      if (act.kind === 'push' && top >= 0) slots[top].g.classList.add('sel');
      if (act.kind === 'pop' && top + 1 < capacity) slots[top + 1].g.classList.add('cmp');
      if (act.kind === 'error') slots.forEach((s) => s.g.classList.add('err'));

      if (top >= 0) {
        topArrow.setAttribute('opacity', 1);
        topArrow.setAttribute('transform', `translate(0,${yOf(top)})`);
        topLabel.textContent = 'top = ' + top;
      } else {
        topArrow.setAttribute('opacity', 0);
      }
      emptyLabel.textContent = top < 0 ? 'stack vuoto (top = -1)' : '';
    }

    function legend() {
      return [
        { color: 'var(--c-selected)', label: 'cima dello stack' },
        { color: 'var(--c-compare)',  label: 'elemento estratto' },
        { color: 'var(--c-error)',    label: 'overflow / underflow' },
      ];
    }

    return { id: 'stack', mount, render, legend };
  };
})();
