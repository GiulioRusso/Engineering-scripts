// Renderer: array
//
// Rectangular cells with value and index, arrow markers for the pointers
// (i, j, min, low, high, mid), coloured regions, and translation animations for
// swaps and shifts. A pointer slides to its new index instead of teleporting:
// watching the loop counter move is half of what there is to learn.
(function () {
  const SVGNS = 'http://www.w3.org/2000/svg';

  function el(tag, attrs, parent) {
    const n = document.createElementNS(SVGNS, tag);
    for (const k in attrs) n.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(n);
    return n;
  }

  function pickArray(step, opts) {
    if (!step.arrays) return null;
    if (opts && opts.source && step.arrays[opts.source]) return step.arrays[opts.source];
    const keys = Object.keys(step.arrays);
    return keys.length ? step.arrays[keys[0]] : null;
  }

  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.array = function createArrayRenderer() {
    let svg, gRegions, gCells, gMarks, gLifted;
    let cells = [], marks = {}, markRows = {}, lifted = null;
    let W, H, GAP, PAD, cellY, indexY, marksY;
    let n = 0;

    const x = (i) => PAD + i * (W + GAP);

    function mount(container, trace, opts) {
      opts = opts || {};
      const compact = !!opts.compact;
      W = compact ? 30 : 46;
      H = compact ? 30 : 44;
      GAP = compact ? 4 : 6;
      PAD = 8;

      const steps = opts.steps || [];
      n = 0;
      let usesLift = false;
      const markNames = [];
      for (const s of steps) {
        const a = pickArray(s, opts);
        if (a && a.length > n) n = a.length;
        if (s.lifted) usesLift = true;
        if (s.marks) for (const k in s.marks) if (!markNames.includes(k)) markNames.push(k);
      }
      if (!n) n = 1;

      const topBand = usesLift ? (compact ? 34 : 46) : 4;
      cellY = topBand;
      indexY = cellY + H + 13;
      marksY = indexY + 6;
      markRows = {};
      markNames.forEach((name, k) => { markRows[name] = k; });
      const rowH = compact ? 16 : 21;   // arrow + label, so rows cannot overlap
      const totalH = marksY + Math.max(1, markNames.length) * rowH + 6;
      const totalW = PAD * 2 + n * W + (n - 1) * GAP;

      container.innerHTML = '';
      svg = el('svg', { viewBox: `0 0 ${totalW} ${totalH}`, width: totalW, height: totalH }, container);
      gRegions = el('g', {}, svg);
      gCells = el('g', {}, svg);
      gMarks = el('g', {}, svg);
      gLifted = el('g', {}, svg);

      cells = [];
      for (let i = 0; i < n; i++) {
        const g = el('g', { class: 'cell', transform: `translate(${x(i)},${cellY})` }, gCells);
        el('rect', { width: W, height: H, rx: 5 }, g);
        const v = el('text', { class: 'v', x: W / 2, y: H / 2 + 5, 'text-anchor': 'middle' }, g);
        v.textContent = '';
        const idx = el('text', { class: 'i', x: W / 2, y: H + 13, 'text-anchor': 'middle' }, g);
        idx.textContent = i;
        if (compact) { v.setAttribute('font-size', '11'); idx.setAttribute('font-size', '9'); }
        cells.push({ g, rect: g.firstChild, value: v });
      }

      marks = {};
      for (const name of markNames) {
        const g = el('g', { class: 'mark', opacity: 0 }, gMarks);
        const y = marksY + markRows[name] * rowH;
        el('path', { d: `M ${W / 2 - 5} ${y + 8} L ${W / 2 + 5} ${y + 8} L ${W / 2} ${y} Z` }, g);
        const t = el('text', { x: W / 2, y: y + 17 }, g);
        t.textContent = name;
        if (compact) t.setAttribute('font-size', '9');
        marks[name] = g;
      }

      if (usesLift) {
        const g = el('g', { class: 'lifted', opacity: 0 }, gLifted);
        el('rect', { width: W, height: H - 8, rx: 5, y: 2 }, g);
        const val = el('text', { x: W / 2, y: H / 2 + 3 }, g);
        const lbl = el('text', { class: 'lbl', x: W / 2, y: H - 12, 'text-anchor': 'middle' }, g);
        lifted = { g, val, lbl };
      } else {
        lifted = null;
      }
    }

    function reset() {
      svg.classList.add('no-anim');
      for (let i = 0; i < n; i++) {
        cells[i].g.setAttribute('transform', `translate(${x(i)},${cellY})`);
        cells[i].g.setAttribute('class', 'cell');
      }
      svg.getBoundingClientRect();  // force reflow so the reset is not animated
      svg.classList.remove('no-anim');
    }

    function render(step, prevStep, opts) {
      opts = opts || {};
      const arr = pickArray(step, opts);
      reset();

      for (let i = 0; i < n; i++) {
        const c = cells[i];
        const has = arr && i < arr.length;
        c.value.textContent = has ? arr[i] : '';
        c.g.setAttribute('opacity', has ? 1 : 0);
      }

      // Regions paint the background and also restyle the cells they cover.
      gRegions.innerHTML = '';
      const regions = step.regions || [];
      for (const r of regions) {
        const from = Math.max(0, r.from), to = Math.min(n - 1, r.to);
        if (to < from) continue;
        el('rect', {
          class: 'region ' + r.kind,
          x: x(from) - 3, y: cellY - 4,
          width: (to - from + 1) * (W + GAP) - GAP + 6, height: H + 8, rx: 6
        }, gRegions);
        if (r.kind === 'excluded' || r.kind === 'empty' || r.kind === 'hole') {
          for (let i = from; i <= to; i++) cells[i].g.classList.add(r.kind);
        }
      }

      // Exactly one action per step: it is the thing being animated.
      const act = step.action || {};
      const dir = opts.dir || 1;
      const add = (i, cls) => { if (i >= 0 && i < n) cells[i].g.classList.add(cls); };

      if (lifted) {
        if (step.lifted) {
          lifted.g.setAttribute('opacity', 1);
          lifted.g.setAttribute('transform', `translate(${x(step.lifted.from)},0)`);
          lifted.val.textContent = step.lifted.value;
          lifted.lbl.textContent = step.lifted.label;
          lifted.g.classList.toggle('cmp', act.kind === 'compare' && act.b < 0);
        } else {
          lifted.g.setAttribute('opacity', 0);
        }
      }

      switch (act.kind) {
        case 'compare':
          add(act.a, 'cmp');
          if (act.b >= 0) add(act.b, 'cmp');
          break;
        case 'select': add(act.index, 'sel'); break;
        case 'found':  add(act.index, 'found'); break;
        case 'assign': add(act.index, 'sel'); break;
        case 'swap':   add(act.a, 'sel'); add(act.b, 'sel'); break;
        case 'shift':  add(act.from, 'sel'); break;
        case 'error':  for (let i = 0; i < n; i++) add(i, 'err'); break;
        default: break;
      }

      for (const name in marks) {
        const g = marks[name];
        const v = step.marks ? step.marks[name] : undefined;
        if (typeof v === 'number' && v >= 0 && v < n) {
          g.setAttribute('opacity', 1);
          g.setAttribute('transform', `translate(${x(v)},0)`);
        } else {
          g.setAttribute('opacity', 0);
        }
      }

      // Movement animations. Backwards stepping plays them in reverse: the cells
      // start where the forward animation left them and slide back.
      if (act.kind === 'swap') animateMove([[act.a, act.b], [act.b, act.a]], dir);
      else if (act.kind === 'shift') animateMove([[act.from, act.to]], dir);
      else if (act.kind === 'compare' || act.kind === 'discard') {
        const targets = act.kind === 'compare'
          ? [act.a, act.b].filter((i) => i >= 0 && i < n)
          : [];
        for (const i of targets) pulse(cells[i].g);
      }
    }

    function pulse(node) {
      node.classList.remove('pulse');
      void node.getBoundingClientRect();
      node.classList.add('pulse');
    }

    function animateMove(pairs, dir) {
      const place = (i, target) => cells[i].g.setAttribute('transform', `translate(${x(target)},${cellY})`);
      if (dir < 0) {
        svg.classList.add('no-anim');
        for (const [from, to] of pairs) place(from, to);
        svg.getBoundingClientRect();
        svg.classList.remove('no-anim');
        requestAnimationFrame(() => { for (const [from] of pairs) place(from, from); });
      } else {
        requestAnimationFrame(() => { for (const [from, to] of pairs) place(from, to); });
      }
    }

    function legend() {
      return [
        { color: 'var(--c-compare)',  label: 'in confronto' },
        { color: 'var(--c-selected)', label: 'selezionato / in movimento' },
        { color: 'var(--c-sorted)',   label: 'ordinato / trovato' },
        { color: 'var(--c-frontier)', label: 'finestra attiva' },
        { color: 'var(--c-excluded)', label: 'scartato' },
      ];
    }

    return { id: 'array', mount, render, legend };
  };
})();
