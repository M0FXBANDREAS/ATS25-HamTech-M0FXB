# ATS-25 HamTech M0FXB Controller

Clean-room firmware project for the ATS-25 max-Decoder hardware owned/tested by M0FXB.

## Current build: V0.2 SAFE HARDWARE PROBE

This is deliberately **not yet the receiver UI**. It validates that GitHub Actions can build a genuine ESP32-WROOM binary and that the USB flash path works on the target before any uncertain peripheral GPIO is driven.

The probe uses only ESP32 internal functions plus Wi-Fi station mode and USB serial. It does **not** drive TFT, touch, SI473x, VFO/encoder, audio, antenna-switching or other unknown GPIO.

### Build on GitHub
Upload this repository preserving the `.github/workflows` folder. Open **Actions -> Build ATS25 HamTech Firmware -> Run workflow**. When green, download the artifact named `ATS25-HamTech-M0FXB-V0.2-SAFE-PROBE`.

### Important
Keep the existing 4,194,304-byte factory backup safe. Do not publish or commit the factory backup to this repository.

The next hardware-test release will add the display/touch/VFO/SI473x only after their exact MAX-Decoder pin mapping is verified.
