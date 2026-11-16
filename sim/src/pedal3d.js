// Cortex — visualizador 3D de pedal (Three.js).
// Pedal ORIGINAL do Cortex, na linguagem "foto de produto" (estúdio claro, vista explodida).
// Procedural: nada de modelo baixado nem marca de terceiros.

import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { RoomEnvironment } from 'three/examples/jsm/environments/RoomEnvironment.js';
import { RoundedBoxGeometry } from 'three/examples/jsm/geometries/RoundedBoxGeometry.js';

const GREEN = 0x2f7d4f, PLATE = 0xcfd4da, BLACK = 0x15181d, CHROME = 0xd7dce2;

let renderer, scene, camera, controls, raf, host;
let knobsGroup, shellGroup, pcbGroup, baseGroup, pedalGroup;
let boardGroup, ground;
let knobMeshes = [];
let explode = 0, explodeTarget = 0;
let mode = 'studio';   // 'studio' | 'board'

function mat(color, metalness, roughness) {
  return new THREE.MeshStandardMaterial({ color, metalness, roughness });
}

function buildPedal(def, state) {
  pedalGroup = new THREE.Group();
  knobsGroup = new THREE.Group();
  shellGroup = new THREE.Group();
  pcbGroup = new THREE.Group();
  baseGroup = new THREE.Group();
  knobMeshes = [];

  // --- base metálica ---
  const base = new THREE.Mesh(new RoundedBoxGeometry(16, 0.7, 12, 4, 0.3), mat(PLATE, 0.9, 0.35));
  base.position.y = 0.35; base.castShadow = base.receiveShadow = true;
  baseGroup.add(base);

  // --- PCB (placa + alguns componentes) ---
  const pcb = new THREE.Mesh(new RoundedBoxGeometry(14.5, 0.3, 10.5, 3, 0.15), mat(0x1f6b45, 0.2, 0.6));
  pcb.position.y = 1.1; pcb.castShadow = pcb.receiveShadow = true;
  pcbGroup.add(pcb);
  const compColors = [0x111111, 0x222831, 0xb08a2e, 0x2a2f38];
  for (let i = 0; i < 26; i++) {
    const s = 0.3 + Math.random() * 0.7;
    const c = new THREE.Mesh(new THREE.BoxGeometry(s, 0.25 + Math.random() * 0.4, s),
      mat(compColors[i % compColors.length], 0.3, 0.6));
    c.position.set((Math.random() - 0.5) * 12, 1.4, (Math.random() - 0.5) * 8.5);
    c.castShadow = true; pcbGroup.add(c);
  }
  // LEDs vermelhos na placa
  for (let i = 0; i < 4; i++) {
    const led = new THREE.Mesh(new THREE.CylinderGeometry(0.12, 0.12, 0.3, 12),
      new THREE.MeshStandardMaterial({ color: 0xff3020, emissive: 0xff2010, emissiveIntensity: 1.4, roughness: 0.4 }));
    led.position.set(-5 + i * 1.2, 1.45, 3.6); pcbGroup.add(led);
  }

  // --- casco verde (chassi) ---
  const shell = new THREE.Mesh(new RoundedBoxGeometry(15.4, 4.6, 11.4, 5, 0.7), mat(GREEN, 0.25, 0.5));
  shell.position.y = 3.6; shell.castShadow = shell.receiveShadow = true;
  shellGroup.add(shell);
  // painel de controle claro no topo (recuado ao fundo)
  const plate = new THREE.Mesh(new RoundedBoxGeometry(9.5, 0.3, 9, 3, 0.2), mat(PLATE, 0.85, 0.35));
  plate.position.set(2.2, 5.95, 0); plate.castShadow = true; shellGroup.add(plate);
  // aletas (heatsink) de um lado
  for (let i = 0; i < 6; i++) {
    const fin = new THREE.Mesh(new THREE.BoxGeometry(0.25, 3.4, 9.6), mat(GREEN, 0.3, 0.5));
    fin.position.set(-7.0 + i * 0.45, 3.6, 0); fin.castShadow = true; shellGroup.add(fin);
  }

  // --- knobs (tampo de plástico preto com ponteiro) ---
  const nKnobs = Math.min(6, (def.modes && def.modes.length ? 1 : 0) + def.knobs.length);
  const positions = [];
  // seletor de modo (maior) no canto, se houver
  let idx = 0;
  if (def.modes && def.modes.length) positions.push({ x: -1.2, z: -2.6, big: true });
  // demais knobs em arco/linha
  const row = def.knobs.length;
  for (let i = 0; i < row; i++) positions.push({ x: -0.5 + i * 1.5 - (row - 1) * 0.75 + 2.5, z: 1.2, big: false });

  positions.slice(0, 6).forEach((p, i) => {
    const r = p.big ? 1.0 : 0.72;
    const k = new THREE.Group();
    const body = new THREE.Mesh(new THREE.CylinderGeometry(r, r * 0.92, 1.4, 48), mat(BLACK, 0.35, 0.5));
    body.castShadow = true;
    const skirt = new THREE.Mesh(new THREE.CylinderGeometry(r * 1.15, r * 1.15, 0.25, 48), mat(BLACK, 0.3, 0.6));
    skirt.position.y = -0.6;
    const ptr = new THREE.Mesh(new THREE.BoxGeometry(0.14, 0.2, r * 0.9), mat(0xf0f3f7, 0.2, 0.4));
    ptr.position.set(0, 0.75, -r * 0.42);
    k.add(skirt, body, ptr);
    k.position.set(p.x, 6.5, p.z);
    knobMeshes.push({ group: k, pointer: ptr });
    knobsGroup.add(k);
  });

  // --- footswitches cromados ---
  for (let i = 0; i < 4; i++) {
    const fs = new THREE.Mesh(new THREE.CylinderGeometry(0.55, 0.62, 0.9, 32), mat(CHROME, 0.95, 0.2));
    fs.position.set(-5.2 + i * 3.1, 6.1, 4.0); fs.castShadow = true; shellGroup.add(fs);
  }

  pedalGroup.add(baseGroup, pcbGroup, shellGroup, knobsGroup);
  pedalGroup.position.y = -3;   // centraliza verticalmente na cena
  scene.add(pedalGroup);
  updateKnobs(def, state);
}

