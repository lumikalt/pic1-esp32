#include <Wire.h>
#define MPU6050_ADDR 0x68

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();

  Serial.println("MPU6050 ready");
}

void loop() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 6, true);

  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();

  Serial.print("ax: "); Serial.print(ax);
  Serial.print("  ay: "); Serial.print(ay);
  Serial.print("  az: "); Serial.println(az);

  delay(200);
}