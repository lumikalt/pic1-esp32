#include <Wire.h>
#include "qmc5883p.h"
#include "MPU6050_tockn.h"

// Define I2C pins for the ESP32-S3 Mini
// Note: Pins 8 (SDA) and 9 (SCL) are the defaults for the Lolin S3 Mini. 
// If your specific S3 Mini uses different pins, change them here.
#define I2C_SDA 8
#define I2C_SCL 9


QMC5883P mag;
MPU6050 mpu6050(Wire);

void setup() {
  Serial.begin(115200);

  // Iniciar I2C no ESP32-S3 usando os pinos definidos
  Wire.begin(I2C_SDA, I2C_SCL);
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

  // --- Leitura do Magnetómetro  ---
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