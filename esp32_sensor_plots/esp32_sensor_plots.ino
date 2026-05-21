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

// ------------------- WI-FI -------------------
const char* ssid = "RAMLX131E";
const char* password = "Ssgnr2018#";

WebServer server(80);

// ------------------- I2C PINS ESP32-S3 MINI -------------------
#define I2C_SDA 8
#define I2C_SCL 9

// ------------------- DRV8833 PWM PINS -------------------
#define MOTOR_A_IN1 5
#define MOTOR_A_IN2 6
#define MOTOR_B_IN3 7
#define MOTOR_B_IN4 10

QMC5883P mag;
MPU6050 mpu6050(Wire);

// ------------------- TIMERS -------------------
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 200;   // lê sensores 5x por segundo

unsigned long lastMotorTime = 0;
const unsigned long motorInterval = 4000;
int motorState = 0;

// ------------------- VARIÁVEIS GLOBAIS -------------------
float magX = 0, magY = 0, magZ = 0;
float accX = 0, accY = 0, accZ = 0;
float roll = 0, pitch = 0, yaw = 0;
String currentMotor = "Stopped";

// ------------------- WEBPAGE -------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="utf-8">
  <title>ESP32 Sensor Dashboard</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background: #f4f4f9;
      padding: 10px;
      margin: 0;
    }
    .card {
      background: white;
      border-radius: 12px;
      padding: 15px;
      margin: 15px auto;
      max-width: 900px;
      box-shadow: 0 4px 10px rgba(0,0,0,0.08);
    }
    h1 { color: #333; font-size: 1.6em; }
    h2 { color: #007BFF; margin-bottom: 10px; font-size: 1.15em; }
    p { font-size: 1.05em; color: #555; font-family: monospace; font-weight: bold; }
    canvas {
      width: 100%;
      height: 260px;
      display: block;
      background: #fff;
      border: 1px solid #eee;
      border-radius: 10px;
    }
    .small {
      font-size: 0.9em;
      font-weight: normal;
      font-family: Arial, sans-serif;
    }
  </style>
</head>
<body>
  <h1>Painel de Controlo ESP32</h1>

  <div class="card">
    <h2>Estado das Bobinas (Motores)</h2>
    <p id="motor">A carregar...</p>
  </div>

  <div class="card">
    <h2>Magnetómetro</h2>
    <canvas id="magChart"></canvas>
    <p class="small">X = vermelho, Y = verde, Z = azul</p>
  </div>

  <div class="card">
    <h2>Aceleração</h2>
    <canvas id="accChart"></canvas>
    <p class="small">X = vermelho, Y = verde, Z = azul</p>
  </div>

  <div class="card">
    <h2>Orientação (Yaw / Pitch / Roll)</h2>
    <canvas id="yprChart"></canvas>
    <p class="small">Yaw = roxo, Pitch = laranja, Roll = ciano</p>
  </div>

  <script>
    const maxPoints = 60;

    function makeSeries() {
      return {
        labels: [],
        values: [[], [], []]
      };
    }

    const magData = makeSeries();
    const accData = makeSeries();
    const yprData = makeSeries();

    function pushValue(series, values) {
      series.labels.push('');
      for (let i = 0; i < 3; i++) {
        series.values[i].push(values[i]);
      }

      if (series.labels.length > maxPoints) {
        series.labels.shift();
        for (let i = 0; i < 3; i++) {
          series.values[i].shift();
        }
      }
    }

    function drawChart(canvasId, series, title, lineColors) {
      const canvas = document.getElementById(canvasId);
      const ctx = canvas.getContext('2d');

      const dpr = window.devicePixelRatio || 1;
      const rect = canvas.getBoundingClientRect();
      const width = Math.max(300, rect.width);
      const height = Math.max(220, rect.height);

      if (canvas.width !== Math.floor(width * dpr) || canvas.height !== Math.floor(height * dpr)) {
        canvas.width = Math.floor(width * dpr);
        canvas.height = Math.floor(height * dpr);
      }

      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, width, height);

      const padL = 50;
      const padR = 15;
      const padT = 20;
      const padB = 30;
      const plotW = width - padL - padR;
      const plotH = height - padT - padB;

      // Fundo
      ctx.fillStyle = "#ffffff";
      ctx.fillRect(0, 0, width, height);

      // Moldura
      ctx.strokeStyle = "#dddddd";
      ctx.strokeRect(padL, padT, plotW, plotH);

      const allValues = [...series.values[0], ...series.values[1], ...series.values[2]];
      if (allValues.length === 0) return;

      let minY = Math.min(...allValues);
      let maxY = Math.max(...allValues);

      if (minY === maxY) {
        minY -= 1;
        maxY += 1;
      }

      const margin = (maxY - minY) * 0.15;
      minY -= margin;
      maxY += margin;

      const stepY = (maxY - minY) / 4;

      // Grid horizontal + labels
      ctx.font = "12px Arial";
      ctx.fillStyle = "#666";
      ctx.strokeStyle = "#eeeeee";

      for (let i = 0; i <= 4; i++) {
        const y = padT + (plotH * i) / 4;
        ctx.beginPath();
        ctx.moveTo(padL, y);
        ctx.lineTo(padL + plotW, y);
        ctx.stroke();

        const val = maxY - i * stepY;
        ctx.fillText(val.toFixed(2), 5, y + 4);
      }

      if (series.labels.length < 2) {
        ctx.fillStyle = "#999";
        ctx.fillText("A aguardar dados...", padL + 10, padT + 20);
        return;
      }

      const xStep = plotW / (maxPoints - 1);

      // Desenha linhas
      for (let s = 0; s < 3; s++) {
        const values = series.values[s];
        if (!values || values.length < 2) continue;

        ctx.strokeStyle = lineColors[s];
        ctx.lineWidth = 2;
        ctx.beginPath();

        for (let i = 0; i < values.length; i++) {
          const x = padL + i * xStep;
          const y = padT + plotH - ((values[i] - minY) / (maxY - minY)) * plotH;

          if (i === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        }
        ctx.stroke();
      }

      // Título
      ctx.fillStyle = "#333";
      ctx.font = "bold 13px Arial";
      ctx.fillText(title, padL, 14);
    }

    function updateCharts() {
      drawChart("magChart", magData, "Magnetómetro", ["red", "green", "blue"]);
      drawChart("accChart", accData, "Aceleração", ["red", "green", "blue"]);
      drawChart("yprChart", yprData, "Yaw / Pitch / Roll", ["purple", "orange", "cyan"]);
    }

    async function fetchData() {
      try {
        const response = await fetch('/data', { cache: 'no-store' });
        const data = await response.json();

        document.getElementById('motor').innerText = data.motor;

        pushValue(magData, [data.mx, data.my, data.mz]);
        pushValue(accData, [data.ax, data.ay, data.az]);
        pushValue(yprData, [data.yaw, data.pitch, data.roll]);

        updateCharts();
      } catch (err) {
        console.log("Erro a ligar ao ESP32:", err);
      }
    }

    window.addEventListener("resize", updateCharts);
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
  json += "\"motor\":\"" + currentMotor + "\",";
  json += "\"mx\":" + String(magX, 2) + ",";
  json += "\"my\":" + String(magY, 2) + ",";
  json += "\"mz\":" + String(magZ, 2) + ",";
  json += "\"ax\":" + String(accX, 2) + ",";
  json += "\"ay\":" + String(accY, 2) + ",";
  json += "\"az\":" + String(accZ, 2) + ",";
  json += "\"yaw\":" + String(yaw, 2) + ",";
  json += "\"pitch\":" + String(pitch, 2) + ",";
  json += "\"roll\":" + String(roll, 2);
  json += "}";

  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "application/json", json);
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

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
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

  // Atualiza dados do MPU6050 continuamente
  roll  = mpu6050.getAngleX();
  pitch = mpu6050.getAngleY();
  yaw   = mpu6050.getAngleZ();

  accX = mpu6050.getAccX();
  accY = mpu6050.getAccY();
  accZ = mpu6050.getAccZ();

  if (millis() - lastMotorTime >= motorInterval) {
    lastMotorTime = millis();
    switch (motorState) {
      case 0:
        setMotorA(200);
        setMotorB(200);
        currentMotor = "Frente / Ativo (+)";
        motorState = 1;
        break;

      case 1:
        stopMotors();
        currentMotor = "Parado";
        motorState = 2;
        break;

      case 2:
        setMotorA(-150);
        setMotorB(-150);
        currentMotor = "Trás / Ativo (-)";
        motorState = 3;
        break;

      case 3:
        stopMotors();
        currentMotor = "Parado";
        motorState = 0;
        break;
    }
  }

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
  if (speed > 0) {
    analogWrite(MOTOR_A_IN1, speed);
    analogWrite(MOTOR_A_IN2, 0);
  } else if (speed < 0) {
    analogWrite(MOTOR_A_IN1, 0);
    analogWrite(MOTOR_A_IN2, -speed);
  } else {
    analogWrite(MOTOR_A_IN1, 0);
    analogWrite(MOTOR_A_IN2, 0);
  }
}

void setMotorB(int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    analogWrite(MOTOR_B_IN3, speed);
    analogWrite(MOTOR_B_IN4, 0);
  } else if (speed < 0) {
    analogWrite(MOTOR_B_IN3, 0);
    analogWrite(MOTOR_B_IN4, -speed);
  } else {
    analogWrite(MOTOR_B_IN3, 0);
    analogWrite(MOTOR_B_IN4, 0);
  }
}

void stopMotors() {
  analogWrite(MOTOR_A_IN1, 0);
  analogWrite(MOTOR_A_IN2, 0);
  analogWrite(MOTOR_B_IN3, 0);
  analogWrite(MOTOR_B_IN4, 0);
}