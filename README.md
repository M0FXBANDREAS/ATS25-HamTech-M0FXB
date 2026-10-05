# ATS-25 HamTech M0FXB Controller — V0.5
Display/backlight correction only. V0.4 proved the ESP32 firmware stable but the panel changed white to black. The public ATS-25X2 hardware notes show GPIO14 driving a PNP BC557 backlight stage, so LOW is the expected ON state. V0.5 keeps GPIO14 LOW after TFT initialization.

No touch, SI473x, encoder, audio, Wi-Fi, or front-end control is enabled in this build.
