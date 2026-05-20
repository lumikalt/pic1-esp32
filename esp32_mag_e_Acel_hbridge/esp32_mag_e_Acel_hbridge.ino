#include <Wire.h>
#include "qmc5883p.h"
#include "MPU6050_tockn.h"

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

void setup() {
  Serial.begin(115200);
  delay(500); 

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
  // --- Constant Sensor Updates ---
  mpu6050.update();
  float roll  = mpu6050.getAngleX();
  float pitch = mpu6050.getAngleY();
  float yaw   = mpu6050.getAngleZ();

  // --- Non-blocking Coil Control Example ---
  // Demonstrates driving the coils smoothly without interrupting sensor readings
  if (millis() - lastMotorTime >= motorInterval) {
    lastMotorTime = millis();
    
    switch (motorState) {
      case 0: // Drive Forward at ~78% duty cycle (200/255)
        setMotorA(200);
        setMotorB(200);
        Serial.println(">>> COILS: Forward / Active Positive");
        motorState = 1;
        break;
      case 1: // Coast / Off
        stopMotors();
        Serial.println(">>> COILS: Stopped / Off");
        motorState = 2;
        break;
      case 2: // Drive Reverse at ~60% duty cycle (150/255)
        setMotorA(-150);
        setMotorB(-150);
        Serial.println(">>> COILS: Reverse / Active Negative");
        motorState = 3;
        break;
      case 3: // Coast / Off
        stopMotors();
        Serial.println(">>> COILS: Stopped / Off");
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

      Serial.print("X:");
      Serial.print(xyz[0], 2); 
      Serial.print("  Y:");
      Serial.print(xyz[1], 2);
      Serial.print("  Z:");
      Serial.print(xyz[2], 2);
      Serial.print(" uT  |  Heading: ");
      Serial.print(heading, 0);
      Serial.println(" deg");
    }

    // --- MPU6050 Reading ---
    Serial.print("ANGULOS -> Roll: "); Serial.print(roll);
    Serial.print(" | Pitch: "); Serial.print(pitch);
    Serial.print(" | Yaw: "); Serial.println(yaw);
    
    Serial.println("----------------------------------------------");
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