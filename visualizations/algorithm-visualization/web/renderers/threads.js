// Renderer: threads
//
// One column of code per process, side by side, with the current line
// highlighted ONLY in the process executing at this step. Underneath: the
// shared variables and the queue of blocked processes.
//
// When two processes are inside the critical section at the same time, both
// their lines light up in the error colour and a banner names the violation.
// That single frame explains atomicity better than any paragraph.
(function () {
  window.RENDERERS = window.RENDERERS || {};

  window.RENDERERS.threads = function createThreadsRenderer() {
    let root, cols = [], sharedBox, blockedBox, banner, code = [], compact;

    const esc = (s) => String(s).replace(/[&<>]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' }[c]));

    function mount(container, trace, opts) {
      opts = opts || {};
      compact = !!opts.compact;

      // The per-process source is constant, so it travels only on the first
      // step that carries it; grab it once here.
      code = [];
      for (const s of opts.steps || []) {
        const t = s.threads;
        if (t && t.procs && t.procs.some((p) => p.lines)) {
          code = t.procs.map((p) => ({ name: p.name, lines: p.lines || [] }));
          break;
        }
      }

      container.innerHTML = '';
      root = document.createElement('div');
      root.className = 'threads' + (compact ? ' compact' : '');
      container.appendChild(root);

      banner = document.createElement('div');
      banner.className = 'thr-banner';
      root.appendChild(banner);

      const grid = document.createElement('div');
      grid.className = 'thr-grid';
      root.appendChild(grid);

      cols = code.map((p) => {
        const col = document.createElement('div');
        col.className = 'thr-col';
        const head = document.createElement('div');
        head.className = 'thr-head';
        head.textContent = p.name;
        const body = document.createElement('div');
        body.className = 'thr-code';
        body.innerHTML = p.lines
          .map((l, i) => '<div class="thr-line" data-i="' + i + '">' + esc(l || ' ') + '</div>')
          .join('');
        col.appendChild(head);
        col.appendChild(body);
        grid.appendChild(col);
        return { col, head, body };
      });

      sharedBox = document.createElement('div');
      sharedBox.className = 'thr-shared';
      root.appendChild(sharedBox);
      blockedBox = document.createElement('div');
      blockedBox.className = 'thr-blocked';
      root.appendChild(blockedBox);
    }

    function render(step) {
      const T = step.threads;
      if (!T) return;

      const inCritical = (T.procs || []).filter((p) => p.critical);
      // A counting semaphore lets several processes hold the resource at once;
      // only exceeding the declared limit is a violation.
      const limit = T.maxInCritical === undefined ? 1 : T.maxInCritical;
      const violation = inCritical.length > limit;
      banner.className = 'thr-banner' + (violation ? ' violation' : (T.note ? ' info' : ''));
      banner.textContent = violation
        ? '⚠ MUTUAL EXCLUSION VIOLATED — ' + inCritical.map((p) => p.name).join(', ') +
          ' inside together, the allowed maximum is ' + limit
        : '';
      banner.style.display = violation ? '' : 'none';

      (T.procs || []).forEach((p, i) => {
        const c = cols[i];
        if (!c) return;
        c.col.className = 'thr-col ' + (p.state || '');
        c.head.textContent = p.name + (p.state ? ' · ' + p.state : '');
        [...c.body.children].forEach((row, k) => {
          row.className = 'thr-line';
          if (k !== p.pc) return;
          if (p.critical && violation) row.classList.add('violated');
          else if (p.critical) row.classList.add('critical');
          else if (p.state === 'running') row.classList.add('current');
          else if (p.state === 'blocked') row.classList.add('blocked');
          else row.classList.add('waiting');
        });
      });

      let html = '';
      for (const k in T.shared || {}) {
        html += '<span class="var"><b>' + esc(k) + '</b> = ' + esc(String(T.shared[k])) + '</span>';
      }
      sharedBox.innerHTML = '<span class="thr-label">shared variables</span>' + html;

      const q = T.blocked || [];
      blockedBox.innerHTML = '<span class="thr-label">blocked processes</span>' +
        (q.length ? q.map((n) => '<span class="var blocked">' + esc(n) + '</span>').join('')
                  : '<span class="thr-none">(none)</span>');
    }

    function legend() {
      return [
        { color: 'var(--c-frontier)', label: 'running line' },
        { color: 'var(--c-sorted)',   label: 'inside the critical section' },
        { color: 'var(--c-compare)',  label: 'busy-waiting' },
        { color: 'var(--c-excluded)', label: 'blocked' },
        { color: 'var(--c-error)',    label: 'mutual exclusion violated' },
      ];
    }

    return { id: 'threads', mount, render, legend };
  };
})();
