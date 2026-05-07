#include "MPU6050_tockn.h"
#include <Wire.h>

MPU6050 mpu6050(Wire);

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  mpu6050.begin();
  
  // Calibração: O sensor deve estar perfeitamente parado nesta fase
  Serial.println("A calcular offsets... não mexas no sensor!");
  mpu6050.calcGyroOffsets(true);
  Serial.println("Calibração concluída!");
  Serial.println("----------------------------------------------");
}

void loop() {
  mpu6050.update();

  // --- Ângulos YPR (Yaw, Pitch, Roll) ---
  // Nota: O Yaw (Z) num IMU sem magnetómetro tende a sofrer "drift" com o tempo.
  float roll  = mpu6050.getAngleX();
  float pitch = mpu6050.getAngleY();
  float yaw   = mpu6050.getAngleZ();

  // --- Aceleração (em G's) ---
  float accX = mpu6050.getAccX();
  float accY = mpu6050.getAccY();
  float accZ = mpu6050.getAccZ();

  // Impressão dos dados
  Serial.print("ANGULOS -> Roll: "); Serial.print(roll);
  Serial.print(" | Pitch: "); Serial.print(pitch);
  Serial.print(" | Yaw: "); Serial.println(yaw);

  Serial.print("ACCEL   -> X: "); Serial.print(accX);
  Serial.print(" | Y: "); Serial.print(accY);
  Serial.print(" | Z: "); Serial.println(accZ);
  
  Serial.println("----------------------------------------------");
  
  delay(200); // Ajusta o delay conforme a tua necessidade de velocidade
}