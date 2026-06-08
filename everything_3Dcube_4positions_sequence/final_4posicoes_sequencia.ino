#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include "qmc5883p.h"
#include "MPU6050_tockn.h"

// ------------------- PROTÓTIPOS -------------------
void setMotorA(int speed);
void setMotorB(int speed);
void stopMotors();
String motorSymbol(int speed);
String positionName(int posCase);
String motorStateText(const String &label);
void applyMotorCase(int posCase);
void startAutoSequence();
void updateAutoSequence();
void handleRoot();
void handleData();
void handleMotor();

// ------------------- WI-FI -------------------
const char* ssid     = "CW";
const char* password = "244466666";
WebServer server(80);

// ------------------- I2C PINS ESP32-S3 MINI -------------------
#define I2C_SDA 9
#define I2C_SCL 8

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
String  currentMotor = "Stopped | Motor A: 0 | Motor B: 0";
int motorACommand = 0;
int motorBCommand = 0;
int currentPosition = -1;
bool autoSequenceActive = false;
int autoSequenceCase = 0;
unsigned long lastAutoSequenceTime = 0;
const unsigned long autoSequenceInterval = 30000;

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
      border: 0;
      background: transparent;
      padding: 0;
      font: inherit;
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

    .switch-btn.active .switch-label {
      border-color: #007BFF;
      background: #e8f2ff;
      box-shadow: 0 0 10px #007BFF22;
    }
    .switch-btn.active .switch-label .name { color: #007BFF; }
    .switch-btn.active .switch-label .pill { background: #007BFF55; }
    .switch-btn.active .switch-label .pill::after {
      transform: translateX(20px);
      background: #007BFF;
    }

    .switch-btn.left.active .switch-label { border-color: #00aa55; background: #e6f9ef; box-shadow: 0 0 10px #00aa5522; }
    .switch-btn.left.active .switch-label .name { color: #00aa55; }
    .switch-btn.left.active .switch-label .pill { background: #00aa5555; }
    .switch-btn.left.active .switch-label .pill::after { background: #00aa55; }

    .switch-btn.right.active .switch-label { border-color: #dd2222; background: #fff0f0; box-shadow: 0 0 10px #dd222222; }
    .switch-btn.right.active .switch-label .name { color: #dd2222; }
    .switch-btn.right.active .switch-label .pill { background: #dd222255; }
    .switch-btn.right.active .switch-label .pill::after { background: #dd2222; }

    .switch-btn.stop.active .switch-label { border-color: #ee8800; background: #fff8e6; box-shadow: 0 0 10px #ee880022; }
    .switch-btn.stop.active .switch-label .name { color: #ee8800; }
    .switch-btn.stop.active .switch-label .pill { background: #ee880055; }
    .switch-btn.stop.active .switch-label .pill::after { background: #ee8800; }

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
      background: #ffffff; 
      border: 1px solid #ddd;
    }
    .legend {
      margin-top: 8px;
      font-size: 0.75em;
      color: #666;
      text-align: center;
      letter-spacing: 0.06em;
    }

    /* ---- 3D CUBE ---- */
    .scene {
      width: 100%;
      height: 220px;
      perspective: 600px;
      display: flex;
      justify-content: center;
      align-items: center;
      background: #ffffff;
      border-radius: 6px;
      border: 1px solid #ddd;
    }
    .cube {
      width: 100px;
      height: 100px;
      position: relative;
      transform-style: preserve-3d;
      /* Smooth transition to animate the cube at each 500 ms reading */
      transition: transform 0.5s ease-out; 
    }
    .cube__face {
      position: absolute;
      width: 100px;
      height: 100px;
      border: 2px solid #fff;
      line-height: 96px;
      font-size: 16px;
      font-weight: bold;
      color: white;
      text-align: center;
      box-shadow: inset 0 0 20px rgba(0,0,0,0.3);
    }
    .cube__face--front  { background: rgba(255, 68, 68, 0.9);  transform: rotateY(  0deg) translateZ(50px); }
    .cube__face--back   { background: rgba(0, 170, 85, 0.9);   transform: rotateY(180deg) translateZ(50px); }
    .cube__face--right  { background: rgba(68, 136, 255, 0.9); transform: rotateY( 90deg) translateZ(50px); }
    .cube__face--left   { background: rgba(204, 68, 255, 0.9); transform: rotateY(-90deg) translateZ(50px); }
    .cube__face--top    { background: rgba(255, 136, 0, 0.9);  transform: rotateX( 90deg) translateZ(50px); }
    .cube__face--bottom { background: rgba(0, 204, 204, 0.9);  transform: rotateX(-90deg) translateZ(50px); }

  </style>
</head>
<body>
  <h1>ESP32 Control Panel</h1>

  <div class="card">
    <h2>Motor Control</h2>
    <div class="switch-group" id="motorGroup">

      <button type="button" class="switch-btn pos0" data-cmd="pos0" onclick="sendMotor('pos0')">
        <div class="switch-label">
          <span class="icon">1</span>
          <span class="name">Back</span>
          <div class="pill"></div>
        </div>
      </button>

      <button type="button" class="switch-btn pos1" data-cmd="pos1" onclick="sendMotor('pos1')">
        <div class="switch-label">
          <span class="icon">2</span>
          <span class="name">Left</span>
          <div class="pill"></div>
        </div>
      </button>

      <button type="button" class="switch-btn pos2" data-cmd="pos2" onclick="sendMotor('pos2')">
        <div class="switch-label">
          <span class="icon">3</span>
          <span class="name">Front</span>
          <div class="pill"></div>
        </div>
      </button>

      <button type="button" class="switch-btn pos3" data-cmd="pos3" onclick="sendMotor('pos3')">
        <div class="switch-label">
          <span class="icon">4</span>
          <span class="name">Right</span>
          <div class="pill"></div>
        </div>
      </button>

      <button type="button" class="switch-btn auto" data-cmd="auto" onclick="sendMotor('auto')">
        <div class="switch-label">
          <span class="icon">🔁</span>
          <span class="name">Sequence</span>
          <div class="pill"></div>
        </div>
      </button>

      <button type="button" class="switch-btn stop active" data-cmd="stop" onclick="sendMotor('stop')">
        <div class="switch-label">
          <span class="icon">⏹️</span>
          <span class="name">Stop</span>
          <div class="pill"></div>
        </div>
      </button>

    </div>
    <p id="motor-status">Motor status: <span id="motorLabel">Stopped | Motor A: 0 | Motor B: 0</span></p>
  </div>

  <div class="card">
    <h2>Magnetometer</h2>
    <canvas id="magChart"></canvas>
    <p class="legend">X = red &nbsp;|&nbsp; Y = green &nbsp;|&nbsp; Z = blue</p>
  </div>

  <div class="card">
    <h2>Acceleration</h2>
    <canvas id="accChart"></canvas>
    <p class="legend">X = red &nbsp;|&nbsp; Y = green &nbsp;|&nbsp; Z = blue</p>
  </div>

  <div class="card">
    <h2>3D Orientation (Yaw / Pitch / Roll)</h2>
    <div class="scene">
      <div class="cube" id="cube3d">
        <div class="cube__face cube__face--front">Front</div>
        <div class="cube__face cube__face--back">Back</div>
        <div class="cube__face cube__face--right">Right</div>
        <div class="cube__face cube__face--left">Left</div>
        <div class="cube__face cube__face--top">Top</div>
        <div class="cube__face cube__face--bottom">Bottom</div>
      </div>
    </div>
    <p class="legend" id="yprText">Yaw: 0° | Pitch: 0° | Roll: 0°</p>
  </div>

  <script>
    // ---- Motor buttons ----
    const motorButtons = document.querySelectorAll('.switch-btn');
    const motorLabel = document.getElementById('motorLabel');

    const stateLabels = {
      pos0: 'Back',
      pos1: 'Left',
      pos2: 'Front',
      pos3: 'Right',
      auto: 'Auto sequence',
      stop: 'Stopped | Motor A: 0 | Motor B: 0'
    };

    function sendMotor(cmd) {
      motorLabel.textContent = stateLabels[cmd] || cmd;

      motorButtons.forEach(btn => btn.classList.remove('active'));
      const selected = document.querySelector('.switch-btn[data-cmd="' + cmd + '"]');
      if (selected) selected.classList.add('active');

      fetch('/motor?cmd=' + cmd, { method: 'GET' })
        .then(r => r.text())
        .then(txt => { motorLabel.textContent = txt.replace('OK:', ''); })
        .catch(err => console.warn('Error sending command:', err));
    }

    // ---- Charts ----
    const maxPoints = 60;
    function makeSeries() { return { labels: [], values: [[], [], []] }; }
    const magData = makeSeries();
    const accData = makeSeries();

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
    }

    async function fetchData() {
      try {
        const res  = await fetch('/data', { cache: 'no-store' });
        const data = await res.json();

        // Keep switch UI in sync with ESP32 state
        motorLabel.textContent = data.motor;

        // Update 2D Charts
        pushValue(magData, [data.mx, data.my, data.mz]);
        pushValue(accData, [data.ax, data.ay, data.az]);
        updateCharts();

        // Update 3D Cube
        const cube = document.getElementById('cube3d');
        const yprText = document.getElementById('yprText');
        
        // CSS rotations are applied using the sensor angles
        cube.style.transform = `rotateX(${data.pitch}deg) rotateY(${data.yaw}deg) rotateZ(${data.roll}deg)`;
        yprText.textContent = `Yaw: ${data.yaw.toFixed(2)}° | Pitch: ${data.pitch.toFixed(2)}° | Roll: ${data.roll.toFixed(2)}°`;

      } catch (e) {
        console.warn('Error connecting to ESP32:', e);
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
  server.send(200, "application/json", json);
}

void handleMotor() {
  if (!server.hasArg("cmd")) {
    server.send(400, "text/plain", "Missing cmd");
    return;
  }
  String cmd = server.arg("cmd");

  if (cmd == "pos0") {
    autoSequenceActive = false;
    applyMotorCase(0);
    currentMotor = motorStateText("Back");
  } else if (cmd == "pos1") {
    autoSequenceActive = false;
    applyMotorCase(1);
    currentMotor = motorStateText("Left");
  } else if (cmd == "pos2") {
    autoSequenceActive = false;
    applyMotorCase(2);
    currentMotor = motorStateText("Front");
  } else if (cmd == "pos3") {
    autoSequenceActive = false;
    applyMotorCase(3);
    currentMotor = motorStateText("Right");
  } else if (cmd == "auto") {
    startAutoSequence();
  } else if (cmd == "stop") {
    autoSequenceActive = false;
    stopMotors();
    currentPosition = -1;
    currentMotor = motorStateText("Stopped");
  } else {
    server.send(400, "text/plain", "Invalid command");
    return;
  }

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK:" + currentMotor);
  Serial.println("Motor cmd: " + cmd + " -> " + currentMotor);
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
    Serial.println("QMC5883P error!");
    while (true) delay(1000);
  }

  mpu6050.begin();
  Serial.println("A calcular offsets... nao mexas no sensor!");
  mpu6050.calcGyroOffsets(true);
  mpu6050.update();
  yaw = mpu6050.getAngleZ();
  Serial.println("Controlo dos motores por posicoes fixas, sem depender do yaw.");

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
  updateAutoSequence();

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
const int MOTOR_SPEED = 200;

String motorSymbol(int speed) {
  if (speed > 0) return "+";
  if (speed < 0) return "-";
  return "0";
}

String positionName(int posCase) {
  switch (((posCase % 4) + 4) % 4) {
    case 0: return "Back";
    case 1: return "Left";
    case 2: return "Front";
    case 3: return "Right";
  }
  return "Unknown";
}

String motorStateText(const String &label) {
  return label + " | Motor A: " + motorSymbol(motorACommand) + " | Motor B: " + motorSymbol(motorBCommand);
}

void applyMotorCase(int posCase) {
  posCase = ((posCase % 4) + 4) % 4;
  currentPosition = posCase;

  // Map of the 4 positions from the image, in the order where the red face rotates to the right.
  switch (posCase) {
    case 0:  // Front
      setMotorA(0);
      setMotorB(MOTOR_SPEED);
      break;
    case 1:  // Right
      setMotorA(MOTOR_SPEED);
      setMotorB(0);
      break;
    case 2:  // Back
      setMotorA(-95);
      setMotorB(-MOTOR_SPEED);
      break;
    case 3:  // Left
      setMotorA(-MOTOR_SPEED);
      setMotorB(0);
      break;
  }
}

void startAutoSequence() {
  autoSequenceActive = true;

  // If no direction has been chosen yet, start at Front.
  // If a direction was already active, continue from the next direction.
  if (currentPosition < 0) {
    autoSequenceCase = 0;
  } else {
    autoSequenceCase = (currentPosition + 1) % 4;
  }

  applyMotorCase(autoSequenceCase);
  lastAutoSequenceTime = millis();
  currentMotor = motorStateText("Sequence | " + positionName(currentPosition));
}

void updateAutoSequence() {
  if (!autoSequenceActive) return;

  unsigned long now = millis();
  if (now - lastAutoSequenceTime >= autoSequenceInterval) {
    lastAutoSequenceTime = now;
    autoSequenceCase = (currentPosition + 1) % 4;
    applyMotorCase(autoSequenceCase);
    currentMotor = motorStateText("Sequence | " + positionName(currentPosition));
    Serial.println(currentMotor);
  }
}

void setMotorA(int speed) {
  speed = constrain(speed, -255, 255);
  motorACommand = speed;
  if      (speed > 0) { analogWrite(MOTOR_A_IN1, speed);  analogWrite(MOTOR_A_IN2, 0);      }
  else if (speed < 0) { analogWrite(MOTOR_A_IN1, 0);      analogWrite(MOTOR_A_IN2, -speed); }
  else                { analogWrite(MOTOR_A_IN1, 0);      analogWrite(MOTOR_A_IN2, 0);      }
}

void setMotorB(int speed) {
  speed = constrain(speed, -255, 255);
  motorBCommand = speed;
  if      (speed > 0) { analogWrite(MOTOR_B_IN3, speed);  analogWrite(MOTOR_B_IN4, 0);      }
  else if (speed < 0) { analogWrite(MOTOR_B_IN3, 0);      analogWrite(MOTOR_B_IN4, -speed); }
  else                { analogWrite(MOTOR_B_IN3, 0);      analogWrite(MOTOR_B_IN4, 0);      }
}

void stopMotors() {
  motorACommand = 0;
  motorBCommand = 0;
  analogWrite(MOTOR_A_IN1, 0);
  analogWrite(MOTOR_A_IN2, 0);
  analogWrite(MOTOR_B_IN3, 0);
  analogWrite(MOTOR_B_IN4, 0);
}
