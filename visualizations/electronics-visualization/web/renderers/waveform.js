// Renderer: waveform
//
// N stepped signals over discrete time, a current-time cursor, rising-edge
// ticks on clock lanes, and an optional shaded window (setup/hold). Every
// signal's full value history is already known — the renderer only draws the
// staircase and moves the cursor, it never computes a value.
(function () {
  const SVGNS = 'http://www.w3.org/2000/svg';
  const el = (tag, attrs, parent) => {
    const n = document.createElementNS(SVGNS, tag);
    for (const k in attrs) n.setAttribute(k, attrs[k]);
    if (parent) parent.appendChild(n);
    return n;
  };

  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.waveform = function createWaveformRenderer() {
    let svg, compact, cellW, laneH, labelW;

    const waveOf = (step, opts) =>
      opts && opts.source ? (step.waves || {})[opts.source] : step.wave;

    const risingEdges = (values) => {
      const out = [];
      for (let i = 1; i < values.length; i++) if (values[i - 1] === 0 && values[i] === 1) out.push(i);
      return out;
    };

    function mount(container, trace, opts) {
      opts = opts || {};
      compact = !!opts.compact;
      cellW = compact ? 22 : 32;
      laneH = compact ? 24 : 34;
      labelW = compact ? 34 : 50;
      container.innerHTML = '';
      svg = el('svg', {}, container);
    }

    // 1 -> top, 0 -> bottom, -1 (high impedance) -> a mid-height line.
    const levelOf = (v) => (v === 1 ? 0 : v === 0 ? 1 : 0.5);

    function staircasePath(values, x0, top, ampl) {
      const pts = [];
      for (let i = 0; i < values.length; i++) {
        const y = top + levelOf(values[i]) * ampl;
        pts.push([x0 + i * cellW, y], [x0 + (i + 1) * cellW, y]);
      }
      if (!pts.length) return '';
      return 'M' + pts[0].join(',') + ' L ' + pts.slice(1).map((p) => p.join(',')).join(' ');
    }

    function render(step, prevStep, opts) {
      const W = waveOf(step, opts);
      svg.innerHTML = '';
      if (!W || !W.signals || !W.signals.length) return;

      const n = Math.max(...W.signals.map((s) => s.values.length));
      const ampl = laneH * 0.62;
      const topPad = compact ? 8 : 12;
      const width = labelW + n * cellW + (compact ? 6 : 12);
      const height = topPad + W.signals.length * laneH + (compact ? 4 : 8);
      svg.setAttribute('viewBox', `0 0 ${width} ${height}`);
      svg.setAttribute('width', width);
      svg.setAttribute('height', height);

      // window (setup/hold shading) drawn first, under everything else
      if (W.window) {
        const wx = labelW + W.window.from * cellW;
        const ww = (W.window.to - W.window.from) * cellW;
        const g = el('g', { class: 'wave-window' }, svg);
        el('rect', { x: wx, y: topPad - 2, width: ww, height: height - topPad }, g);
        if (W.window.label) {
          const t = el('text', { x: wx + ww / 2, y: topPad - 4, 'text-anchor': 'middle' }, g);
          t.textContent = W.window.label;
        }
      }

      W.signals.forEach((sig, row) => {
        const top = topPad + row * laneH;
        const label = el('text', { class: 'wave-label', x: labelW - 8, y: top + ampl * 0.62,
          'text-anchor': 'end' }, svg);
        label.textContent = sig.name;

        const path = el('path', { class: 'wavesig ' + (sig.kind || 'data'),
          d: staircasePath(sig.values, labelW, top, ampl) }, svg);

        if (sig.kind === 'clk') {
          risingEdges(sig.values).forEach((i) => {
            el('line', { class: 'wave-marker', x1: labelW + i * cellW, y1: top - 2,
              x2: labelW + i * cellW, y2: top + ampl + 2 }, svg);
          });
        }
      });

      // markers (named instants: an edge, a violation) span the full height
      (W.markers || []).forEach((m) => {
        const x = labelW + m.t * cellW;
        const g = el('g', { class: 'wave-marker' }, svg);
        el('line', { x1: x, y1: topPad - 2, x2: x, y2: height - 2 }, g);
        if (m.label) {
          const t = el('text', { x, y: height - 2 }, g);
          t.textContent = m.label;
        }
      });

      const cx = labelW + (W.t + 0.5) * cellW;
      el('line', { class: 'wavecursor', x1: cx, y1: topPad - 4, x2: cx, y2: height - 2 }, svg);
    }

    function legend() {
      return [
        { color: 'var(--c-frontier)', label: 'clock' },
        { color: 'var(--ink)',        label: 'data' },
        { color: 'var(--c-sorted)',   label: 'output' },
        { color: 'var(--c-selected)', label: 'current instant' },
      ];
    }

    return { id: 'waveform', mount, render, legend };
  };
})();
