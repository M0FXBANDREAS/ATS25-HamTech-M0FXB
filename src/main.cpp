#include <Arduino.h>
#include <SPI.h>

// V0.4 candidate ATS-25 2.8-inch ILI9341 mapping.
// ONLY TFT-related pins are intentionally driven in this build.
static constexpr int TFT_SCLK = 18;
static constexpr int TFT_MOSI = 23;
static constexpr int TFT_MISO = 19; // configured for shared SPI bus; display reads are not used
static constexpr int TFT_CS   = 15;
static constexpr int TFT_DC   = 2;
static constexpr int TFT_RST  = 4;
static constexpr int TFT_LED  = 14;

SPIClass tftSPI(VSPI);

static void selectTft(bool on) { digitalWrite(TFT_CS, on ? LOW : HIGH); }
static void cmd(uint8_t c) {
  digitalWrite(TFT_DC, LOW); selectTft(true); tftSPI.transfer(c); selectTft(false);
}
static void data8(uint8_t d) {
  digitalWrite(TFT_DC, HIGH); selectTft(true); tftSPI.transfer(d); selectTft(false);
}
static void data16(uint16_t d) { data8(d >> 8); data8(d & 0xFF); }
static void setAddr(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1) {
  cmd(0x2A); data16(x0); data16(x1);
  cmd(0x2B); data16(y0); data16(y1);
  cmd(0x2C);
}
static void fill(uint16_t color) {
  setAddr(0,0,319,239);
  digitalWrite(TFT_DC,HIGH); selectTft(true);
  for (uint32_t i=0;i<320UL*240UL;i++) { tftSPI.transfer16(color); }
  selectTft(false);
}
static void rect(int x,int y,int w,int h,uint16_t c) {
  setAddr(x,y,x+w-1,y+h-1); digitalWrite(TFT_DC,HIGH); selectTft(true);
  for (uint32_t i=0;i<(uint32_t)w*h;i++) tftSPI.transfer16(c);
  selectTft(false);
}
static void initILI9341() {
  pinMode(TFT_CS,OUTPUT); pinMode(TFT_DC,OUTPUT); pinMode(TFT_RST,OUTPUT); pinMode(TFT_LED,OUTPUT);
  digitalWrite(TFT_CS,HIGH); digitalWrite(TFT_LED,LOW);
  digitalWrite(TFT_RST,HIGH); delay(10); digitalWrite(TFT_RST,LOW); delay(20); digitalWrite(TFT_RST,HIGH); delay(150);
  tftSPI.begin(TFT_SCLK,TFT_MISO,TFT_MOSI,TFT_CS);
  tftSPI.beginTransaction(SPISettings(20000000,MSBFIRST,SPI_MODE0));
  cmd(0x01); delay(120); // software reset
  cmd(0x28);             // display off
  cmd(0x3A); data8(0x55); // RGB565
  cmd(0x36); data8(0x28); // landscape candidate orientation, BGR
  cmd(0x11); delay(120); // sleep out
  cmd(0x29); delay(20);  // display on
  digitalWrite(TFT_LED,HIGH);
}

void setup() {
  Serial.begin(115200); delay(1000);
  Serial.println("ATS-25 HamTech M0FXB Controller");
  Serial.println("V0.4 TFT HARDWARE TEST");
  Serial.println("Candidate ILI9341 mapping: SCLK18 MOSI23 MISO19 CS15 DC2 RST4 LED14");
  Serial.println("Touch/SI473x/encoder/audio/front-end GPIO are NOT initialized.");
  initILI9341();
  // Unmistakable static color-bar diagnostic. No third-party graphics or copied UI.
  fill(0x0000);               // black
  rect(0,0,320,36,0x001F);    // blue header
  rect(0,48,320,12,0xFFFF);   // white separator
  rect(0,76,80,120,0xF800);   // red
  rect(80,76,80,120,0x07E0);  // green
  rect(160,76,80,120,0x001F); // blue
  rect(240,76,80,120,0xFFFF); // white
  rect(0,212,320,28,0x001F);  // blue footer
  Serial.println("V0.4 DISPLAY PATTERN DRAWN");
}

void loop() {
  static uint32_t last=0;
  if (millis()-last>=5000) { last=millis(); Serial.printf("V0.4 alive: %lu s | heap %u\n",millis()/1000UL,ESP.getFreeHeap()); }
  delay(10);
}
