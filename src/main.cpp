#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SI4735.h>
#include <patch_ssb_compressed.h>

// ATS-25 HamTech M0FXB Controller V1.3 BETA
// Working V1.1 direct ILI9341 driver retained. Adds real PU2CLR SSB patch, USB/LSB, volume mode, branding and segmented S-meter.
static constexpr int TFT_SCLK=18,TFT_MOSI=23,TFT_MISO=19,TFT_CS=15,TFT_DC=2,TFT_RST=4,TFT_LED=14;
static constexpr int RX_RST=12,I2C_SDA=21,I2C_SCL=22;
static constexpr int ENC_A=17,ENC_B=16,ENC_SW=33;
SPIClass lcd(VSPI); SI4735 rx;

enum RadioMode : uint8_t { MODE_AM, MODE_FM, MODE_LSB, MODE_USB };
RadioMode mode=MODE_AM;
bool ssbLoaded=false, volumeMode=false;
const uint16_t size_content=sizeof ssb_patch_content;
const uint16_t cmd_0x15_size=sizeof cmd_0x15;
uint16_t amFreq=7100, fmFreq=10000;
uint16_t amStep=1, fmStep=10; // AM kHz; FM 10 = 100 kHz in SI4735 units
uint8_t volume=35,rssi=0,snr=0;
volatile int encDelta=0;
uint32_t pressStart=0; bool pressHandled=false;

static const uint16_t BLACK=0x0000,WHITE=0xFFFF,BLUE=0x001F,CYAN=0x07FF,GREEN=0x07E0,YELLOW=0xFFE0,RED=0xF800,DGREY=0x4208;

static void sel(bool v){digitalWrite(TFT_CS,v?LOW:HIGH);}
static void cmd(uint8_t c){digitalWrite(TFT_DC,LOW);sel(1);lcd.transfer(c);sel(0);}
static void d8(uint8_t d){digitalWrite(TFT_DC,HIGH);sel(1);lcd.transfer(d);sel(0);}
static void d16(uint16_t d){d8(d>>8);d8(d);}
static void addr(int x0,int y0,int x1,int y1){cmd(0x2A);d16(x0);d16(x1);cmd(0x2B);d16(y0);d16(y1);cmd(0x2C);}
static void rect(int x,int y,int w,int h,uint16_t c){if(w<=0||h<=0)return;addr(x,y,x+w-1,y+h-1);digitalWrite(TFT_DC,HIGH);sel(1);for(uint32_t i=0;i<(uint32_t)w*h;i++)lcd.transfer16(c);sel(0);}

static void initLCD(){
  pinMode(TFT_CS,OUTPUT);pinMode(TFT_DC,OUTPUT);pinMode(TFT_RST,OUTPUT);pinMode(TFT_LED,OUTPUT);
  digitalWrite(TFT_CS,HIGH);digitalWrite(TFT_LED,LOW);digitalWrite(TFT_RST,HIGH);delay(10);
  digitalWrite(TFT_RST,LOW);delay(30);digitalWrite(TFT_RST,HIGH);delay(150);
  lcd.begin(TFT_SCLK,TFT_MISO,TFT_MOSI,TFT_CS);lcd.beginTransaction(SPISettings(20000000,MSBFIRST,SPI_MODE0));
  cmd(0x01);delay(120);cmd(0x28);cmd(0x3A);d8(0x55);cmd(0x36);d8(0x28);cmd(0x11);delay(120);cmd(0x29);delay(20);
  digitalWrite(TFT_LED,LOW);
}

