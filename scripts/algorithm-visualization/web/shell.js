// shell.js - the player.
//
// Everything common to every algorithm lives here and lives here once: loading
// traces, the current step index, play/pause/step/scrub, the code panel with
// its line highlight, variables, counters, the explanation line, keyboard and
// theme. Views are supplied by the renderer registry in web/renderers/.
//
// The shell executes nothing. It replays a list of states that a real C++ run
// already computed, which is why stepping backwards is a decrement.
(function () {
  const $ = (sel) => document.querySelector(sel);

  const state = {
    algo: null,      // catalog entry
    trace: null,
    inputIndex: 0,
    step: 0,
    playing: false,
    timer: null,
    interval: 600,
    primary: null,
    panels: [],
  };

  const steps = () => (state.trace ? state.trace.inputs[state.inputIndex].steps : []);

  // --------------------------------------------------------------- catalog --

  function buildCatalog() {
    const nav = $('#catalog-list');
    nav.innerHTML = '';
    window.CATALOG.forEach((group) => {
      const h = document.createElement('div');
      h.className = 'chapter-title';
      h.textContent = group.chapter;
      nav.appendChild(h);
      group.items.forEach((item) => {
        const a = document.createElement('div');
        a.className = 'algo-link';
        a.dataset.id = item.id;
        a.innerHTML = '<span></span><span class="cx"></span>';
        a.firstChild.textContent = item.name;
        a.lastChild.textContent = item.time;
        a.title = `tempo ${item.time} · spazio ${item.space}`;
        a.onclick = () => selectAlgorithm(item.id);
        nav.appendChild(a);
      });
    });
  }

  function findEntry(id) {
    for (const g of window.CATALOG) {
      const hit = g.items.find((i) => i.id === id);
      if (hit) return hit;
    }
    return null;
  }

  // ----------------------------------------------------------- trace load --

  function loadTrace(id, done, fail) {
    window.TRACES = window.TRACES || {};
    if (window.TRACES[id]) return done(window.TRACES[id]);
    const s = document.createElement('script');
    s.src = '../traces/' + id + '.js';
    s.onload = () => (window.TRACES[id] ? done(window.TRACES[id]) : fail('traccia vuota'));
    s.onerror = () => fail('impossibile caricare ../traces/' + id + '.js');
    document.head.appendChild(s);
  }

  function selectAlgorithm(id) {
    pause();
    const entry = findEntry(id);
    if (!entry) return;
    document.querySelectorAll('.algo-link').forEach((n) =>
      n.classList.toggle('active', n.dataset.id === id));
    $('#note').textContent = 'Carico ' + entry.name + '…';
    loadTrace(id, (trace) => {
      state.algo = entry;
      state.trace = trace;
      state.inputIndex = 0;
      location.hash = id;
      buildHeader();
      buildCode();
      buildInputSelect();
      // ?input=&step= deep-link: lets a lesson link straight to one moment.
      const q = new URLSearchParams(location.search);
      const wantInput = Math.min(Math.max(0, +q.get('input') || 0), trace.inputs.length - 1);
      setInput(wantInput, +q.get('step') || 0);
    }, (msg) => { $('#note').textContent = 'Errore: ' + msg; });
  }

  function buildHeader() {
    const t = state.trace;
    $('#algo-name').textContent = t.name;
    $('#badge-chapter').textContent = t.chapter;
    $('#badge-time').textContent = 'tempo ' + t.complexity.time;
    $('#badge-space').textContent = 'spazio ' + t.complexity.space;
  }

  function buildInputSelect() {
    const sel = $('#input-select');
    sel.innerHTML = '';
    state.trace.inputs.forEach((inp, i) => {
      const o = document.createElement('option');
      o.value = i;
      o.textContent = inp.label;
      sel.appendChild(o);
    });
    sel.value = 0;
  }

  // ------------------------------------------------------------ code panel --

  function buildCode() {
    const pre = $('#code');
    pre.innerHTML = '';
    state.trace.displaySource.forEach((line, i) => {
      const row = document.createElement('div');
      row.className = 'row';
      row.dataset.i = i;
      const gut = document.createElement('span');
      gut.className = 'gut';
      gut.textContent = i + 1;
      const txt = document.createElement('span');
      txt.textContent = line || ' ';
      row.appendChild(gut);
      row.appendChild(txt);
      pre.appendChild(row);
    });
  }

  // A step's `line` is the real source line. Instrumentation lines are hidden,
  // and lineMap already resolves them to the nearest visible line above.
  function displayIndexFor(line) {
    const map = state.trace.lineMap;
    if (map[line] !== undefined) return map[line];
    for (let l = line; l > 0; l--) if (map[l] !== undefined) return map[l];
    return -1;
  }

  function highlightLine(line) {
    const pre = $('#code');
    const prev = pre.querySelector('.row.cur');
    if (prev) prev.classList.remove('cur');
    const idx = displayIndexFor(line);
    if (idx < 0) return;
    const row = pre.children[idx];
    if (!row) return;
    row.classList.add('cur');
    // Scroll only when the line is actually leaving the viewport: scrolling on
    // every step makes the panel jitter and is unreadable.
    const rb = row.getBoundingClientRect();
    const pb = pre.getBoundingClientRect();
    if (rb.top < pb.top + 24 || rb.bottom > pb.bottom - 24) {
      pre.scrollTop += (rb.top - pb.top) - pb.height / 2 + rb.height / 2;
    }
  }

  // ---------------------------------------------------------------- views --

  function makeCard(title) {
    const card = document.createElement('div');
    card.className = 'card';
    const h = document.createElement('h3');
    h.textContent = title;
    const body = document.createElement('div');
    body.className = 'viewbox';
    card.appendChild(h);
    card.appendChild(body);
    return { card, body };
  }

  function instantiate(viewId, container, opts) {
    const factory = window.RENDERERS && window.RENDERERS[viewId];
    if (!factory) {
      container.innerHTML = '<em style="color:var(--ink-dim)">Renderer «' + viewId +
        '» non ancora disponibile.</em>';
      return null;
    }
    const r = factory();
    r.mount(container, state.trace, opts);
    return r;
  }

  function mountViews() {
    const t = state.trace;
    const stepList = steps();

    const layout = t.inputs[state.inputIndex].layout || null;
    const seek = (i) => { pause(); state.step = i; renderStep(1); };

    const primaryBox = $('#primary');
    primaryBox.innerHTML = '';
    state.primary = instantiate(t.view, primaryBox, { steps: stepList, compact: false, layout, seek });

    const panelsBox = $('#panels');
    panelsBox.innerHTML = '';
    state.panels = [];
    (t.panels || []).forEach((p) => {
      const { card, body } = makeCard(p.title || p.view);
      panelsBox.appendChild(card);
      const r = instantiate(p.view, body,
        { steps: stepList, compact: true, source: p.source, marks: p.marks, layout, seek });
      if (r) state.panels.push({ renderer: r, source: p.source });
    });

    const leg = $('#legend');
    leg.innerHTML = '';
    if (state.primary && state.primary.legend) {
      state.primary.legend().forEach((e) => {
        const s = document.createElement('span');
        s.innerHTML = '<i></i>';
        s.firstChild.style.background = e.color;
        s.appendChild(document.createTextNode(e.label));
        leg.appendChild(s);
      });
    }
  }

  function setInput(i, atStep) {
    pause();
    state.inputIndex = i;
    $('#input-select').value = i;
    state.step = atStep || 0;
    mountViews();
    $('#scrub').max = Math.max(0, steps().length - 1);
    renderStep(1);
  }

  // ---------------------------------------------------------------- render --

  function renderStep(dir) {
    const list = steps();
    if (!list.length) return;
    state.step = Math.max(0, Math.min(state.step, list.length - 1));
    const step = list[state.step];
    const prev = state.step > 0 ? list[state.step - 1] : null;

    highlightLine(step.line);
    $('#fn-header').textContent = step.function ? step.function + '()' : '';

    const varsBox = $('#vars');
    varsBox.innerHTML = '';
    const prevVars = (prev && prev.vars) || {};
    for (const k in step.vars || {}) {
      const chip = document.createElement('span');
      chip.className = 'var' + (prevVars[k] !== step.vars[k] ? ' changed' : '');
      chip.innerHTML = '<b></b>';
      chip.firstChild.textContent = k;
      chip.appendChild(document.createTextNode(' = ' + step.vars[k]));
      varsBox.appendChild(chip);
    }
    for (const k in step.counters || {}) {
      const c = document.createElement('span');
      c.className = 'counter';
      c.textContent = k + ': ' + step.counters[k];
      varsBox.appendChild(c);
    }

    $('#note').textContent = step.note || '';
    $('#step-count').textContent = (state.step + 1) + ' / ' + list.length;
    $('#scrub').value = state.step;

    const opts = { dir: dir || 1 };
    if (state.primary) state.primary.render(step, prev, opts);
    state.panels.forEach((p) => p.renderer.render(step, prev, { dir: opts.dir, source: p.source }));

    $('#btn-back').disabled = state.step === 0;
    $('#btn-fwd').disabled = state.step === list.length - 1;
  }

  // -------------------------------------------------------------- controls --

  function go(delta) {
    const list = steps();
    const next = state.step + delta;
    if (next < 0 || next >= list.length) { pause(); return; }
    state.step = next;
    renderStep(delta > 0 ? 1 : -1);
  }

  function play() {
    if (state.playing || state.step >= steps().length - 1) return;
    state.playing = true;
    $('#btn-play').textContent = '⏸';
    tick();
  }

  function tick() {
    state.timer = setTimeout(() => {
      if (!state.playing) return;
      if (state.step >= steps().length - 1) { pause(); return; }
      go(1);
      tick();
    }, state.interval);
  }

  function pause() {
    state.playing = false;
    clearTimeout(state.timer);
    const b = $('#btn-play');
    if (b) b.textContent = '▶';
  }

  function setSpeed(v) {
    state.interval = 1500 - v * 14;              // slider 0..100 -> 1500..100 ms
    const anim = Math.max(90, Math.min(400, state.interval * 0.55));
    document.documentElement.style.setProperty('--anim-ms', anim + 'ms');
  }

  function wire() {
    $('#btn-reset').onclick = () => { pause(); state.step = 0; renderStep(1); };
    $('#btn-back').onclick = () => { pause(); go(-1); };
    $('#btn-fwd').onclick = () => { pause(); go(1); };
    $('#btn-play').onclick = () => (state.playing ? pause() : play());
    $('#scrub').oninput = (e) => {
      pause();
      const v = +e.target.value;
      const dir = v >= state.step ? 1 : -1;
      state.step = v;
      renderStep(dir);
    };
    $('#speed').oninput = (e) => setSpeed(+e.target.value);
    $('#input-select').onchange = (e) => setInput(+e.target.value);
    $('#btn-theme').onclick = toggleTheme;

    document.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'SELECT' || e.target.tagName === 'INPUT') return;
      const list = steps();
      switch (e.key) {
        case 'ArrowRight': e.preventDefault(); pause(); go(1); break;
        case 'ArrowLeft':  e.preventDefault(); pause(); go(-1); break;
        case ' ':          e.preventDefault(); state.playing ? pause() : play(); break;
        case 'r': case 'R': pause(); state.step = 0; renderStep(1); break;
        case 'Home':       e.preventDefault(); pause(); state.step = 0; renderStep(-1); break;
        case 'End':        e.preventDefault(); pause(); state.step = list.length - 1; renderStep(1); break;
        default:
          if (e.key >= '1' && e.key <= '9') {
            const g = window.CATALOG[+e.key - 1];
            if (g && g.items.length) selectAlgorithm(g.items[0].id);
          }
      }
    });
  }

  // ----------------------------------------------------------------- theme --

  function toggleTheme() {
    const now = document.documentElement.getAttribute('data-theme') === 'dark' ? 'light' : 'dark';
    document.documentElement.setAttribute('data-theme', now);
    try { localStorage.setItem('algoviz-theme', now); } catch (_) {}
  }

  function initTheme() {
    let saved = null;
    try { saved = localStorage.getItem('algoviz-theme'); } catch (_) {}
    const dark = saved ? saved === 'dark'
      : window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches;
    document.documentElement.setAttribute('data-theme', dark ? 'dark' : 'light');
  }

  // ------------------------------------------------------------------ boot --

  function boot() {
    initTheme();
    buildCatalog();
    wire();
    setSpeed(+$('#speed').value);
    const wanted = location.hash.slice(1);
    const first = window.CATALOG[0].items[0].id;
    selectAlgorithm(findEntry(wanted) ? wanted : first);
  }

  document.addEventListener('DOMContentLoaded', boot);
})();
