// Renderer: gantt
//
// The horizontal process timeline: coloured blocks, a time axis, a marker for
// the current instant, and underneath the table with arrival, burst, waiting
// and turnaround per process plus the averages. Those averages are the numbers
// that let the four scheduling algorithms be compared, so they are part of the
// view, not an afterthought.
(function () {
  const SVGNS = 'http://www.w3.org/2000/svg';
  const el = (tag, attrs, parent) => {
    const n = document.createElementNS(SVGNS, tag);
    for (const k in attrs) n.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(n);
    return n;
  };

  // Enough hues to keep four or five processes apart, reused from the shared
  // palette so a process keeps its colour across the four algorithms.
  const HUES = ['var(--c-frontier)', 'var(--c-sorted)', 'var(--c-compare)',
                'var(--c-selected)', 'var(--c-visited)'];

  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.gantt = function createGanttRenderer() {
    let root, svg, gBlocks, gAxis, nowLine, table, compact;
    let UNIT, H, TOP, PAD, span = 1, order = [];

    const ganttOf = (step, opts) =>
      opts && opts.source ? (step.gantts || {})[opts.source] : step.gantt;

    function mount(container, trace, opts) {
      opts = opts || {};
      compact = !!opts.compact;
      H = compact ? 22 : 32;
      TOP = compact ? 6 : 10;
      PAD = 8;

      span = 1;
      order = [];
      for (const s of opts.steps || []) {
        const G = ganttOf(s, opts);
        if (!G) continue;
        for (const sl of G.slices || []) span = Math.max(span, sl.end);
        for (const p of G.processes || []) {
          span = Math.max(span, p.arrival + p.burst);
          if (!order.includes(p.name)) order.push(p.name);
        }
      }
      UNIT = Math.max(compact ? 9 : 14, Math.min(compact ? 18 : 30, (compact ? 260 : 520) / span));

      const width = PAD * 2 + span * UNIT + 10;
      const height = TOP + H + (compact ? 20 : 26);

      container.innerHTML = '';
      root = document.createElement('div');
      container.appendChild(root);
      svg = el('svg', { viewBox: `0 0 ${width} ${height}`, width, height }, root);
      gBlocks = el('g', {}, svg);
      gAxis = el('g', {}, svg);
      nowLine = el('line', { class: 'gantt-now', y1: TOP - 4, y2: TOP + H + 4 }, svg);
      table = document.createElement('div');
      table.className = 'tablebox' + (compact ? ' compact' : '');
      root.appendChild(table);
    }

    const x = (t) => PAD + t * UNIT;
    const colorOf = (name) => HUES[Math.max(0, order.indexOf(name)) % HUES.length];

    function render(step, prevStep, opts) {
      const G = ganttOf(step, opts);
      gBlocks.innerHTML = '';
      gAxis.innerHTML = '';
      if (!G) { table.innerHTML = ''; return; }

      const ticks = new Set([0]);
      for (const sl of G.slices || []) {
        const w = Math.max(1, (sl.end - sl.start) * UNIT);
        const g = el('g', {}, gBlocks);
        el('rect', { class: 'gantt-block' + (sl.idle ? ' idle' : ''),
                     x: x(sl.start), y: TOP, width: w, height: H, rx: 3,
                     fill: sl.idle ? 'none' : colorOf(sl.proc) }, g);
        if (w > (compact ? 12 : 18)) {
          const t = el('text', { class: 'gantt-label', x: x(sl.start) + w / 2, y: TOP + H / 2 + 4,
                                 'text-anchor': 'middle' }, g);
          t.textContent = sl.idle ? '—' : sl.proc;
          if (compact) t.setAttribute('font-size', '9');
        }
        ticks.add(sl.start);
        ticks.add(sl.end);
      }

      for (const t of [...ticks].sort((a, b) => a - b)) {
        const label = el('text', { class: 'gantt-tick', x: x(t), y: TOP + H + (compact ? 12 : 16),
                                   'text-anchor': 'middle' }, gAxis);
        label.textContent = t;
        if (compact) label.setAttribute('font-size', '8');
      }

      if (G.now !== undefined) {
        nowLine.setAttribute('x1', x(G.now));
        nowLine.setAttribute('x2', x(G.now));
        nowLine.setAttribute('opacity', 1);
      } else {
        nowLine.setAttribute('opacity', 0);
      }

      // Metrics table. Empty cells mean "not finished yet", which is itself
      // information while stepping through.
      const m = G.metrics || {};
      let html = '<table><thead><tr><th>processo</th><th>arrivo</th><th>burst</th>';
      if (G.showPriority) html += '<th>priorità</th>';
      html += '<th>attesa</th><th>turnaround</th></tr></thead><tbody>';
      let sw = 0, st = 0, done = 0;
      for (const p of G.processes || []) {
        const row = m[p.name] || {};
        html += '<tr><th style="color:' + colorOf(p.name) + '">' + p.name + '</th>' +
                '<td>' + p.arrival + '</td><td>' + p.burst + '</td>';
        if (G.showPriority) html += '<td>' + (p.priority !== undefined ? p.priority : '') + '</td>';
        html += '<td>' + (row.wait !== undefined ? row.wait : '') + '</td>' +
                '<td>' + (row.turn !== undefined ? row.turn : '') + '</td></tr>';
        if (row.wait !== undefined) { sw += row.wait; st += row.turn; done++; }
      }
      if (done > 0) {
        const cols = G.showPriority ? 3 : 2;
        html += '<tr><th>media</th><td colspan="' + cols + '"></td>' +
                '<td class="avg">' + (sw / done).toFixed(2) + '</td>' +
                '<td class="avg">' + (st / done).toFixed(2) + '</td></tr>';
      }
      html += '</tbody></table>';
      if (G.ready) {
        html += '<div class="gantt-ready">coda dei pronti: <b>' +
                (G.ready.length ? G.ready.join(' → ') : '(vuota)') + '</b></div>';
      }
      table.innerHTML = html;
    }

    function legend() {
      return [
        { color: 'var(--c-frontier)', label: 'un colore per processo' },
        { color: 'var(--c-selected)', label: 'istante corrente' },
      ];
    }

    return { id: 'gantt', mount, render, legend };
  };
})();
