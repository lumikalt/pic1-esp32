
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include "qmc5883p.h"
#include "MPU6050_tockn.h"

// ------------------- PROTÓTIPOS -------------------
void setMotorA(int speed);
void setMotorB(int speed);
void stopMotors();
void handleRoot();
void handleData();
void handleMotor();

// ------------------- WI-FI -------------------
const char* ssid     = "CW";
const char* password = "244466666";
WebServer server(80);

// ------------------- I2C PINS ESP32-S3 MINI -------------------
#define I2C_SDA 8
#define I2C_SCL 9

// ------------------- DRV8833 PWM PINS -------------------
#define MOTOR_A_IN1 6
#define MOTOR_A_IN2 5
#define MOTOR_B_IN3 4
#define MOTOR_B_IN4 3

QMC5883P mag;
MPU6050  mpu6050(Wire);

// ------------------- TIMERS -------------------
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 200;

// ------------------- VARIÁVEIS GLOBAIS -------------------
float   magX = 0, magY = 0, magZ = 0;
float   accX = 0, accY = 0, accZ = 0;
float   roll = 0, pitch = 0, yaw = 0;
String  currentMotor = "Parado";

// ------------------- WEBPAGE -------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="utf-8">
  <title>ESP32 Sensor Dashboard</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      font-family: 'Courier New', monospace;
      /* Nova imagem de fundo (Galáxia e Estrelas) */
      background-image: url('https://c4.wallpaperflare.com/wallpaper/43/858/375/stars-planets-4k-galaxy-wallpaper-preview.jpg');
      background-size: cover;
      background-position: center;
      background-repeat: no-repeat;
      background-attachment: fixed;
      color: #333;
      padding: 16px;
      min-height: 100vh;
    }

    h1 {
      text-align: center;
      font-size: 1.4em;
      letter-spacing: 0.1em;
      color: #007BFF;
      text-transform: uppercase;
      margin-bottom: 20px;
      text-shadow: 0 0 10px rgba(0, 123, 255, 0.7), 0 0 20px rgba(0,0,0,0.9);
    }

    .card {
      background: rgba(255, 255, 255, 0.95);
      border: 1px solid #e0e0e0;
      border-radius: 10px;
      padding: 16px;
      margin: 14px auto;
      max-width: 920px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.5);
    }

    h2 {
      font-size: 0.85em;
      letter-spacing: 0.1em;
      text-transform: uppercase;
      color: #007BFF;
      margin-bottom: 14px;
    }

    /* ---- MOTOR SWITCHES ---- */
    .switch-group {
      display: flex;
      flex-wrap: wrap;
      gap: 12px;
      justify-content: center;
    }

    .switch-btn {
      position: relative;
      cursor: pointer;
      user-select: none;
    }

    .switch-btn input {
      position: absolute;
      opacity: 0;
      width: 0; height: 0;
    }

    .switch-label {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 8px;
      padding: 14px 22px;
      border: 2px solid #ddd;
      border-radius: 10px;
      background: #f9f9f9;
      transition: all 0.18s ease;
      min-width: 130px;
    }

    .switch-label .icon { font-size: 1.8em; }
    .switch-label .name {
      font-size: 0.8em;
      letter-spacing: 0.1em;
      text-transform: uppercase;
      color: #999;
      transition: color 0.18s;
    }

    /* Pill toggle inside label */
    .switch-label .pill {
      width: 42px; height: 22px;
      background: #ddd;
      border-radius: 11px;
      position: relative;
      transition: background 0.18s;
    }
    .switch-label .pill::after {
      content: '';
      position: absolute;
      width: 16px; height: 16px;
      border-radius: 50%;
      background: #bbb;
      top: 3px; left: 3px;
      transition: transform 0.18s, background 0.18s;
    }

    /* Active state fallback */
    .switch-btn input:checked + .switch-label {
      border-color: #007BFF;
      background: #e8f2ff;
      box-shadow: 0 0 10px #007BFF22;
    }
    .switch-btn input:checked + .switch-label .name { color: #007BFF; }
    .switch-btn input:checked + .switch-label .pill { background: #007BFF55; }
    .switch-btn input:checked + .switch-label .pill::after {
      transform: translateX(20px);
      background: #007BFF;
    }

    /* Per-button accent colours */
    .switch-btn.forward input:checked + .switch-label { border-color: #00aa55; background: #e6f9ef; box-shadow: 0 0 10px #00aa5522; }
    .switch-btn.forward input:checked + .switch-label .name { color: #00aa55; }
    .switch-btn.forward input:checked + .switch-label .pill { background: #00aa5555; }
    .switch-btn.forward input:checked + .switch-label .pill::after { background: #00aa55; }

    .switch-btn.backward input:checked + .switch-label { border-color: #dd2222; background: #fff0f0; box-shadow: 0 0 10px #dd222222; }
    .switch-btn.backward input:checked + .switch-label .name { color: #dd2222; }
    .switch-btn.backward input:checked + .switch-label .pill { background: #dd222255; }
    .switch-btn.backward input:checked + .switch-label .pill::after { background: #dd2222; }

    .switch-btn.stop input:checked + .switch-label { border-color: #ee8800; background: #fff8e6; box-shadow: 0 0 10px #ee880022; }
    .switch-btn.stop input:checked + .switch-label .name { color: #ee8800; }
    .switch-btn.stop input:checked + .switch-label .pill { background: #ee880055; }
    .switch-btn.stop input:checked + .switch-label .pill::after { background: #ee8800; }

    /* Status bar */
    #motor-status {
      margin-top: 14px;
      text-align: center;
      font-size: 0.9em;
      letter-spacing: 0.08em;
      color: #666;
    }
    #motor-status span { color: #007BFF; font-weight: bold; }

    /* ---- CHARTS ---- */
    canvas {
      width: 100%;
      height: 220px;
      display: block;
      border-radius: 6px;
      background: #ffffff; /* Fundo mantido em Branco */
      border: 1px solid #ddd;
    }
    .legend {
      margin-top: 8px;
      font-size: 0.75em;
      color: #666;
      text-align: center;
      letter-spacing: 0.06em;
    }
  </style>
</head>
<body>
  <h1>Painel de Controlo ESP32</h1>

  <div class="card">
    <h2>Controlo dos Motores</h2>
    <div class="switch-group" id="motorGroup">

      <label class="switch-btn forward">
        <input type="radio" name="motor" value="forward">
        <div class="switch-label">
          <span class="icon">⬆️</span>
          <span class="name">Frente</span>
          <div class="pill"></div>
        </div>
      </label>

      <label class="switch-btn stop">
        <input type="radio" name="motor" value="stop" checked>
        <div class="switch-label">
          <span class="icon">⏹️</span>
          <span class="name">Parar</span>
          <div class="pill"></div>
        </div>
      </label>

      <label class="switch-btn backward">
        <input type="radio" name="motor" value="backward">
        <div class="switch-label">
          <span class="icon">⬇️</span>
          <span class="name">Trás</span>
          <div class="pill"></div>
        </div>
      </label>

    </div>
    <p id="motor-status">Estado atual: <span id="motorLabel">Parado</span></p>
  </div>

  <div class="card">
    <h2>Magnetómetro</h2>
    <canvas id="magChart"></canvas>
    <p class="legend">X = vermelho &nbsp;|&nbsp; Y = verde &nbsp;|&nbsp; Z = azul</p>
  </div>

  <div class="card">
    <h2>Aceleração</h2>
    <canvas id="accChart"></canvas>
    <p class="legend">X = vermelho &nbsp;|&nbsp; Y = verde &nbsp;|&nbsp; Z = azul</p>
  </div>

  <div class="card">
    <h2>Orientação (Yaw / Pitch / Roll)</h2>
    <canvas id="yprChart"></canvas>
    <p class="legend">Yaw = roxo &nbsp;|&nbsp; Pitch = laranja &nbsp;|&nbsp; Roll = ciano</p>
  </div>

  <script>
    // ---- Motor switches ----
    const radios = document.querySelectorAll('input[name="motor"]');
    const motorLabel = document.getElementById('motorLabel');

    const stateLabels = {
      forward:  'Frente / Ativo (+)',
      stop:     'Parado',
      backward: 'Trás / Ativo (-)'
    };

    radios.forEach(radio => {
      radio.addEventListener('change', () => {
        if (!radio.checked) return;
        const cmd = radio.value;
        motorLabel.textContent = stateLabels[cmd] || cmd;

        fetch('/motor?cmd=' + cmd, { method: 'GET' })
          .catch(err => console.warn('Erro ao enviar comando:', err));
      });
    });

    // ---- Charts ----
    const maxPoints = 60;
    function makeSeries() { return { labels: [], values: [[], [], []] }; }
    const magData = makeSeries();
    const accData = makeSeries();
    const yprData = makeSeries();

    function pushValue(series, values) {
      series.labels.push('');
      for (let i = 0; i < 3; i++) series.values[i].push(values[i]);
      if (series.labels.length > maxPoints) {
        series.labels.shift();
        for (let i = 0; i < 3; i++) series.values[i].shift();
      }
    }

    function drawChart(canvasId, series, lineColors) {
      const canvas = document.getElementById(canvasId);
      const ctx    = canvas.getContext('2d');
      const dpr    = window.devicePixelRatio || 1;
      const rect   = canvas.getBoundingClientRect();
      const width  = Math.max(300, rect.width);
      const height = Math.max(200, rect.height);

      if (canvas.width  !== Math.floor(width  * dpr) ||
          canvas.height !== Math.floor(height * dpr)) {
        canvas.width  = Math.floor(width  * dpr);
        canvas.height = Math.floor(height * dpr);
      }
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, width, height);

      const padL = 52, padR = 12, padT = 18, padB = 28;
      const plotW = width  - padL - padR;
      const plotH = height - padT - padB;

      ctx.fillStyle = '#ffffff';
      ctx.fillRect(0, 0, width, height);

      const all = series.values.flat();
      if (all.length === 0) return;
      let minY = Math.min(...all), maxY = Math.max(...all);
      if (minY === maxY) { minY -= 1; maxY += 1; }
      const mg = (maxY - minY) * 0.15;
      minY -= mg; maxY += mg;
      const rng = maxY - minY;

      ctx.strokeStyle = '#eee';
      ctx.lineWidth = 1;
      ctx.font = '11px Courier New';
      ctx.fillStyle = '#666';
      for (let i = 0; i <= 4; i++) {
        const y = padT + (plotH * i) / 4;
        ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(padL + plotW, y); ctx.stroke();
        ctx.fillText((maxY - i * rng / 4).toFixed(1), 4, y + 4);
      }

      if (series.labels.length < 2) return;
      const xStep = plotW / (maxPoints - 1);

      for (let s = 0; s < 3; s++) {
        const vals = series.values[s];
        if (vals.length < 2) continue;
        ctx.strokeStyle = lineColors[s];
        ctx.lineWidth = 1.8;
        ctx.shadowColor  = lineColors[s];
        ctx.shadowBlur = 2;
        ctx.beginPath();
        for (let i = 0; i < vals.length; i++) {
          const x = padL + i * xStep;
          const y = padT + plotH - ((vals[i] - minY) / rng) * plotH;
          i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
        }
        ctx.stroke();
        ctx.shadowBlur = 0;
      }
    }

    function updateCharts() {
      drawChart('magChart', magData, ['#ff4444', '#00aa55', '#4488ff']);
      drawChart('accChart', accData, ['#ff4444', '#00aa55', '#4488ff']);
      drawChart('yprChart', yprData, ['#cc44ff', '#ff8800', '#00cccc']);
    }

    async function fetchData() {
      try {
        const res  = await fetch('/data', { cache: 'no-store' });
        const data = await res.json();

        // Keep switch UI in sync with ESP32 state
        motorLabel.textContent = data.motor;

        pushValue(magData, [data.mx, data.my, data.mz]);
        pushValue(accData, [data.ax, data.ay, data.az]);
        pushValue(yprData, [data.yaw, data.pitch, data.roll]);
        updateCharts();
      } catch (e) {
        console.warn('Erro a ligar ao ESP32:', e);
      }
    }

    window.addEventListener('resize', updateCharts);
    setInterval(fetchData, 500);
    fetchData();
  </script>
</body>
</html>
)rawliteral";

// ------------------- HANDLERS -------------------
void handleRoot() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "text/html; charset=utf-8", index_html);
}

void handleData() {
  String json = "{";
  json += "\"motor\":\""  + currentMotor    + "\",";
  json += "\"mx\":"       + String(magX, 2) + ",";
  json += "\"my\":"       + String(magY, 2) + ",";
  json += "\"mz\":"       + String(magZ, 2) + ",";
  json += "\"ax\":"       + String(accX, 2) + ",";
  json += "\"ay\":"       + String(accY, 2) + ",";
  json += "\"az\":"       + String(accZ, 2) + ",";
  json += "\"yaw\":"      + String(yaw,  2) + ",";
  json += "\"pitch\":"    + String(pitch, 2) + ",";
  json += "\"roll\":"     + String(roll,  2);
  json += "}";
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  // Enable CORS
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleMotor() {
  if (!server.hasArg("cmd")) {
    server.send(400, "text/plain", "Missing cmd");
    return;
  }
  String cmd = server.arg("cmd");

  if (cmd == "forward") {
    setMotorA(200);
    setMotorB(200);
    currentMotor = "Frente / Ativo (+)";
  } else if (cmd == "backward") {
    setMotorA(-150);
    setMotorB(-150);
    currentMotor = "Trás / Ativo (-)";
  } else {
    stopMotors();
    currentMotor = "Parado";
  }

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK:" + currentMotor);
  Serial.println("Motor cmd: " + cmd + " → " + currentMotor);
}

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.print("A ligar ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi ligado!");
  Serial.print("Abre no browser este IP: ");
  Serial.println(WiFi.localIP());

  server.on("/",      HTTP_GET, handleRoot);
  server.on("/data",  HTTP_GET, handleData);
  server.on("/motor", HTTP_GET, handleMotor);
  server.begin();

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  if (!mag.begin()) {
    Serial.println("Erro no QMC5883P!");
    while (true) delay(1000);
  }

  mpu6050.begin();
  Serial.println("A calcular offsets... nao mexas no sensor!");
  mpu6050.calcGyroOffsets(true);

  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);
  stopMotors();

  Serial.println("Tudo pronto. Podes abrir o IP no browser.");
}

// ------------------- LOOP -------------------
void loop() {
  server.handleClient();

  mpu6050.update();
  roll  = mpu6050.getAngleX();
  pitch = mpu6050.getAngleY();
  yaw   = mpu6050.getAngleZ();
  accX  = mpu6050.getAccX();
  accY  = mpu6050.getAccY();
  accZ  = mpu6050.getAccZ();

  if (millis() - lastPrintTime >= printInterval) {
    lastPrintTime = millis();
    float xyz[3];
    if (mag.readXYZ(xyz)) {
      magX = xyz[0];
      magY = xyz[1];
      magZ = xyz[2];
    }
    Serial.printf(
      "Mag(X:%.2f Y:%.2f Z:%.2f) | Acc(X:%.2f Y:%.2f Z:%.2f) | YPR(%.2f, %.2f, %.2f)\n",
      magX, magY, magZ, accX, accY, accZ, yaw, pitch, roll
    );
  }
}

// ------------------- MOTORES -------------------
void setMotorA(int speed) {
  speed = constrain(speed, -255, 255);
  if      (speed > 0) { analogWrite(MOTOR_A_IN1, speed);  analogWrite(MOTOR_A_IN2, 0);      }
  else if (speed < 0) { analogWrite(MOTOR_A_IN1, 0);      analogWrite(MOTOR_A_IN2, -speed); }
  else                { analogWrite(MOTOR_A_IN1, 0);      analogWrite(MOTOR_A_IN2, 0);      }
}

void setMotorB(int speed) {
  speed = constrain(speed, -255, 255);
  if      (speed > 0) { analogWrite(MOTOR_B_IN3, speed);  analogWrite(MOTOR_B_IN4, 0);      }
  else if (speed < 0) { analogWrite(MOTOR_B_IN3, 0);      analogWrite(MOTOR_B_IN4, -speed); }
  else                { analogWrite(MOTOR_B_IN3, 0);      analogWrite(MOTOR_B_IN4, 0);      }
}

void stopMotors() {
  analogWrite(MOTOR_A_IN1, 0);
  analogWrite(MOTOR_A_IN2, 0);
  analogWrite(MOTOR_B_IN3, 0);
  analogWrite(MOTOR_B_IN4, 0);
}