function updateKnobs(def, state) {
  const list = [];
  if (def.modes && def.modes.length) list.push((state.mode % def.modes.length) / Math.max(1, def.modes.length - 1));
  for (const k of def.knobs) {
    const v = state.params[k.id];
    list.push((v - k.min) / (k.max - k.min));
  }
  knobMeshes.forEach((km, i) => {
    const n = list[i] ?? 0.5;
    km.group.rotation.y = (-130 + n * 260) * Math.PI / 180;
  });
}

function applyExplode(e) {
  knobsGroup.position.y = e * 7.5;
  shellGroup.position.y = e * 3.2;
  pcbGroup.position.y = e * 0.2;
  baseGroup.position.y = -e * 2.2;
  // leve espalhar dos knobs
  knobMeshes.forEach((km, i) => { km.group.position.y = 6.5 + e * (1 + i * 0.25); });
}

function buildBoard() {
  boardGroup = new THREE.Group();
  // pedaleira de alumínio: travessas inclinadas
  const slatMat = mat(0x3a3f47, 0.85, 0.4);
  for (let i = 0; i < 9; i++) {
    const slat = new THREE.Mesh(new RoundedBoxGeometry(26, 0.5, 1.6, 2, 0.2), slatMat);
    slat.position.set(0, -3.1 + i * 0.0, -8 + i * 2.0);
    slat.position.y = -3.1 + (i * 0.14);          // leve inclinação
    slat.receiveShadow = true; slat.castShadow = true;
    boardGroup.add(slat);
  }
  // moldura lateral
  const railMat = mat(0x22262d, 0.8, 0.45);
  [-13.2, 13.2].forEach(x => {
    const rail = new THREE.Mesh(new RoundedBoxGeometry(1.2, 2.2, 20, 2, 0.3), railMat);
    rail.position.set(x, -2.4, -0.2); rail.castShadow = rail.receiveShadow = true;
    boardGroup.add(rail);
  });
  boardGroup.visible = false;
  scene.add(boardGroup);
}