// 5x7 dependency-free font: digits + selected letters.
struct Glyph { char c; uint8_t r[7]; };
static const Glyph font[]={
{'0',{14,17,19,21,25,17,14}},{'1',{4,12,4,4,4,4,14}},{'2',{14,17,1,2,4,8,31}},
{'3',{30,1,1,14,1,1,30}},{'4',{2,6,10,18,31,2,2}},{'5',{31,16,16,30,1,1,30}},
{'6',{14,16,16,30,17,17,14}},{'7',{31,1,2,4,8,8,8}},{'8',{14,17,17,14,17,17,14}},
{'9',{14,17,17,15,1,1,14}},{'.',{0,0,0,0,0,12,12}},{':',{0,4,4,0,4,4,0}},
{'A',{14,17,17,31,17,17,17}},{'B',{30,17,17,30,17,17,30}},{'E',{31,16,16,30,16,16,31}},
{'F',{31,16,16,30,16,16,16}},{'H',{17,17,17,31,17,17,17}},{'I',{14,4,4,4,4,4,14}},
{'K',{17,18,20,24,20,18,17}},{'M',{17,27,21,21,17,17,17}},{'N',{17,25,21,19,17,17,17}},
{'R',{30,17,17,30,20,18,17}},{'S',{15,16,16,14,1,1,30}},{'T',{31,4,4,4,4,4,4}},
{'V',{17,17,17,17,17,10,4}},{'X',{17,17,10,4,10,17,17}},{'Z',{31,1,2,4,8,16,31}},
{' ',{0,0,0,0,0,0,0}}
};
static const uint8_t* glyph(char c){for(auto &g:font)if(g.c==c)return g.r;return font[sizeof(font)/sizeof(font[0])-1].r;}
static void chr(int x,int y,char c,int s,uint16_t col){auto r=glyph(c);for(int yy=0;yy<7;yy++)for(int xx=0;xx<5;xx++)if(r[yy]&(1<<(4-xx)))rect(x+xx*s,y+yy*s,s,s,col);}
static int textWidth(const char* s,int sc){return strlen(s)*6*sc-sc;}
static void text(int x,int y,const char* s,int sc,uint16_t col){while(*s){chr(x,y,*s++,sc,col);x+=6*sc;}}
static void centered(int y,const char* s,int sc,uint16_t col){text((320-textWidth(s,sc))/2,y,s,sc,col);}

static uint16_t curFreq(){return mode==MODE_FM?fmFreq:amFreq;}
static const char* modeName(){return mode==MODE_FM?"FM":mode==MODE_AM?"AM":mode==MODE_LSB?"LSB":"USB";}
static void drawHeader(){
  rect(0,0,320,62,BLUE);
  char b[24];
  if(mode==MODE_FM) snprintf(b,sizeof(b),"%u.%u",fmFreq/100,(fmFreq%100)/10);
  else if(amFreq>=1000) snprintf(b,sizeof(b),"%u.%03u",amFreq/1000,amFreq%1000);
  else snprintf(b,sizeof(b),"%u",amFreq);
  int sc=(strlen(b)<=6)?6:5; centered(8,b,sc,WHITE);
}
static void drawGlobe(int cx,int cy){
  // Small original pixel globe: cyan ocean outline, green land marks.
  for(int y=-9;y<=9;y++)for(int x=-9;x<=9;x++){int q=x*x+y*y;if(q>=64&&q<=88)rect(cx+x,cy+y,1,1,CYAN);}
  rect(cx-1,cy-8,2,17,CYAN); rect(cx-7,cy-1,15,2,CYAN);
  rect(cx-5,cy-5,4,3,GREEN);rect(cx+2,cy-3,4,4,GREEN);rect(cx-2,cy+3,5,3,GREEN);
}
static void drawStatus(){
  rect(0,62,320,44,BLACK);
  char a[40],b[40];
  if(mode==MODE_FM){snprintf(a,sizeof(a),"FM STEP %u KHZ",fmStep*10);snprintf(b,sizeof(b),"VOL %u",volume);}
  else {snprintf(a,sizeof(a),"%s STEP %u KHZ",modeName(),amStep);snprintf(b,sizeof(b),"VOL %u%s",volume,volumeMode?" ADJUST":"");}
  text(6,66,a,2,WHITE); text(6,88,b,1,volumeMode?YELLOW:CYAN);
}
static void drawMeter(){
  rect(0,106,320,55,BLACK); text(6,108,"S 1 3 5 7 9 +20 +40",1,WHITE);
  const int n=24,gap=2,sw=11,x0=7,y=124,h=18;
  int lit=map(constrain(rssi,0,80),0,80,0,n);
  for(int i=0;i<n;i++){
    uint16_t c=i<14?GREEN:(i<20?YELLOW:RED);
    rect(x0+i*(sw+gap),y,sw,h,i<lit?c:DGREY);
  }
  char b[32];snprintf(b,sizeof(b),"RSSI %u  SNR %u",rssi,snr);text(7,146,b,1,WHITE);
}
static void drawFooter(){
  rect(0,161,320,79,BLACK);
  drawGlobe(18,178); text(34,169,"HAMTECH M0FXB",2,CYAN);
  text(6,194,"PRESS STEP",1,WHITE);
  text(112,194,"DOUBLE VOL",1,YELLOW);
  text(224,194,"HOLD MODE",1,GREEN);
  text(6,214,"AM FM LSB USB",2,WHITE);
}
static void drawUI(){drawHeader();drawStatus();drawMeter();drawFooter();}

