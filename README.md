# ATS-25 HamTech M0FXB Controller — V0.3

Factory-layout hardware probe for the classic ESP32-WROOM ATS25 MAX-Decoder.

## Safety scope
This build deliberately does not drive unverified TFT, XPT2046 touch, SI473x, rotary encoder, antenna switching, battery ADC or audio GPIOs. It provides USB serial diagnostics at 115200 baud.

## Factory-compatible 4 MB partition target
- NVS 0x9000 / 0x5000
- OTA data 0xE000 / 0x2000
- APP0 0x10000 / 0x240000
- SPIFFS 0x250000 / 0x160000
- COREDUMP 0x3B0000 / 0x10000

Keep the original 4,194,304-byte factory backup private and safe. Do not commit it.
