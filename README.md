# ATS-25 HamTech M0FXB Controller V1.7.1 BETA

Stability fix for V1.7 spectrum flashing.

The V1.7 scope performed a 52-point blocking sweep and redrew the whole lower panel every 1.5 seconds.
V1.7.1 replaces that with an incremental sweep:
- one RSSI sample approximately every 25 ms
- centre frequency restored after every individual sample
- spectrum frame redrawn only after a completed sweep
- no periodic clearing of the whole footer
- AM/FM/LSB/USB, BFO, filters, bands and VFO A/B retained

The SI4735 remains a swept receiver, not a wideband I/Q SDR, so individual measurements still require
brief retuning. This revision minimizes the disruption rather than pretending it is instantaneous.
