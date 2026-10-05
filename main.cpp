#include <Arduino.h>
#include <SPI.h>

// V0.5 ATS-25 2.8-inch ILI9341 display/backlight diagnostic.
// ONLY TFT-related pins are intentionally driven in this build.
static constexpr int TFT_SCLK = 18;
static constexpr int TFT_MOSI = 23;
static constexpr int TFT_MISO = 19;
static constexpr int TFT_CS   = 15;
static constexpr int TFT_DC   = 2;
static constexpr int TFT_RST  = 4;
static constexpr int TFT_LED  = 14;
SPIClass tftSPI(VSPI);
static void sel(bool on){ digitalWrite(TFT_CS,on?LOW:HIGH); }
static void cmd(uint8_t c){ digitalWrite(TFT_DC,LOW); sel(true); tftSPI.transfer(c); sel(false); }
static void d8(uint8_t d){ digitalWrite(TFT_DC,HIGH); sel(true); tftSPI.transfer(d); sel(false); }
static void d16(uint16_t d){ d8(d>>8); d8(d&0xff); }
static void addr(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1){ cmd(0x2A);d16(x0);d16(x1);cmd(0x2B);d16(y0);d16(y1);cmd(0x2C); }
static void rect(int x,int y,int w,int h,uint16_t c){ addr(x,y,x+w-1,y+h-1);digitalWrite(TFT_DC,HIGH);sel(true);for(uint32_t i=0;i<(uint32_t)w*h;i++)tftSPI.transfer16(c);sel(false); }
static void initTFT(){
 pinMode(TFT_CS,OUTPUT);pinMode(TFT_DC,OUTPUT);pinMode(TFT_RST,OUTPUT);pinMode(TFT_LED,OUTPUT);
 digitalWrite(TFT_CS,HIGH); digitalWrite(TFT_LED,LOW); // candidate PNP stage: LOW = backlight ON
 digitalWrite(TFT_RST,HIGH);delay(10);digitalWrite(TFT_RST,LOW);delay(20);digitalWrite(TFT_RST,HIGH);delay(150);
 tftSPI.begin(TFT_SCLK,TFT_MISO,TFT_MOSI,TFT_CS); tftSPI.beginTransaction(SPISettings(20000000,MSBFIRST,SPI_MODE0));
 cmd(0x01);delay(120);cmd(0x28);cmd(0x3A);d8(0x55);cmd(0x36);d8(0x28);cmd(0x11);delay(120);cmd(0x29);delay(20);
 digitalWrite(TFT_LED,LOW);
}
void setup(){
 Serial.begin(115200);delay(1000);
 Serial.println("ATS-25 HamTech M0FXB Controller");
 Serial.println("V0.5 DISPLAY BACKLIGHT FIX");
 Serial.println("Candidate ILI9341: SCLK18 MOSI23 MISO19 CS15 DC2 RST4 LED14 LOW=ON");
 Serial.println("Touch/SI473x/encoder/audio/front-end GPIO are NOT initialized.");
 initTFT();
 rect(0,0,320,240,0x0000); rect(0,0,320,36,0x001F); rect(0,48,320,12,0xFFFF);
 rect(0,76,80,120,0xF800); rect(80,76,80,120,0x07E0); rect(160,76,80,120,0x001F); rect(240,76,80,120,0xFFFF);
 rect(0,212,320,28,0x001F);
 Serial.println("V0.5 DISPLAY PATTERN DRAWN");
}
void loop(){static uint32_t last=0;if(millis()-last>=5000){last=millis();Serial.printf("V0.5 alive: %lu s | heap %u\n",millis()/1000UL,ESP.getFreeHeap());}delay(10);}
