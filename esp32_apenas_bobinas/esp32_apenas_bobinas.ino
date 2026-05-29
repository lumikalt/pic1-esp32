#include <WiFi.h>
#include <WebServer.h>

// ------------------- PROTÓTIPOS -------------------
void setMotorA(int speed);
void setMotorB(int speed);
void stopMotors();
void handleRoot();
void handleData();

// ------------------- WI-FI -------------------
const char* ssid = "CW";
const char* password = "244466666";

WebServer server(80);

// ------------------- DRV8833 PINS (ESP32-S3 MINI) -------------------
#define MOTOR_A_IN1 6   // Out1 e Out2 -> Bobina 1 (B1)
#define MOTOR_A_IN2 5
#define MOTOR_B_IN3 4   // Out3 e Out4 -> Bobina 2 (B2)
#define MOTOR_B_IN4 3

// ------------------- TIMERS -------------------
unsigned long lastMotorTime = 0;
const unsigned long motorInterval = 5000;  // 5 segundos por estado
int motorState = 0;

// ------------------- VARIÁVEIS GLOBAIS -------------------
String currentMotor = "Inicializando...";

// ----------- WEBPAGE MINIMALISTA (Para ver o Estado via Wi-Fi) -----------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="utf-8">
  <title>Teste de Bobinas ESP32</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background: #f4f4f9;
      padding: 30px;
      margin: 0;
    }
    .card {
      background: white;
      border-radius: 12px;
      padding: 30px;
      margin: 20px auto;
      max-width: 500px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.1);
    }
    h1 { color: #333; font-size: 1.8em; }
    h2 { color: #007BFF; font-size: 1.3em; }
    p { font-size: 1.4em; color: #222; font-family: monospace; font-weight: bold; background: #eee; padding: 15px; border-radius: 8px; }
  </style>
</head>
<body>
  <h1>Painel de Monitorização Wi-Fi</h1>
  <div class="card">
    <h2>Estado Atual do Passo (Bobinas)</h2>
    <p id="motor">A ligar ao ESP32...</p>
  </div>

  <script>
    async function fetchData() {
      try {
        const response = await fetch('/data', { cache: 'no-store' });
        const data = await response.json();
        document.getElementById('motor').innerText = data.motor;
      } catch (err) {
        document.getElementById('motor').innerText = "Erro de conexão...";
      }
    }
    setInterval(fetchData, 400); // Atualiza no browser a cada 400ms
    fetchData();
  </script>
</body>
</html>
)rawliteral";

// ------------------- HANDLERS WI-FI -------------------
void handleRoot() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "text/html; charset=utf-8", index_html);
}

void handleData() {
  String json = "{\"motor\":\"" + currentMotor + "\"}";
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "application/json", json);
}

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  // Inicializar Pinos do Driver
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);
  stopMotors();

  // Conexão Wi-Fi
  Serial.print("A ligar ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi ligado!");
  Serial.print("Aceda a este IP no browser para monitorizar: ");
  Serial.println(WiFi.localIP());

  // Rotas do Servidor
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.begin();

  Serial.println("Sequência de teste iniciada.");
}

// ------------------- LOOP PRINCIPAL -------------------
void loop() {
  server.handleClient(); // Processa os pedidos Wi-Fi

  // Máquina de estados baseada rigorosamente na tua tabela (Cada estado = 5 segundos)
  if (millis() - lastMotorTime >= motorInterval) {
    lastMotorTime = millis();
    
    switch (motorState) {
      case 0: // ESTADO A: B1 = OFF, B2 = OFF
        setMotorA(0);
        setMotorB(0);
        currentMotor = "A -> B1: OFF | B2: OFF";
        motorState = 1;
        break;

      case 1: // ESTADO B: B1 = +, B2 = OFF
        setMotorA(200);
        setMotorB(0);
        currentMotor = "B -> B1:   +  | B2: OFF";
        motorState = 2;
        break;

      case 2: // ESTADO C: B1 = OFF, B2 = -
        setMotorA(0);
        setMotorB(-200);
        currentMotor = "C -> B1: OFF | B2:   -";
        motorState = 3;
        break;

      case 3: // ESTADO D: B1 = -, B2 = OFF
        setMotorA(-200);
        setMotorB(0);
        currentMotor = "D -> B1:   -  | B2: OFF";
        motorState = 4;
        break;

      case 4: // ESTADO E: B1 = OFF, B2 = +
        setMotorA(0);
        setMotorB(200);
        currentMotor = "E -> B1: OFF | B2:   +";
        motorState = 0; // Volta a fechar o ciclo no Estado A
        break;
    }
    
    // Imprime também no Monitor Serial para validação imediata
    Serial.println("[ESTADO ATUAL] " + currentMotor);
  }
}

// ------------------- FUNÇÕES DOS MOTORES / BOBINAS -------------------
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