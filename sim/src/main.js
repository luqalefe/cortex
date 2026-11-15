// Simulador do Codex Delay — interface do pedal ligada ao motor por WebSocket.
// O motor é a fonte da verdade: enviamos mudanças e desenhamos o estado recebido.

const PORT = 7070;
const PARAMS = [
  { id: 'time',    name: 'Time',    min: 1,   max: 2000,  def: 480,  fmt: v => v.toFixed(0) + ' ms' },
  { id: 'repeats', name: 'Repeats', min: 0,   max: 0.95,  def: 0.4,  fmt: v => Math.round(v * 100) + '%' },
  { id: 'mix',     name: 'Mix',     min: 0,   max: 1,     def: 0.35, fmt: v => Math.round(v * 100) + '%' },
  { id: 'tone',    name: 'Tone',    min: 800, max: 16000, def: 6000, fmt: v => (v / 1000).toFixed(1) + ' kHz' },
];
const ACCENT = '#35e0cf', AMBER = '#ffb13b';

const el = (id) => document.getElementById(id);
const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
const lerp = (a, b, t) => a + (b - a) * t;

// estado mais recente vindo do motor
const state = {
  params: Object.fromEntries(PARAMS.map(p => [p.id, p.def])),
  bypass: false, level: 0, sr: 44100, connected: false,
};
let dragging = null;
const knobs = {};

// ---------- WebSocket ----------
let ws = null;
function connect() {
  ws = new WebSocket(`ws://localhost:${PORT}`);
  ws.onopen = () => { state.connected = true; setConn(true); };
  ws.onclose = () => { state.connected = false; setConn(false); setTimeout(connect, 1000); };
  ws.onerror = () => ws.close();
  ws.onmessage = (ev) => {
    let m; try { m = JSON.parse(ev.data); } catch { return; }
    if (m.type !== 'state') return;
    state.bypass = m.bypass; state.level = m.level || 0; state.sr = m.sr;
    if (m.params) for (const p of PARAMS) {
      if (dragging === p.id) continue;
      state.params[p.id] = m.params[p.id];
      setKnobVisual(p.id, m.params[p.id]);
    }
    el('fsw').textContent = m.bypass ? 'Bypassed' : 'Active';
    el('led').classList.toggle('on', !m.bypass);
    el('ro-left').textContent = m.bypass ? '[ BYPASS ]' : 'CODEX DELAY';
    el('ro-right').textContent = `${(state.params.time).toFixed(0)}ms · ${Math.round(state.params.repeats*100)}%`;
  };
}
function setConn(on) {
  el('dot').classList.toggle('on', on);
  el('status').textContent = on ? 'conectado' : 'motor offline — tentando…';
}
function send(obj) { if (state.connected && ws.readyState === 1) ws.send(JSON.stringify(obj)); }

// ---------- knobs ----------
function makeKnob(p) {
  const wrap = document.createElement('div');
  wrap.className = 'knob';
  // anel de escala com traços
  let ticks = '';
  for (let i = 0; i <= 10; i++) {
    const a = (-130 + i * 26) * Math.PI / 180;
    const x1 = 44 + Math.sin(a) * 36, y1 = 44 - Math.cos(a) * 36;
    const x2 = 44 + Math.sin(a) * 40, y2 = 44 - Math.cos(a) * 40;
    ticks += `<line x1="${x1.toFixed(1)}" y1="${y1.toFixed(1)}" x2="${x2.toFixed(1)}" y2="${y2.toFixed(1)}" stroke="#4a515e" stroke-width="1.5"/>`;
  }
  wrap.innerHTML = `
    <svg width="88" height="88" viewBox="0 0 88 88">
      ${ticks}
      <circle cx="44" cy="44" r="31" fill="none" stroke="#1a1e26" stroke-width="5"/>
      <circle cx="44" cy="44" r="31" fill="none" stroke="${ACCENT}" stroke-width="4"
              stroke-linecap="round" class="arc" pathLength="100"
              stroke-dasharray="0 100" transform="rotate(130 44 44)"/>
      <circle cx="44" cy="44" r="26" fill="url(#kg)" stroke="#05070a" stroke-width="1.5"/>
      <defs><radialGradient id="kg" cx="50%" cy="35%" r="70%">
        <stop offset="0%" stop-color="#3a4150"/><stop offset="70%" stop-color="#232832"/>
        <stop offset="100%" stop-color="#171b22"/></radialGradient></defs>
      <line x1="44" y1="44" x2="44" y2="23" stroke="${ACCENT}" stroke-width="3.5"
            stroke-linecap="round" class="ind"/>
      <circle cx="44" cy="44" r="2.5" fill="#05070a"/>
    </svg>
    <div class="name">${p.name}</div>
    <div class="val">—</div>`;
  knobs[p.id] = {
    p, ind: wrap.querySelector('.ind'), arc: wrap.querySelector('.arc'),
    valEl: wrap.querySelector('.val'), value: p.def,
  };

  const norm = () => (knobs[p.id].value - p.min) / (p.max - p.min);
  let startY = 0, startVal = 0;
  const onMove = (e) => {
    const y = e.touches ? e.touches[0].clientY : e.clientY;
    const nv = clamp(startVal + (startY - y) / 220, 0, 1);
    const value = p.min + nv * (p.max - p.min);
    setKnobVisual(p.id, value); send({ param: p.id, value });
  };
  const onUp = () => {
    dragging = null;
    window.removeEventListener('mousemove', onMove);
    window.removeEventListener('mouseup', onUp);
    window.removeEventListener('touchmove', onMove);
    window.removeEventListener('touchend', onUp);
  };
  const onDown = (e) => {
    e.preventDefault(); dragging = p.id;
    startY = e.touches ? e.touches[0].clientY : e.clientY; startVal = norm();
    window.addEventListener('mousemove', onMove); window.addEventListener('mouseup', onUp);
    window.addEventListener('touchmove', onMove, { passive:false }); window.addEventListener('touchend', onUp);
  };
  const svg = wrap.querySelector('svg');
  svg.addEventListener('mousedown', onDown);
  svg.addEventListener('touchstart', onDown, { passive:false });
  svg.addEventListener('dblclick', () => { setKnobVisual(p.id, p.def); send({ param:p.id, value:p.def }); });
  return wrap;
}
function setKnobVisual(id, value) {
  const k = knobs[id];
  k.value = clamp(value, k.p.min, k.p.max);
  const n = (k.value - k.p.min) / (k.p.max - k.p.min);
  k.ind.setAttribute('transform', `rotate(${-130 + n * 260} 44 44)`);
  k.arc.setAttribute('stroke-dasharray', `${(n * 72.2).toFixed(1)} 100`); // 260° de 360° = 72%
  k.valEl.textContent = k.p.fmt(k.value);
}

