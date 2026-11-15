// Cortex — rig (editor imersivo de pedaleira).
// Monta a board a partir dos JSONs em /pedals, modos e presets. Áudio vem depois.

import { open3D, close3D, setExplode, setMode } from './pedal3d.js';

const PRESETS_KEY = 'cortex_presets_v1';
const LAST_KEY = 'cortex_last_preset';

const el = (id) => document.getElementById(id);
const clamp = (v, a, b) => Math.min(b, Math.max(a, v));

let defs = {}, order = [], state = {}, presets = {}, current = null, dirty = false;

// ---------- carregamento ----------
async function loadPedals() {
  const man = await fetch('/pedals/manifest.json').then(r => r.json());
  order = man.board.slice();
  const ids = Array.from(new Set([...man.board, ...man.library]));
  await Promise.all(ids.map(async id => { defs[id] = await fetch(`/pedals/${id}.json`).then(r => r.json()); }));
}
function defaultState() {
  const s = {};
  for (const id of order) {
    s[id] = { on: true, mode: 0, params: {} };
    for (const k of defs[id].knobs) s[id].params[k.id] = k.def;
  }
  return s;
}

// ---------- presets ----------
function loadPresets() {
  try { presets = JSON.parse(localStorage.getItem(PRESETS_KEY)) || {}; } catch { presets = {}; }
  if (!Object.keys(presets).length) seedFactory();
}
function savePresetsStore() { try { localStorage.setItem(PRESETS_KEY, JSON.stringify(presets)); } catch {} }
function seedFactory() {
  const base = () => JSON.parse(JSON.stringify(defaultState()));
  const clean = base();
  const lead = base();
  lead.ts9.params.drive = 7; lead.ts9.params.level = 6;
  lead.delay.mode = 2; lead.delay.params.mix = 28; lead.delay.params.time = 420;
  lead.reverb.mode = 1; lead.reverb.params.level = 25;
  const ambient = base();
  ambient.ts9.on = false;
  ambient.delay.mode = 6; ambient.delay.params.mix = 55; ambient.delay.params.repeats = 70;
  ambient.reverb.mode = 5; ambient.reverb.params.decay = 9; ambient.reverb.params.level = 55;
  presets = {
    'Limpo':   { name: 'Limpo',   pedals: clean },
    'Solo':    { name: 'Solo',    pedals: lead },
    'Ambient': { name: 'Ambient', pedals: ambient },
  };
  savePresetsStore();
}
function applyPreset(name) {
  const p = presets[name]; if (!p) return;
  state = JSON.parse(JSON.stringify(p.pedals));
  for (const id of order) {
    if (!state[id]) state[id] = { on: true, mode: 0, params: {} };
    for (const k of defs[id].knobs) if (state[id].params[k.id] === undefined) state[id].params[k.id] = k.def;
  }
  current = name; setDirty(false);
  try { localStorage.setItem(LAST_KEY, name); } catch {}
  renderBoard(); renderPresetSel();
}
function snapshot() { return { name: current || 'Sem nome', pedals: JSON.parse(JSON.stringify(state)) }; }

