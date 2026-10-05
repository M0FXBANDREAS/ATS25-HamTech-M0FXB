#include <Arduino.h>
#include <WiFi.h>
#include <esp_chip_info.h>

static void banner() {
  esp_chip_info_t info{};
  esp_chip_info(&info);
  Serial.println();
  Serial.println("========================================");
  Serial.println(" ATS-25 HamTech M0FXB Controller");
  Serial.println(" V0.3 FACTORY-LAYOUT HARDWARE PROBE");
  Serial.println("========================================");
  Serial.printf("Cores: %d | silicon rev: %d\n", info.cores, info.revision);
  Serial.printf("CPU: %u MHz | flash: %u bytes\n", ESP.getCpuFreqMHz(), ESP.getFlashChipSize());
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Wi-Fi MAC: %s\n", WiFi.macAddress().c_str());
  Serial.println("Partition target: factory-compatible ATS25 MAX-Decoder 4MB map");
  Serial.println("GPIO policy: TFT/touch/SI473x/encoder/audio pins NOT driven.");
  Serial.println("V0.3 STATUS: SAFE SERIAL PROBE RUNNING");
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  WiFi.mode(WIFI_STA);
  banner();
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 5000) {
    last = millis();
    Serial.printf("V0.3 alive: %lu s | heap %u\n", millis()/1000UL, ESP.getFreeHeap());
  }
  delay(10);
}
