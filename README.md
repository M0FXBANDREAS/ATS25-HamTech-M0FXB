# ATS-25 HamTech M0FXB Controller V1.0
Clean-room controller firmware for the ESP32-based ATS-25 MAX-Decoder project.

Implemented in this release: ILI9341 display UI, VFO encoder, AM/FM receiver control through SI4735, live RSSI/SNR S-meter, spectrum/waterfall display layer, volume, tuning step, Wi-Fi access point and browser status dashboard.

The SI4735 library is MIT licensed (Ricardo Lima Caratti / PU2CLR). This project does not contain the original ATS-25/Diamond firmware or artwork.

Hardware-specific SSB patch loading, touch-controller calibration and Bluetooth audio routing are not enabled because those depend on board/patch/audio-routing details not established by the available hardware evidence.
