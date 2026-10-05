# ATS-25 HamTech M0FXB Controller

## Visual language
Original HamTech interface. Dark background; blue panels and spectrum accents; white primary frequency/text; green receive/decoder success states; red warnings/peaks. No copied Icom or Diamond artwork.

## Main screen
Top shortcuts: BAND, FREQ, WIFI, BT, SCREEN, DECODER.
Large direct frequency display. Mode/BW/STEP/AGC controls. Permanent S-meter. Spectrum above a scrolling waterfall with fixed centre cursor.

## Bands and tuning
Expose every range supported by the installed SI473x/front-end. Amateur band shortcuts plus LW/MW/SW/FM where supported. Tapping FREQ opens a numeric keypad.

## Decoders
FT8, CW and RTTY are first-class pages. Decoder DSP must use an independently implemented or license-compatible audio path; no proprietary decoder code is copied from the factory image.

## Connectivity
Wi-Fi station mode with touchscreen setup. HTTP/WebSocket dashboard at the receiver IP. Bluetooth Classic A2DP source for compatible headphones/speakers, contingent on confirmed audio capture path. Wi-Fi and Bluetooth coexistence must be load-tested.

## Spectrum/waterfall
The SI473x is not an SDR I/Q source. Initial implementation is a swept-RSSI spectrum/waterfall, with sweep span/rate constrained by retune latency and listening interruption. UI styling may be inspired by modern transceivers but remains original.
