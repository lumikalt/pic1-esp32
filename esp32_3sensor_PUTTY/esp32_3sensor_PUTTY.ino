#include <Wire.h>
#include <WiFi.h>
#include "qmc5883p.h"
#include "MPU6050_tockn.h"

// --- Wi-Fi Settings ---
const char* ssid = "RAMLX131E";       // Change this to your network name
const char* password = "Ssgnr2018#"; // Change this to your network password

// Create a TCP server on port 8080
WiFiServer server(8080);
WiFiClient client;

// Define I2C pins for the ESP32-S3 Mini
#define I2C_SDA 8
#define I2C_SCL 9

// --- DRV8833 PWM Pins ---
// Coil 1 (Outputs 1 + 2)
#define MOTOR_A_IN1 5
#define MOTOR_A_IN2 6

// Coil 2 (Outputs 3 + 4)
#define MOTOR_B_IN3 7
#define MOTOR_B_IN4 10

QMC5883P mag;
MPU6050 mpu6050(Wire);

// --- Non-blocking Timer Variables ---
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 1000; // Print data every 1000ms

// --- Non-blocking Coil/Motor Demo Variables ---
unsigned long lastMotorTime = 0;
const unsigned long motorInterval = 4000; // Change states every 4 seconds
int motorState = 0;

// --- Helper function to send data to both Serial and Wi-Fi ---
void printlnBoth(String msg) {
  Serial.println(msg);
  if (client && client.connected()) {
    client.println(msg);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500); 

  // --- Initialize Wi-Fi ---
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Start the TCP server
  server.begin();
  Serial.println("TCP Server started on port 8080.");
  Serial.println("----------------------------------------------");

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  if (!mag.begin()) {
    Serial.println("QMC5883P initialization failed!");
    while (true) delay(1000);
  }

  mpu6050.begin();
  Serial.println("A calcular offsets... nao mexas no sensor!");
  mpu6050.calcGyroOffsets(true);
  
  // --- Initialize DRV8833 Control Pins ---
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);
  
  // Stop everything initially
  stopMotors();

  Serial.println("Sensores e Coils/Motores Prontos.");
  Serial.println("----------------------------------------------");
}

void loop() {
  // --- Handle Incoming Wi-Fi Connections ---
  if (server.hasClient()) {
    if (!client || !client.connected()) {
      if (client) client.stop(); // Stop previous disconnected client
      client = server.available(); // Accept new client
      Serial.println(">>> New Wi-Fi Client Connected!");
    } else {
      // Reject new clients if one is already connected
      server.available().stop();
    }
  }

  // --- Constant Sensor Updates ---
  mpu6050.update();
  float roll  = mpu6050.getAngleX();
  float pitch = mpu6050.getAngleY();
  float yaw   = mpu6050.getAngleZ();

  // --- Non-blocking Coil Control Example ---
  if (millis() - lastMotorTime >= motorInterval) {
    lastMotorTime = millis();
    
    switch (motorState) {
      case 0: // Drive Forward at ~78% duty cycle (200/255)
        setMotorA(200);
        setMotorB(200);
        printlnBoth(">>> COILS: Forward / Active Positive");
        motorState = 1;
        break;
      case 1: // Coast / Off
        stopMotors();
        printlnBoth(">>> COILS: Stopped / Off");
        motorState = 2;
        break;
      case 2: // Drive Reverse at ~60% duty cycle (150/255)
        setMotorA(-150);
        setMotorB(-150);
        printlnBoth(">>> COILS: Reverse / Active Negative");
        motorState = 3;
        break;
      case 3: // Coast / Off
        stopMotors();
        printlnBoth(">>> COILS: Stopped / Off");
        motorState = 0;
        break;
    }
  }

  // --- Non-blocking Serial Print ---
  if (millis() - lastPrintTime >= printInterval) {
    lastPrintTime = millis();

    // --- Magnetometer Reading ---
    float xyz[3];
    if (mag.readXYZ(xyz)) {
      float heading = mag.getHeadingDeg(2.5); 
      
      // Combine into a single string for easy transmission
      String magMsg = "X:" + String(xyz[0], 2) + "  Y:" + String(xyz[1], 2) + 
                      "  Z:" + String(xyz[2], 2) + " uT  |  Heading: " + String(heading, 0) + " deg";
      printlnBoth(magMsg);
    }

    // --- MPU6050 Reading ---
    String mpuMsg = "ANGULOS -> Roll: " + String(roll) + " | Pitch: " + String(pitch) + " | Yaw: " + String(yaw);
    printlnBoth(mpuMsg);
    printlnBoth("----------------------------------------------");
  }
}

// --- DRV8833 Helper Driving Functions ---

// Controls Coil 1 (Outputs 1+2) from -255 to 255
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

// Controls Coil 2 (Outputs 3+4) from -255 to 255
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

// Cuts power to both coils completely
void stopMotors() {
  analogWrite(MOTOR_A_IN1, 0);
  analogWrite(MOTOR_A_IN2, 0);
  analogWrite(MOTOR_B_IN3, 0);
  analogWrite(MOTOR_B_IN4, 0);
}