// ---------- knob metálico ----------
function fmt(k, v) {
  if (k.unit === 'ms') return v.toFixed(0) + ' ms';
  if (k.unit === '%')  return v.toFixed(0) + ' %';
  if (k.unit === 's')  return v.toFixed(1) + ' s';
  return (Number.isInteger(k.def) && v === Math.round(v)) ? v.toFixed(0) : v.toFixed(1);
}
function makeKnob(def, k, accent) {
  const uid = def.id + '_' + k.id;
  const R = 34;
  const wrap = document.createElement('div');
  wrap.className = 'knob';
  // anel de pegada (knurling)
  let grip = '';
  for (let i = 0; i < 40; i++) {
    const a = i * 9 * Math.PI / 180;
    const x1 = R + Math.sin(a) * 23, y1 = R - Math.cos(a) * 23;
    const x2 = R + Math.sin(a) * 26, y2 = R - Math.cos(a) * 26;
    grip += `<line x1="${x1.toFixed(1)}" y1="${y1.toFixed(1)}" x2="${x2.toFixed(1)}" y2="${y2.toFixed(1)}" stroke="rgba(0,0,0,.45)" stroke-width="1"/>`;
  }
  wrap.innerHTML = `
    <svg width="68" height="68" viewBox="0 0 68 68">
      <defs>
        <radialGradient id="cap${uid}" cx="38%" cy="30%" r="75%">
          <stop offset="0%" stop-color="#8a93a3"/><stop offset="38%" stop-color="#4b5362"/>
          <stop offset="78%" stop-color="#2a303b"/><stop offset="100%" stop-color="#171b22"/>
        </radialGradient>
        <linearGradient id="bez${uid}" x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stop-color="#454c59"/><stop offset="100%" stop-color="#0d1015"/>
        </linearGradient>
      </defs>
      <circle cx="${R}" cy="${R}" r="30" fill="none" stroke="rgba(255,255,255,.06)" stroke-width="5"/>
      <circle cx="${R}" cy="${R}" r="30" fill="none" stroke="${accent}" stroke-width="3.5"
              stroke-linecap="round" class="arc" pathLength="100"
              stroke-dasharray="0 100" transform="rotate(130 ${R} ${R})" style="filter:drop-shadow(0 0 4px ${accent})"/>
      <circle cx="${R}" cy="${R}" r="26" fill="url(#bez${uid})" stroke="#05070a" stroke-width="1"/>
      ${grip}
      <circle cx="${R}" cy="${R}" r="20" fill="url(#cap${uid})" stroke="rgba(0,0,0,.6)" stroke-width="1"/>
      <path d="M ${R-13} ${R-11} A 17 17 0 0 1 ${R+11} ${R-13}" fill="none" stroke="rgba(255,255,255,.28)" stroke-width="1.6" stroke-linecap="round"/>
      <line x1="${R}" y1="${R}" x2="${R}" y2="${R-17}" stroke="${accent}" stroke-width="2.8"
            stroke-linecap="round" class="ind" style="filter:drop-shadow(0 0 3px ${accent})"/>
      <circle cx="${R}" cy="${R}" r="2.2" fill="#0a0d12"/>
    </svg>
    <div class="kname">${k.label}</div>
    <div class="kval">—</div>`;
  const ind = wrap.querySelector('.ind'), arc = wrap.querySelector('.arc'), valEl = wrap.querySelector('.kval');
  const st = state[def.id];
  const draw = () => {
    const v = st.params[k.id], n = (v - k.min) / (k.max - k.min);
    ind.setAttribute('transform', `rotate(${-130 + n * 260} ${R} ${R})`);
    arc.setAttribute('stroke-dasharray', `${(n * 72.2).toFixed(1)} 100`);
    valEl.textContent = fmt(k, v);
  };
  draw();
  const norm = () => (st.params[k.id] - k.min) / (k.max - k.min);
  let startY = 0, startN = 0;
  const onMove = (e) => {
    const y = e.touches ? e.touches[0].clientY : e.clientY;
    const n = clamp(startN + (startY - y) / 180, 0, 1);
    let v = k.min + n * (k.max - k.min);
    v = (k.unit === 'ms' || k.unit === '%' || Number.isInteger(k.def)) ? Math.round(v) : Math.round(v * 10) / 10;
    st.params[k.id] = v; draw(); setDirty(true);
  };
  const onUp = () => {
    window.removeEventListener('mousemove', onMove); window.removeEventListener('mouseup', onUp);
    window.removeEventListener('touchmove', onMove); window.removeEventListener('touchend', onUp);
  };
  const onDown = (e) => {
    e.preventDefault();
    startY = e.touches ? e.touches[0].clientY : e.clientY; startN = norm();
    window.addEventListener('mousemove', onMove); window.addEventListener('mouseup', onUp);
    window.addEventListener('touchmove', onMove, { passive:false }); window.addEventListener('touchend', onUp);
  };
  const svg = wrap.querySelector('svg');
  svg.addEventListener('mousedown', onDown);
  svg.addEventListener('touchstart', onDown, { passive:false });
  svg.addEventListener('dblclick', () => { st.params[k.id] = k.def; draw(); setDirty(true); });
  return wrap;
}

