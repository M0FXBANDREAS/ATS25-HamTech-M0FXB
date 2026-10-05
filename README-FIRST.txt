ATS-25 HamTech M0FXB Controller - V0.1 DEVELOPMENT PACKAGE
============================================================

Clean-room project for the classic ESP32 ATS-25 MAX-DECODER family.
Target confirmed from the owner's read-only backup:
  ESP32-D0WD rev 1.0, 240 MHz, 40 MHz crystal, 4 MB flash, CH340 USB serial.

IMPORTANT
---------
This V0.1 package is a DEVELOPMENT package, NOT YET A FLASHABLE RECEIVER IMAGE.
Do not flash guessed display/touch/audio GPIO assignments to a MAX-DECODER.
The exact MAX-DECODER PCB pin map must be confirmed first.

The factory backup is deliberately NOT included in this shareable package.

Planned V1.0 features
---------------------
* Original HamTech blue/white/red/green dark UI
* All receiver-supported bands + direct numeric frequency entry
* S-meter (S1-S9 and +dB) + RSSI/SNR
* Icom-inspired but original spectrum/waterfall presentation
* Fixed centre tuning cursor
* FT8, CW and RTTY decoder pages
* Wi-Fi setup and live IP web dashboard
* Bluetooth scan/pairing and A2DP audio output if the audio path supports it
* Wi-Fi screen/status sharing shortcut
* USB firmware update/recovery workflow

Next hardware milestone
-----------------------
Confirm MAX-DECODER TFT, touch, encoder, SI473x and decoder-audio GPIO wiring.
Once confirmed, fill source/board_max_decoder.h and compile the first hardware-test BIN.
