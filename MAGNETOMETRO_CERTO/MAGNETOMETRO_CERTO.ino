#include <Wire.h>
#include "qmc5883p.h"
QMC5883P mag;

void setup() {
  Serial.begin(115200);

  // Start I2C (Uno uses fixed pins A4/A5)
  Wire.begin();
  Wire.setClock(100000);

  if (!mag.begin()) {
    Serial.println("QMC5883P initialization failed!");
    while (true) delay(1000);
  }

  Serial.println("Sensor is ready.");
}

void loop() {
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

  delay(250);
}