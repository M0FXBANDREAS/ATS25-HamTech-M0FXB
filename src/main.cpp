#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include <esp_chip_info.h>

static void printChipInfo() {
  esp_chip_info_t info;
  esp_chip_info(&info);
  Serial.println();
  Serial.println("========================================");
  Serial.println(" ATS-25 HamTech M0FXB Controller");
  Serial.println(" V0.2 SAFE HARDWARE PROBE");
  Serial.println("========================================");
  Serial.printf("ESP32 cores: %d\n", info.cores);
  Serial.printf("Silicon revision: %d\n", info.revision);
  Serial.printf("Flash size: %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("CPU frequency: %u MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Wi-Fi MAC: %s\n", WiFi.macAddress().c_str());
  Serial.println();
  Serial.println("SAFE PROBE STATUS: RUNNING");
  Serial.println("No TFT, touch, SI473x, encoder or audio GPIO is driven by this build.");
  Serial.println("This build exists to validate the clean build/flash pipeline first.");
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  WiFi.mode(WIFI_STA);
  printChipInfo();
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 5000) {
    last = millis();
    Serial.printf("HamTech probe alive: %lu s | heap %u\n", millis()/1000UL, ESP.getFreeHeap());
  }
  delay(10);
}
