# ATS-25 HamTech M0FXB Controller V1.6 BETA

Builds on the proven V1.5 hardware/radio base.

Added:
- Receiver-style dark UI
- Amateur + broadcast band presets
- VFO A/B
- SSB BFO +/- 2 kHz in 25 Hz steps
- Existing real AM/FM/LSB/USB and SSB filters retained
- Persistent volume and AM/FM frequencies using ESP32 NVS
- Green/yellow/red segmented S-meter retained

Encoder press cycles: VFO, VOL, STEP, FILTER, MODE, BAND, BFO, VFO A/B.
Rotate changes the selected function.

Touch: layout is touch-ready, but touch input is intentionally NOT enabled in V1.6.
The public ATS-25-family pin evidence suggests shared SPI with touch CS GPIO5 and IRQ GPIO34,
but calibration/orientation is not yet proven on this exact MAX-Decoder unit.