// ---------- pedal ----------
function makePedal(id) {
  const d = defs[id], st = state[id], accent = d.color || '#35e0cf';
  const skin = d.skin || 'generic';
  const card = document.createElement('div');
  card.className = 'pedal skin-' + skin + (st.on ? '' : ' off');
  const jewel = skin === 'boss'
    ? `<div class="jewel ${st.on ? 'on' : ''}" style="color:${accent}"></div>` : '';
  card.innerHTML = `
    <div class="accentbar" style="color:${accent};background:${accent}"></div>
    ${jewel}
    <i class="screw tl"></i><i class="screw tr"></i><i class="screw bl"></i><i class="screw br"></i>
    <div class="phead"><div class="pname">${d.name}</div><div class="psub">${d.subtitle || ''}</div></div>`;

  const phead = card.querySelector('.phead');
  phead.style.cursor = 'pointer'; phead.title = 'Ver em 3D';
  phead.onclick = () => open3D(d, st);

  if (d.modes && d.modes.length) {
    const m = document.createElement('div'); m.className = 'mode';
    const name = () => d.modes[st.mode % d.modes.length];
    m.innerHTML = `<button class="mbtn" data-d="-1">◀</button><div class="mname"></div><button class="mbtn" data-d="1">▶</button>`;
    const mn = m.querySelector('.mname'); mn.textContent = name();
    m.querySelectorAll('.mbtn').forEach(b => b.onclick = () => {
      const n = d.modes.length; st.mode = (st.mode + parseInt(b.dataset.d) + n) % n;
      mn.textContent = name(); setDirty(true);
    });
    card.appendChild(m);
  }

  const kg = document.createElement('div'); kg.className = 'knobs';
  for (const k of d.knobs) kg.appendChild(makeKnob(d, k, accent));
  card.appendChild(kg);

  const setOn = (on) => {
    st.on = on; card.classList.toggle('off', !on); setDirty(true); updateChainInfo();
  };

  if (skin === 'boss') {
    // treadle grande; o LED é a joia no topo do painel
    const tr = document.createElement('button'); tr.className = 'treadle';
    tr.textContent = st.on ? 'On' : 'Bypass';
    const jw = card.querySelector('.jewel');
    tr.onclick = () => { setOn(!st.on); tr.textContent = st.on ? 'On' : 'Bypass';
      jw.classList.toggle('on', st.on); };
    card.appendChild(tr);
  } else {
    const foot = document.createElement('div'); foot.className = 'foot';
    foot.innerHTML = `<div class="led ${st.on ? 'on' : ''}" style="color:${accent};background:${st.on ? accent : '#241414'}"></div>
      <button class="fsw">${st.on ? 'On' : 'Off'}</button>`;
    const led = foot.querySelector('.led'), fsw = foot.querySelector('.fsw');
    fsw.onclick = () => { setOn(!st.on);
      led.classList.toggle('on', st.on); led.style.background = st.on ? accent : '#241414';
      fsw.textContent = st.on ? 'On' : 'Off'; };
    card.appendChild(foot);
  }
  return card;
}
function renderBoard() {
  const b = el('board'); b.innerHTML = '';
  order.forEach((id, i) => {
    b.appendChild(makePedal(id));
    if (i < order.length - 1) { const f = document.createElement('div'); f.className = 'flow'; b.appendChild(f); }
  });
  updateChainInfo();
}
function updateChainInfo() {
  const on = order.filter(id => state[id]?.on).length;
  el('chainInfo').textContent = `${on}/${order.length} pedais`;
}

// ---------- presets UI ----------
function renderPresetSel() {
  const sel = el('presetSel'); sel.innerHTML = '';
  const names = Object.keys(presets);
  if (current === null) { const o = document.createElement('option'); o.value=''; o.textContent='— não salvo —'; sel.appendChild(o); }
  for (const name of names) {
    const o = document.createElement('option'); o.value = name; o.textContent = name;
    if (name === current) o.selected = true; sel.appendChild(o);
  }
}
function setDirty(v) { dirty = v; el('dirtyDot').classList.toggle('on', v); }
function stepPreset(dir) {
  const names = Object.keys(presets); if (!names.length) return;
  let i = names.indexOf(current); i = (i + dir + names.length) % names.length;
  applyPreset(names[i]);
}
function toast(msg) { const t = el('toast'); t.textContent = msg; t.classList.add('show');
  clearTimeout(toast._t); toast._t = setTimeout(() => t.classList.remove('show'), 1600); }

