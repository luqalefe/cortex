// Simulador do Codex Delay: interface do pedal ligada ao motor por WebSocket.
// O motor é a fonte da verdade; aqui só enviamos mudanças e exibimos o estado.

const PORT = 7070;
const PARAMS = [
  { id: 'time',    name: 'Time',    min: 1,   max: 2000,  fmt: v => v.toFixed(0) + ' ms' },
  { id: 'repeats', name: 'Repeats', min: 0,   max: 0.95,  fmt: v => Math.round(v * 100) + '%' },
  { id: 'mix',     name: 'Mix',     min: 0,   max: 1,     fmt: v => Math.round(v * 100) + '%' },
  { id: 'tone',    name: 'Tone',    min: 800, max: 16000, fmt: v => (v / 1000).toFixed(1) + ' kHz' },
];

const el = (id) => document.getElementById(id);
const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
let ws = null, connected = false;
const knobs = {};

// --- WebSocket com reconexão ---
function connect() {
  ws = new WebSocket(`ws://localhost:${PORT}`);
  ws.onopen = () => { connected = true; el('status').textContent = 'conectado'; };
  ws.onclose = () => {
    connected = false; el('status').textContent = 'motor offline — tentando…';
    setTimeout(connect, 1000);
  };
  ws.onerror = () => ws.close();
  ws.onmessage = (ev) => {
    let m; try { m = JSON.parse(ev.data); } catch { return; }
    if (m.type === 'state') applyState(m);
  };
}

function send(obj) {
  if (connected && ws.readyState === 1) ws.send(JSON.stringify(obj));
}

// --- estado vindo do motor ---
let dragging = null, lastBypass = null;
function applyState(m) {
  for (const p of PARAMS) {
    if (dragging === p.id) continue;            // não brigar com o knob em uso
    if (m.params && p.id in m.params) setKnobVisual(p.id, m.params[p.id]);
  }
  if (m.bypass !== lastBypass) {
    lastBypass = m.bypass;
    el('fsw').classList.toggle('bypassed', m.bypass);
    el('fsw').textContent = m.bypass ? 'Bypassed' : 'Active';
  }
  // LED reage ao nível de saída
  const lv = clamp(m.level || 0, 0, 1);
  const g = Math.round(40 + lv * 215);
  el('led').style.background = m.bypass ? '#333' : `rgb(${Math.round(lv*60)},${g},${Math.round(80+lv*120)})`;
  el('led').style.boxShadow = lv > 0.02 && !m.bypass ? `0 0 ${4 + lv*10}px rgba(58,214,197,.8)` : 'inset 0 0 3px #000';
  // OLED
  const vals = PARAMS.map(p => `${p.name.padEnd(8)} ${p.fmt(m.params[p.id]).padStart(8)}`).join('\n');
  el('oled').textContent = (m.bypass ? '  [ BYPASS ]\n' : '  CODEX DELAY\n') + vals;
}

// --- knobs SVG ---
function makeKnob(p) {
  const wrap = document.createElement('div');
  wrap.className = 'knob';
  wrap.innerHTML = `
    <svg width="78" height="78" viewBox="0 0 78 78">
      <circle cx="39" cy="39" r="30" fill="#2a2f39" stroke="#0c0e11" stroke-width="2"/>
      <circle cx="39" cy="39" r="30" fill="none" stroke="#3ad6c5" stroke-width="3"
              stroke-dasharray="141 999" stroke-linecap="round"
              transform="rotate(130 39 39)" class="arc" opacity=".25"/>
      <line x1="39" y1="39" x2="39" y2="16" stroke="#3ad6c5" stroke-width="3"
            stroke-linecap="round" class="ind" transform="rotate(0 39 39)"/>
    </svg>
    <div class="name">${p.name}</div>
    <div class="val">—</div>`;
  const ind = wrap.querySelector('.ind');
  const valEl = wrap.querySelector('.val');
  knobs[p.id] = { p, ind, valEl, value: p.min };

  let startY = 0, startVal = 0;
  const norm = () => (knobs[p.id].value - p.min) / (p.max - p.min);
  const onMove = (e) => {
    const y = (e.touches ? e.touches[0].clientY : e.clientY);
    const dn = (startY - y) / 200;                     // 200px = curso completo
    const nv = clamp(startVal + dn, 0, 1);
    const value = p.min + nv * (p.max - p.min);
    setKnobVisual(p.id, value);
    send({ param: p.id, value });
  };
  const onUp = () => {
    dragging = null;
    window.removeEventListener('mousemove', onMove);
    window.removeEventListener('mouseup', onUp);
    window.removeEventListener('touchmove', onMove);
    window.removeEventListener('touchend', onUp);
  };
  const onDown = (e) => {
    e.preventDefault();
    dragging = p.id;
    startY = (e.touches ? e.touches[0].clientY : e.clientY);
    startVal = norm();
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    window.addEventListener('touchmove', onMove, { passive: false });
    window.addEventListener('touchend', onUp);
  };
  const svg = wrap.querySelector('svg');
  svg.addEventListener('mousedown', onDown);
  svg.addEventListener('touchstart', onDown, { passive: false });
  svg.addEventListener('dblclick', () => {           // duplo clique = padrão
    const def = p.id === 'time' ? 480 : p.id === 'repeats' ? 0.4 : p.id === 'mix' ? 0.35 : 6000;
    setKnobVisual(p.id, def); send({ param: p.id, value: def });
  });
  return wrap;
}

function setKnobVisual(id, value) {
  const k = knobs[id];
  k.value = clamp(value, k.p.min, k.p.max);
  const n = (k.value - k.p.min) / (k.p.max - k.p.min);
  k.ind.setAttribute('transform', `rotate(${-130 + n * 260} 39 39)`);
  k.valEl.textContent = k.p.fmt(k.value);
}

// --- montagem ---
const grid = el('knobs');
for (const p of PARAMS) grid.appendChild(makeKnob(p));
el('fsw').addEventListener('click', () => send({ toggle: true }));
connect();
