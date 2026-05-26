#include "WiFi.h"


void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println();
  Serial.println("A procurar redes WiFi...");

  int n = WiFi.scanNetworks();

  if (n == 0) {
    Serial.println("Nenhuma rede encontrada");
  } else {

    Serial.println("Redes encontradas:");
    Serial.println("-------------------");

    for (int i = 0; i < n; i++) {

      Serial.print(i + 1);
      Serial.print(" -> ");

      Serial.print(WiFi.SSID(i));

      Serial.print(" | Canal: ");
      Serial.print(WiFi.channel(i));

      Serial.print(" | RSSI: ");
      Serial.print(WiFi.RSSI(i));

      Serial.print(" dBm");

      Serial.println();
    }
  }
}

void loop() {
}