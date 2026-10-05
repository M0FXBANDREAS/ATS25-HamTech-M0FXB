#pragma once
// MAX-DECODER GPIO MAP - MUST BE CONFIRMED FROM THE ACTUAL PCB BEFORE FLASHING.
// This file intentionally contains no guessed GPIO values.
struct HamTechBoardPins {
  int i2c_sda=-1, i2c_scl=-1, si473x_reset=-1;
  int enc_a=-1, enc_b=-1, enc_push=-1;
  int tft_cs=-1, tft_dc=-1, tft_rst=-1, tft_sclk=-1, tft_mosi=-1, tft_miso=-1;
  int touch_cs=-1, touch_irq=-1;
  int backlight=-1, battery_adc=-1;
  int decoder_audio_adc=-1;
};