// ---------- visualização (canvas) ----------
const cv = el('viz'), ctx = cv.getContext('2d');
let W = 0, H = 0, dpr = 1;
const hist = new Float32Array(240); let hi = 0;
function resize() {
  dpr = Math.min(window.devicePixelRatio || 1, 2);
  W = cv.clientWidth; H = cv.clientHeight;
  cv.width = W * dpr; cv.height = H * dpr;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}
window.addEventListener('resize', resize);

function draw() {
  requestAnimationFrame(draw);
  if (!W) resize();
  const { time, repeats, mix } = state.params;
  const lv = state.bypass ? 0 : state.level;
  hist[hi] = lv; hi = (hi + 1) % hist.length;

  ctx.clearRect(0, 0, W, H);
  // fundo + grade
  ctx.fillStyle = '#04100e'; ctx.fillRect(0, 0, W, H);
  ctx.strokeStyle = 'rgba(53,224,207,.07)'; ctx.lineWidth = 1;
  for (let x = 0; x < W; x += 28) { ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, H); ctx.stroke(); }
  const midY = H * 0.46;

  // --- ecos do delay: barras nas posições das repetições ---
  const windowMs = Math.min(2600, Math.max(600, time * 6));
  const baseX = 30, usableW = W - 48;
  const dim = state.bypass ? 0.25 : 1;
  // "seco" (hit de entrada) na origem
  const inGlow = clamp(lv * 3, 0, 1);
  for (let n = 0; n <= 12; n++) {
    const amp = n === 0 ? 1 : mix * Math.pow(repeats, n - 1);
    if (amp < 0.015) break;
    const x = baseX + (time * n / windowMs) * usableW;
    if (x > W - 10) break;
    const h = amp * (H * 0.34);
    const pulse = 0.55 + 0.45 * inGlow;       // reage ao sinal de entrada
    const a = amp * pulse * dim;
    const col = n === 0 ? AMBER : ACCENT;
    // haste
    ctx.strokeStyle = n === 0 ? `rgba(255,177,59,${a})` : `rgba(53,224,207,${a})`;
    ctx.lineWidth = n === 0 ? 3 : 2.5;
    ctx.beginPath(); ctx.moveTo(x, midY - h); ctx.lineTo(x, midY + h); ctx.stroke();
    // brilho na ponta
    ctx.fillStyle = ctx.strokeStyle;
    ctx.beginPath(); ctx.arc(x, midY - h, n === 0 ? 3.5 : 2.8, 0, 7); ctx.fill();
    ctx.beginPath(); ctx.arc(x, midY + h, n === 0 ? 3.5 : 2.8, 0, 7); ctx.fill();
  }
  // linha central
  ctx.strokeStyle = 'rgba(53,224,207,.18)';
  ctx.beginPath(); ctx.moveTo(0, midY); ctx.lineTo(W, midY); ctx.stroke();

  // --- scope de entrada ao vivo (faixa inferior) ---
  const scopeY = H - 16, scopeH = 22;
  ctx.beginPath();
  for (let i = 0; i < hist.length; i++) {
    const idx = (hi + i) % hist.length;
    const x = (i / (hist.length - 1)) * W;
    const y = scopeY - clamp(hist[idx], 0, 1) * scopeH;
    i ? ctx.lineTo(x, y) : ctx.moveTo(x, y);
  }
  ctx.strokeStyle = `rgba(53,224,207,${state.bypass ? .2 : .8})`; ctx.lineWidth = 1.5; ctx.stroke();
  // medidor numérico de nível
  ctx.fillStyle = 'rgba(124,134,151,.8)'; ctx.font = '10px ui-monospace, monospace';
  const db = lv > 0.0001 ? (20 * Math.log10(lv)).toFixed(0) + ' dB' : '-inf';
  ctx.fillText(db, W - 52, H - 6);
}

// ---------- montagem ----------
const grid = el('knobs');
for (const p of PARAMS) { grid.appendChild(makeKnob(p)); setKnobVisual(p.id, p.def); }
el('fsw').addEventListener('click', () => send({ toggle: true }));
resize(); draw(); connect();
