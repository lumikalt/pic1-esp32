#include <Wire.h>
#include "qmc5883p.h"
#include "MPU6050_tockn.h"

// Instanciar os objetos
QMC5883P mag;
MPU6050 mpu6050(Wire);

void setup() {
  Serial.begin(115200);

  // Iniciar I2C (A4/A5 no Uno)
  Wire.begin();
  Wire.setClock(100000);

  // Inicializar Magnetómetro
  if (!mag.begin()) {
    Serial.println("QMC5883P initialization failed!");
    while (true) delay(1000);
  }

  // Inicializar MPU6050
  mpu6050.begin();
  Serial.println("A calcular offsets... não mexas no sensor!");
  mpu6050.calcGyroOffsets(true);
  
  Serial.println("Sensores prontos.");
  Serial.println("----------------------------------------------");
}

void loop() {
  // --- Leitura do MPU6050 ---
  mpu6050.update();
  float roll  = mpu6050.getAngleX();
  float pitch = mpu6050.getAngleY();
  float yaw   = mpu6050.getAngleZ();

  // --- Leitura do Magnetómetro (Exatamente como o original) ---
  float xyz[3];
  if (mag.readXYZ(xyz)) {
    float heading = mag.getHeadingDeg(2.5); // Declinação magnética

    // Print do Magnetómetro em uT
    Serial.print("X:");
    Serial.print(xyz[0], 2); // 2 casas decimais
    Serial.print("  Y:");
    Serial.print(xyz[1], 2);
    Serial.print("  Z:");
    Serial.print(xyz[2], 2);
    Serial.print(" uT  |  Heading: ");
    Serial.print(heading, 0);
    Serial.println(" deg");
  }

  // --- Print dos ângulos e aceleração do MPU6050 ---
  Serial.print("ANGULOS -> Roll: "); Serial.print(roll);
  Serial.print(" | Pitch: "); Serial.print(pitch);
  Serial.print(" | Yaw: "); Serial.println(yaw);

  Serial.print("ACCEL   -> X: "); Serial.print(mpu6050.getAccX());
  Serial.print(" | Y: "); Serial.print(mpu6050.getAccY());
  Serial.print(" | Z: "); Serial.println(mpu6050.getAccZ());
  
  Serial.println("----------------------------------------------");

  delay(1000); 
}