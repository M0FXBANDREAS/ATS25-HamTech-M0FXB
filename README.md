# ATS-25 HamTech M0FXB Controller V1.7 BETA

Adds a REAL SI4735 swept-RSSI spectrum/waterfall display to the proven V1.6 receiver.

Important: SI4735 is not a wideband I/Q SDR. The scope deliberately retunes through points around
the centre frequency, measures RSSI, restores the centre frequency, and therefore briefly interrupts
normal reception while sweeping. It is real measured RF signal strength, not decorative/fake FFT data.

- 52-point swept RSSI scope
- waterfall intensity strip
- red centre marker
- selectable +/-5 to +/-50 kHz scope span
- AM/FM/LSB/USB retained
- bands, VFO A/B, BFO and filters retained
- NVS settings retained

Touch is still held for the next revision so the exact panel calibration can be proven separately.
