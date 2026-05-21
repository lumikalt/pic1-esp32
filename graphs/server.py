from flask import Flask, Response, render_template_string
import requests, json, threading, time

app = Flask(__name__)

ESP32_IP = "192.168.1.90"   # change to ESP32 IP (shown on Serial)
ESP32_URL = f"http://{ESP32_IP}/data"

latest = {}
lock = threading.Lock()

def poller():
    """Background thread: polls ESP32 every 150ms."""
    global latest
    while True:
        try:
            r = requests.get(ESP32_URL, timeout=1)
            with lock:
                latest = r.json()
        except Exception as e:
            print(f"[poll error] {e}")
        time.sleep(0.15)

threading.Thread(target=poller, daemon=True).start()

@app.route('/data')
def data():
    with lock:
        return Response(json.dumps(latest), mimetype='application/json')

HTML = r"""
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP32 3D Dashboard</title>
  <script src="https://cdn.jsdelivr.net/npm/three@0.160.0/build/three.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { background: #0d0d1a; color: #eee; font-family: Arial, sans-serif; }
    h1 { text-align:center; padding: 14px; color: #4af; font-size: 1.4em; letter-spacing:2px; }
    #status { text-align:center; font-size:.85em; color:#0f0; margin-bottom:8px; }

    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      grid-template-rows: auto auto auto;
      gap: 12px;
      padding: 0 12px 12px;
      max-width: 1300px;
      margin: auto;
    }

    /* 3D viewer spans both columns */
    #viewer-card {
      grid-column: 1 / -1;
      background: #12122a;
      border-radius: 12px;
      padding: 10px;
      border: 1px solid #223;
    }
    #viewer-card h2 { color:#4af; margin-bottom:6px; font-size:1em; text-align:center; }
    #three-container {
      width: 100%;
      height: 320px;
      border-radius: 8px;
      overflow: hidden;
      position: relative;
    }
    #three-container canvas { width:100%!important; height:100%!important; }

    /* Angle readouts */
    #angles {
      display:flex; justify-content:center; gap:30px;
      margin-top:8px; font-size:1.1em; font-family:monospace;
    }
    #angles span { color:#aaa; }
    #angles b { color:#fff; }

    .card {
      background: #12122a;
      border-radius: 12px;
      padding: 12px;
      border: 1px solid #223;
    }
    .card h2 { color:#4af; font-size:.95em; margin-bottom:8px; text-align:center; }

    /* Motor status spans both columns */
    #motor-card {
      grid-column: 1 / -1;
      background: #12122a;
      border-radius: 12px;
      padding: 10px 20px;
      border: 1px solid #223;
      text-align: center;
    }
    #motor-card h2 { color:#4af; font-size:.95em; margin-bottom:4px; }
    #motor-val { font-size:1.2em; font-family:monospace; color:#ff0; }
  </style>
</head>
<body>
<h1>⚡ ESP32 Live Dashboard</h1>
<div id="status">⏳ Connecting...</div>

<div class="grid">

  <!-- 3D Viewer -->
  <div id="viewer-card">
    <h2>3D Orientation (Roll / Pitch / Yaw)</h2>
    <div id="three-container"></div>
    <div id="angles">
      <span>Roll: <b id="v-roll">0.00</b>°</span>
      <span>Pitch: <b id="v-pitch">0.00</b>°</span>
      <span>Yaw: <b id="v-yaw">0.00</b>°</span>
    </div>
  </div>

  <!-- Magnetometer chart -->
  <div class="card">
    <h2>Magnetometer (µT)</h2>
    <canvas id="magChart" height="180"></canvas>
  </div>

  <!-- Accelerometer chart -->
  <div class="card">
    <h2>Accelerometer (g)</h2>
    <canvas id="accChart" height="180"></canvas>
  </div>

  <!-- Motor status -->
  <div id="motor-card">
    <h2>Coil / Motor State</h2>
    <div id="motor-val">—</div>
  </div>

</div>

<script>
// ─── THREE.JS 3D SETUP ───────────────────────────────────────────────
const container = document.getElementById('three-container');
const renderer  = new THREE.WebGLRenderer({ antialias: true, alpha: true });
renderer.setSize(container.clientWidth, container.clientHeight);
renderer.setPixelRatio(window.devicePixelRatio);
container.appendChild(renderer.domElement);

const scene  = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(45, container.clientWidth / container.clientHeight, 0.1, 100);
camera.position.set(0, 1.5, 4);
camera.lookAt(0, 0, 0);

// Lighting
scene.add(new THREE.AmbientLight(0xffffff, 0.6));
const dirLight = new THREE.DirectionalLight(0xffffff, 1.2);
dirLight.position.set(5, 8, 5);
scene.add(dirLight);

// Build a PCB-like box representing the ESP32
const group = new THREE.Group();

// Main board
const boardGeo  = new THREE.BoxGeometry(3, 0.15, 2);
const boardMat  = new THREE.MeshPhongMaterial({ color: 0x1a6b1a });
group.add(new THREE.Mesh(boardGeo, boardMat));

// Chip on top
const chipGeo = new THREE.BoxGeometry(0.8, 0.12, 0.8);
const chipMat = new THREE.MeshPhongMaterial({ color: 0x222222 });
const chip = new THREE.Mesh(chipGeo, chipMat);
chip.position.set(0, 0.135, 0);
group.add(chip);

// Antenna stub
const antGeo = new THREE.BoxGeometry(0.15, 0.4, 0.05);
const antMat = new THREE.MeshPhongMaterial({ color: 0x888888 });
const ant = new THREE.Mesh(antGeo, antMat);
ant.position.set(1.35, 0.27, 0);
group.add(ant);

// Axes helper (small)
group.add(new THREE.AxesHelper(1.8));

scene.add(group);

// Grid floor
const grid = new THREE.GridHelper(10, 20, 0x334, 0x223);
grid.position.y = -1.5;
scene.add(grid);

// Target Euler (smoothed)
let targetRoll = 0, targetPitch = 0, targetYaw = 0;

function animate() {
  requestAnimationFrame(animate);
  // Smooth interpolation
  group.rotation.x += (targetPitch * Math.PI / 180 - group.rotation.x) * 0.15;
  group.rotation.z += (targetRoll  * Math.PI / 180 - group.rotation.z) * 0.15;
  group.rotation.y += (targetYaw   * Math.PI / 180 - group.rotation.y) * 0.15;
  renderer.render(scene, camera);
}
animate();

window.addEventListener('resize', () => {
  const w = container.clientWidth, h = container.clientHeight;
  renderer.setSize(w, h);
  camera.aspect = w / h;
  camera.updateProjectionMatrix();
});

// ─── CHART.JS SETUP ─────────────────────────────────────────────────
const MAXPTS = 80;
const chartDefaults = {
  animation: false,
  responsive: true,
  maintainAspectRatio: true,
  plugins: { legend: { labels: { color:'#aaa', boxWidth:12 } } },
  scales: {
    x: { display: false },
    y: { ticks:{ color:'#888' }, grid:{ color:'#223' } }
  }
};

function makeChart(id, label, colors) {
  const ds = (lbl, color) => ({
    label: lbl, borderColor: color, backgroundColor: 'transparent',
    borderWidth: 1.8, pointRadius: 0, data: []
  });
  return new Chart(document.getElementById(id), {
    type: 'line',
    data: {
      labels: [],
      datasets: [
        ds(label[0], colors[0]),
        ds(label[1], colors[1]),
        ds(label[2], colors[2])
      ]
    },
    options: chartDefaults
  });
}

const magChart = makeChart('magChart',
  ['X (µT)','Y (µT)','Z (µT)'], ['#f55','#5f5','#55f']);
const accChart = makeChart('accChart',
  ['X (g)','Y (g)','Z (g)'], ['#f90','#0cf','#f0f']);

function pushChart(chart, v0, v1, v2) {
  const t = new Date().toLocaleTimeString();
  const d = chart.data;
  if (d.labels.length >= MAXPTS) {
    d.labels.shift();
    d.datasets.forEach(ds => ds.data.shift());
  }
  d.labels.push(t);
  d.datasets[0].data.push(v0);
  d.datasets[1].data.push(v1);
  d.datasets[2].data.push(v2);
  chart.update('none');
}

// ─── DATA POLLING ────────────────────────────────────────────────────
async function fetchData() {
  try {
    const r = await fetch('/data', { cache:'no-store' });
    const d = await r.json();
    if (!d || d.roll === undefined) return;

    // 3D model
    targetRoll  = d.roll;
    targetPitch = d.pitch;
    targetYaw   = d.yaw;

    // Readouts
    document.getElementById('v-roll').textContent  = d.roll.toFixed(2);
    document.getElementById('v-pitch').textContent = d.pitch.toFixed(2);
    document.getElementById('v-yaw').textContent   = d.yaw.toFixed(2);

    // Charts
    pushChart(magChart, d.mx, d.my, d.mz);
    pushChart(accChart, d.ax, d.ay, d.az);

    // Motor
    document.getElementById('motor-val').textContent = d.motor;
    document.getElementById('status').textContent =
      '🟢 Live — ' + new Date().toLocaleTimeString();

  } catch(e) {
    document.getElementById('status').textContent = '🔴 No connection';
  }
}

setInterval(fetchData, 200);
fetchData();
</script>
</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML)

if __name__ == '__main__':
    print(f"Polling ESP32 at {ESP32_URL}")
    print("Open http://localhost:5000 in your browser")
    app.run(host='0.0.0.0', port=5000, debug=False)