void IRAM_ATTR encISR(){
 static uint8_t old=0;uint8_t n=(digitalRead(ENC_A)<<1)|digitalRead(ENC_B);uint8_t q=(old<<2)|n;
 if(q==0xD||q==4||q==2||q==0xB)encDelta++; if(q==0xE||q==7||q==1||q==8)encDelta--; old=n;
}
static void loadSSB(){
  rx.reset(); rx.queryLibraryId(); rx.patchPowerUp(); delay(50);
  rx.setI2CFastModeCustom(500000);
  rx.downloadCompressedPatch(ssb_patch_content,size_content,cmd_0x15,cmd_0x15_size);
  rx.setSSBConfig(1,1,0,1,0,1); // 2.2 kHz, AVC on, AFC disabled for SSB
  rx.setI2CStandardMode(); ssbLoaded=true;
}
static void applyBand(){
 if(mode==MODE_FM){ssbLoaded=false;rx.setFM(6400,10800,fmFreq,fmStep);}
 else if(mode==MODE_AM){ssbLoaded=false;rx.setAM(150,30000,amFreq,amStep);rx.setBandwidth(2,1);}
 else {
   if(!ssbLoaded) loadSSB();
   rx.setSSB(520,30000,amFreq,amStep,mode==MODE_USB?2:1);
   rx.setSSBAutomaticVolumeControl(1); rx.setSsbSoftMuteMaxAttenuation(0); rx.setSSBBfo(0);
 }
 rx.setVolume(volume);
}
static void tune(int dir){
 if(volumeMode){
   int v=constrain((int)volume+dir,0,63);volume=v;rx.setVolume(volume);drawStatus();return;
 }
 if(mode==MODE_FM){fmFreq=constrain((int)fmFreq+dir*fmStep,6400,10800);}
 else {amFreq=constrain((int)amFreq+dir*amStep,mode==MODE_AM?150:520,30000);}
 rx.setFrequency(curFreq()); drawHeader();
}
static void shortPress(){
 if(volumeMode){volumeMode=false;drawStatus();return;}
 if(mode==MODE_FM) fmStep=(fmStep==1?10:1);
 else amStep=(amStep==1?5:amStep==5?9:amStep==9?10:amStep==10?100:1);
 rx.setFrequencyStep(mode==MODE_FM?fmStep:amStep);drawStatus();
}
static void toggleVolume(){volumeMode=!volumeMode;drawStatus();}
static void nextMode(){
 volumeMode=false;
 if(mode==MODE_AM)mode=MODE_FM; else if(mode==MODE_FM)mode=MODE_LSB; else if(mode==MODE_LSB)mode=MODE_USB; else mode=MODE_AM;
 applyBand();drawUI();Serial.printf("MODE %s FREQ %u\n",modeName(),curFreq());
}

void setup(){
 Serial.begin(115200);delay(300);Serial.println("ATS-25 HamTech M0FXB Controller V1.3 BETA");
 initLCD();rect(0,0,320,240,BLACK);
 pinMode(ENC_A,INPUT_PULLUP);pinMode(ENC_B,INPUT_PULLUP);pinMode(ENC_SW,INPUT_PULLUP);
 attachInterrupt(ENC_A,encISR,CHANGE);attachInterrupt(ENC_B,encISR,CHANGE);
 Wire.begin(I2C_SDA,I2C_SCL);rx.setI2CFastModeCustom(100000);
 uint8_t addr=rx.getDeviceI2CAddress(RX_RST);Serial.printf("SI47XX I2C address: 0x%02X\n",addr);
 rx.setup(RX_RST,0);applyBand();drawUI();
 Serial.println("V1.2 BETA READY");
}
void loop(){
 int d;noInterrupts();d=encDelta;encDelta=0;interrupts();if(d)tune(d>0?1:-1);
 static bool lastDown=false; static uint32_t downAt=0,lastClick=0; static bool longDone=false;
 bool down=digitalRead(ENC_SW)==LOW;
 if(down&&!lastDown){downAt=millis();longDone=false;}
 if(down&&!longDone&&millis()-downAt>=900){longDone=true;nextMode();}
 if(!down&&lastDown&&!longDone){
   uint32_t now=millis();
   if(now-lastClick<420){lastClick=0;toggleVolume();}
   else lastClick=now;
 }
 if(lastClick && millis()-lastClick>=420){lastClick=0;shortPress();}
 lastDown=down;
 static uint32_t t=0;if(millis()-t>500){t=millis();rx.getCurrentReceivedSignalQuality();rssi=rx.getCurrentRSSI();snr=rx.getCurrentSNR();drawMeter();Serial.printf("V1.3 %s %u VOL %u RSSI %u SNR %u heap %u\n",modeName(),curFreq(),volume,rssi,snr,ESP.getFreeHeap());}
}