// ---------- ações ----------
function doSave() { if (!current) return doSaveAs();
  presets[current] = snapshot(); savePresetsStore(); setDirty(false); toast(`"${current}" salvo`); }
function doSaveAs() { const name = prompt('Nome do preset:', current || 'Meu preset'); if (!name) return;
  current = name; presets[name] = snapshot(); savePresetsStore(); setDirty(false); renderPresetSel(); toast(`"${name}" salvo`); }
function doNew() { state = defaultState(); current = null; setDirty(false); renderBoard(); renderPresetSel(); toast('Board nova (padrão)'); }
function doDelete() { if (!current || !presets[current]) return;
  if (!confirm(`Excluir o preset "${current}"?`)) return;
  delete presets[current]; savePresetsStore();
  const first = Object.keys(presets)[0]; first ? applyPreset(first) : doNew(); toast('Preset excluído'); }
function doExport() { const data = JSON.stringify(snapshot(), null, 2);
  const blob = new Blob([data], { type: 'application/json' }); const a = document.createElement('a');
  a.href = URL.createObjectURL(blob); a.download = `${(current || 'preset')}.cortex.json`; a.click(); URL.revokeObjectURL(a.href); }
function doImport(file) { const r = new FileReader();
  r.onload = () => { try { const p = JSON.parse(r.result); if (!p.pedals) throw new Error('sem campo pedals');
      const name = p.name || file.name.replace(/\.json$/, ''); presets[name] = { name, pedals: p.pedals };
      savePresetsStore(); applyPreset(name); toast(`Importado "${name}"`);
    } catch (e) { toast('Falha ao importar: ' + e.message); } };
  r.readAsText(file); }

// medidores: respiração sutil (placeholder até haver áudio)
function idleMeters() {
  const t = performance.now() / 1000;
  el('meterIn').style.width  = (6 + 4 * (0.5 + 0.5 * Math.sin(t * 1.3))).toFixed(1) + '%';
  el('meterOut').style.width = (5 + 4 * (0.5 + 0.5 * Math.sin(t * 1.3 + 1))).toFixed(1) + '%';
  requestAnimationFrame(idleMeters);
}

// ---------- boot ----------
async function main() {
  await loadPedals(); loadPresets();
  const last = (() => { try { return localStorage.getItem(LAST_KEY); } catch { return null; } })();
  if (last && presets[last]) applyPreset(last);
  else { const first = Object.keys(presets)[0]; first ? applyPreset(first) : doNew(); }

  el('presetSel').onchange = (e) => { if (e.target.value) applyPreset(e.target.value); };
  el('prevPreset').onclick = () => stepPreset(-1);
  el('nextPreset').onclick = () => stepPreset(1);
  el('btnSave').onclick = doSave;

  const menu = el('menu');
  el('btnMenu').onclick = (e) => { e.stopPropagation(); menu.classList.toggle('open'); };
  document.addEventListener('click', () => menu.classList.remove('open'));
  menu.onclick = (e) => e.stopPropagation();
  el('mSaveAs').onclick = () => { menu.classList.remove('open'); doSaveAs(); };
  el('mNew').onclick = () => { menu.classList.remove('open'); doNew(); };
  el('mExport').onclick = () => { menu.classList.remove('open'); doExport(); };
  el('mImport').onclick = () => { menu.classList.remove('open'); el('fileImport').click(); };
  el('mDelete').onclick = () => { menu.classList.remove('open'); doDelete(); };
  el('fileImport').onchange = (e) => { if (e.target.files[0]) doImport(e.target.files[0]); e.target.value = ''; };

  document.querySelectorAll('.tab').forEach(t => t.onclick = () => {
    if (t.classList.contains('soon')) { toast(t.textContent + ' — em breve (vem com o áudio)'); return; }
  });

  // visualizador 3D
  el('view3dClose').onclick = close3D;
  el('explodeRange').oninput = (e) => setExplode(e.target.value / 100);
  document.querySelectorAll('.v3-mode').forEach(b => b.onclick = () => {
    document.querySelectorAll('.v3-mode').forEach(x => x.classList.remove('active'));
    b.classList.add('active'); setMode(b.dataset.mode);
    el('explodeRange').disabled = b.dataset.mode !== 'studio';
  });
  window.addEventListener('keydown', e => { if (e.key === 'Escape') close3D(); });

  idleMeters();
}
main();