export function setMode(m) {
  mode = m;
  if (m === 'board') {
    scene.background = new THREE.Color(0x0e1116);
    boardGroup.visible = true;
    ground.material.color.set(0x0a0c10);
    ground.position.y = -3.4;
    explodeTarget = 0;
    pedalGroup.position.y = -2.0;   // assenta na pedaleira
    camera.position.set(14, 11, 18);
  } else {
    scene.background = new THREE.Color(0xdfe2e6);
    boardGroup.visible = false;
    ground.material.color.set(0xdfe2e6);
    ground.position.y = -3.2;
    pedalGroup.position.y = -3;
    camera.position.set(16, 13, 20);
  }
}

function animate() {
  raf = requestAnimationFrame(animate);
  explode += (explodeTarget - explode) * 0.12;
  applyExplode(explode);
  controls.update();
  renderer.render(scene, camera);
}

export function open3D(def, state) {
  host = document.getElementById('view3d');
  const canvasHost = document.getElementById('view3d-canvas');
  document.getElementById('view3d-title').textContent = `${def.name} — 3D`;
  host.classList.add('open');

  const w = canvasHost.clientWidth, h = canvasHost.clientHeight;
  renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false });
  renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
  renderer.setSize(w, h);
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.05;
  canvasHost.appendChild(renderer.domElement);

  scene = new THREE.Scene();
  scene.background = new THREE.Color(0xdfe2e6);        // estúdio claro
  const pmrem = new THREE.PMREMGenerator(renderer);
  scene.environment = pmrem.fromScene(new RoomEnvironment(), 0.04).texture;

  camera = new THREE.PerspectiveCamera(38, w / h, 0.1, 200);
  camera.position.set(16, 13, 20);

  controls = new OrbitControls(camera, renderer.domElement);
  controls.target.set(0, 1.5, 0);
  controls.enableDamping = true; controls.dampingFactor = 0.08;
  controls.minDistance = 12; controls.maxDistance = 48;
  controls.maxPolarAngle = Math.PI * 0.52;

  // chão "cove" de estúdio que recebe sombra
  ground = new THREE.Mesh(new THREE.PlaneGeometry(200, 200),
    new THREE.MeshStandardMaterial({ color: 0xdfe2e6, roughness: 1, metalness: 0 }));
  ground.rotation.x = -Math.PI / 2; ground.position.y = -3.2; ground.receiveShadow = true;
  scene.add(ground);

  // luzes de estúdio
  scene.add(new THREE.HemisphereLight(0xffffff, 0xbfc4cc, 0.6));
  const key = new THREE.DirectionalLight(0xffffff, 2.3);
  key.position.set(10, 20, 12); key.castShadow = true;
  key.shadow.mapSize.set(2048, 2048);
  key.shadow.camera.near = 1; key.shadow.camera.far = 80;
  key.shadow.camera.left = -20; key.shadow.camera.right = 20;
  key.shadow.camera.top = 20; key.shadow.camera.bottom = -20;
  key.shadow.bias = -0.0003; key.shadow.radius = 6;
  scene.add(key);
  const fill = new THREE.DirectionalLight(0xffffff, 0.8); fill.position.set(-14, 8, 6); scene.add(fill);
  const rim = new THREE.DirectionalLight(0xffffff, 1.1); rim.position.set(-6, 10, -16); scene.add(rim);

  buildPedal(def, state);
  buildBoard();
  setMode('studio');
  animate();

  const onResize = () => {
    const w2 = canvasHost.clientWidth, h2 = canvasHost.clientHeight;
    camera.aspect = w2 / h2; camera.updateProjectionMatrix(); renderer.setSize(w2, h2);
  };
  window.addEventListener('resize', onResize);
  host._cleanup = () => window.removeEventListener('resize', onResize);
}

export function setExplode(v) { explodeTarget = v; }

export function close3D() {
  const host = document.getElementById('view3d');
  host.classList.remove('open');
  cancelAnimationFrame(raf);
  if (host._cleanup) host._cleanup();
  if (renderer) { renderer.dispose(); renderer.domElement.remove(); }
  scene = null; renderer = null;
}
