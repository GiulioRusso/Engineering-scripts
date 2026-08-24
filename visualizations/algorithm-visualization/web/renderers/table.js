// Renderer: table
//
// The evolving key/value tables: dist[], visited[], gScore/fScore, parent[],
// rank[]. One row per named table, columns in first-seen order. Cells that
// changed since the previous step flash, so the eye is drawn to what the step
// actually did.
(function () {
  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.table = function createTableRenderer() {
    let root, columns = [], names = [], compact;

    // `source` may name several tables, comma separated: a panel usually wants
    // parent[] and rank[] together, not one each.
    function tablesOf(step, opts) {
      const all = step.tables || {};
      if (!opts || !opts.source) return all;
      const want = opts.source.split(',').map((x) => x.trim()).filter(Boolean);
      const out = {};
      for (const name of want) if (all[name] !== undefined) out[name] = all[name];
      return out;
    }

    function mount(container, trace, opts) {
      opts = opts || {};
      compact = !!opts.compact;
      columns = [];
      names = [];
      for (const s of opts.steps || []) {
        const ts = tablesOf(s, opts);
        for (const name in ts) {
          if (!names.includes(name)) names.push(name);
          for (const k in ts[name]) if (!columns.includes(k)) columns.push(k);
        }
      }
      container.innerHTML = '';
      root = document.createElement('div');
      root.className = 'tablebox' + (compact ? ' compact' : '');
      container.appendChild(root);
    }

    function render(step, prevStep, opts) {
      const ts = tablesOf(step, opts);
      const prev = prevStep ? tablesOf(prevStep, opts) : {};

      let html = '<table><thead><tr><th></th>';
      for (const c of columns) html += '<th>' + esc(c) + '</th>';
      html += '</tr></thead><tbody>';
      for (const name of names) {
        if (!ts[name]) continue;
        html += '<tr><th>' + esc(name) + '</th>';
        for (const c of columns) {
          const v = ts[name][c];
          const before = prev[name] ? prev[name][c] : undefined;
          const changed = prevStep && before !== undefined && String(before) !== String(v);
          html += '<td class="' + (changed ? 'changed' : '') + '">' +
                  (v === undefined ? '' : esc(String(v))) + '</td>';
        }
        html += '</tr>';
      }
      root.innerHTML = html + '</tbody></table>';
    }

    const esc = (s) => String(s).replace(/[&<>]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' }[c]));

    function legend() {
      return [{ color: 'var(--c-compare)', label: 'value changed this step' }];
    }

    return { id: 'table', mount, render, legend };
  };
})();
