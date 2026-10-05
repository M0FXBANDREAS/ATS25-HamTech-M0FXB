# ATS-25 HamTech M0FXB Controller — V0.4 TFT HARDWARE TEST

V0.4 is a deliberately limited display bring-up build for the classic ESP32-WROOM ATS-25 family.

It drives only the candidate ILI9341 display interface: SCLK 18, MOSI 23, MISO 19, CS 15, DC 2, RESET 4, backlight 14.
Touch, SI473x, encoder, audio and RF/front-end GPIO are not initialized.

Expected display: black background with blue header/footer, a white separator, and four large RED / GREEN / BLUE / WHITE vertical bars.
Serial output at 115200 contains `V0.4 TFT HARDWARE TEST` and an alive message every five seconds.

Factory-compatible 4 MB partition target is retained. Keep the original factory backup private and do not commit it.
