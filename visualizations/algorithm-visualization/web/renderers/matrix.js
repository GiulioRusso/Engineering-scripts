// Renderer: matrix
//
// An n×n grid with the current row, column and cell highlighted. Used for the
// adjacency matrix, for Floyd-Warshall (where the matrix *is* the content, not
// a side view) and for capacity/flow.
(function () {
  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.matrix = function createMatrixRenderer() {
    let root, compact;

    const matrixOf = (step, opts) =>
      opts && opts.source ? (step.matrices || {})[opts.source] : step.matrix;

    function mount(container, trace, opts) {
      compact = !!(opts && opts.compact);
      container.innerHTML = '';
      root = document.createElement('div');
      root.className = 'matrixbox' + (compact ? ' compact' : '');
      container.appendChild(root);
    }

    function render(step, prevStep, opts) {
      const M = matrixOf(step, opts);
      if (!M) { root.innerHTML = ''; return; }
      const prev = prevStep ? matrixOf(prevStep, opts) : null;
      const [cr, cc] = M.cell || [-1, -1];

      let html = '<table><thead><tr><th></th>';
      M.labels.forEach((l, c) => {
        html += '<th class="' + (c === M.hlCol ? 'hl' : '') + '">' + esc(l) + '</th>';
      });
      html += '</tr></thead><tbody>';
      M.rows.forEach((row, r) => {
        html += '<tr><th class="' + (r === M.hlRow ? 'hl' : '') + '">' + esc(M.labels[r]) + '</th>';
        row.forEach((v, c) => {
          const cls = [];
          if (r === M.hlRow || c === M.hlCol) cls.push('hl');
          if (r === cr && c === cc) cls.push('cell');
          if (prev && prev.rows && prev.rows[r] && String(prev.rows[r][c]) !== String(v)) cls.push('changed');
          html += '<td class="' + cls.join(' ') + '">' + esc(v) + '</td>';
        });
        html += '</tr>';
      });
      root.innerHTML = html + '</tbody></table>';
    }

    const esc = (s) => String(s).replace(/[&<>]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' }[c]));

    function legend() {
      return [
        { color: 'var(--c-frontier)', label: 'current row and column' },
        { color: 'var(--c-selected)', label: 'current cell' },
        { color: 'var(--c-compare)',  label: 'just-updated cell' },
      ];
    }

    return { id: 'matrix', mount, render, legend };
  };
})();
