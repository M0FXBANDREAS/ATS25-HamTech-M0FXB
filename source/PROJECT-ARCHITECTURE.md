# Firmware modules

- board/: confirmed MAX-DECODER hardware abstraction
- radio/: SI473x control and SSB patch integration
- ui/: original 320x240 HamTech UI, keypad, bands, meter, spectrum/waterfall
- audio/: decoder sample acquisition and Bluetooth audio bridge
- decoders/: FT8, CW, RTTY
- net/: Wi-Fi provisioning, HTTP server, WebSocket state feed
- bt/: discovery, pairing, A2DP source
- storage/: NVS settings/memories
- diagnostics/: hardware test and USB serial logging

The factory flash image is recovery/reference material only and is not linked into this project.
