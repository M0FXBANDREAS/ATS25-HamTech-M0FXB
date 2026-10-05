#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Preferences.h>
#include <SI4735.h>
#include <patch_ssb_compressed.h>

// ATS-25 HamTech M0FXB Controller V1.7.1 BETA
// Working V1.1 direct ILI9341 driver retained. Non-blocking incremental RSSI spectrum, stable waterfall, bands, VFO A/B, BFO, filters and persistent settings.
// Touch-ready layout retained, but touch input is not enabled until exact calibration is proven.
static constexpr int TFT_SCLK=18,TFT_MOSI=23,TFT_MISO=19,TFT_CS=15,TFT_DC=2,TFT_RST=4,TFT_LED=14;
static constexpr int RX_RST=12,I2C_SDA=21,I2C_SCL=22;
static constexpr int ENC_A=17,ENC_B=16,ENC_SW=33;
SPIClass lcd(VSPI); SI4735 rx; Preferences prefs;

enum RadioMode : uint8_t { MODE_AM, MODE_FM, MODE_LSB, MODE_USB };
RadioMode mode=MODE_AM;
bool ssbLoaded=false;
enum Control : uint8_t { CTRL_VFO, CTRL_VOL, CTRL_STEP, CTRL_FILTER, CTRL_MODE, CTRL_BAND, CTRL_BFO, CTRL_VFOAB, CTRL_SCOPE };
Control control=CTRL_VFO;
uint8_t ssbBwPos=3; // 2.2 kHz default
int16_t bfo=0;
bool vfoB=false;
uint16_t vfoA=7100,vfoBfreq=14200;
struct Band { const char* name; uint16_t lo,hi,start; RadioMode pref; };
const Band bands[]={
 {"LW",150,520,198,MODE_AM},{"MW",520,1710,1000,MODE_AM},{"SW",1710,30000,6000,MODE_AM},
 {"160",1800,2000,1900,MODE_LSB},{"80",3500,4000,3700,MODE_LSB},{"60",5250,5450,5357,MODE_USB},
 {"40",7000,7300,7100,MODE_LSB},{"30",10100,10150,10120,MODE_USB},{"20",14000,14350,14200,MODE_USB},
 {"17",18068,18168,18100,MODE_USB},{"15",21000,21450,21200,MODE_USB},{"12",24890,24990,24940,MODE_USB},
 {"10",28000,29700,28500,MODE_USB},{"FM",6400,10800,10000,MODE_FM}
};
uint8_t bandPos=6;
bool scopeOn=true; uint8_t scopeSpan=20; // +/- kHz; swept RSSI, not SDR/IQ
const uint8_t ssbBwCode[6]={4,5,0,1,2,3};
const char* ssbBwName[6]={"0.5","1.0","1.2","2.2","3.0","4.0"};
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
static void panel(int x,int y,int w,int h,const char* label,bool on){
  rect(x,y,w,h,on?0x0410:0x1082);
  rect(x,y,w,1,on?CYAN:DGREY);rect(x,y+h-1,w,1,on?CYAN:DGREY);
  int sc=2; text(x+5,y+5,label,sc,on?WHITE:CYAN);
}
static void drawStatus(){
  rect(0,62,320,45,0x0008);
  char b[32];
  snprintf(b,sizeof(b),"%s",modeName()); panel(3,65,50,36,b,control==CTRL_MODE);
  snprintf(b,sizeof(b),"V%u",volume); panel(56,65,50,36,b,control==CTRL_VOL);
  snprintf(b,sizeof(b),"S%u",mode==MODE_FM?fmStep*10:amStep); panel(109,65,50,36,b,control==CTRL_STEP);
  if(mode==MODE_LSB||mode==MODE_USB)snprintf(b,sizeof(b),"F%s",ssbBwName[ssbBwPos]); else snprintf(b,sizeof(b),"F3");
  panel(162,65,50,36,b,control==CTRL_FILTER);
  snprintf(b,sizeof(b),"%s",bands[bandPos].name);panel(215,65,50,36,b,control==CTRL_BAND);
  snprintf(b,sizeof(b),"A%s",vfoB?"B":"A");panel(268,65,49,36,b,control==CTRL_VFOAB);
}static void drawMeter(){
  rect(0,107,320,51,0x0008);
  text(6,109,"S 1 3 5 7 9 +20 +40",1,WHITE);
  const int n=24,gap=2,sw=11,x0=7,y=124,h=17;
  int lit=map(constrain(rssi,0,80),0,80,0,n);
  for(int i=0;i<n;i++){uint16_t c=i<14?GREEN:(i<20?YELLOW:RED);rect(x0+i*(sw+gap),y,sw,h,i<lit?c:0x2104);}
  char b[32];snprintf(b,sizeof(b),"RSSI %u SNR %u",rssi,snr);text(7,145,b,1,WHITE);
}


static uint16_t wfColor(uint8_t v){
 if(v<18)return 0x0008;if(v<30)return BLUE;if(v<42)return CYAN;
 if(v<55)return GREEN;if(v<68)return YELLOW;if(v<78)return RED;return WHITE;
}
static const int SCOPE_PTS=52;
static uint8_t scopeVals[SCOPE_PTS]={0};
static int scopePos=0;
static uint16_t scopeCenter=0;
static uint32_t scopeNext=0;

static void drawScopeFrame(){
 const int x0=3,y0=158,w=314,h=41;
 rect(x0,y0,w,h,0x0000);
 for(int i=0;i<SCOPE_PTS;i++){
   int x=x0+i*6;int bh=constrain(map(scopeVals[i],0,80,1,28),1,28);
   rect(x,y0+30-bh,4,bh,CYAN);
   rect(x,y0+32,4,7,wfColor(scopeVals[i]));
 }
 rect(159,y0,2,39,RED);
 text(5,190,"SWEEP",1,WHITE);text(267,190,"RSSI",1,WHITE);
}
static void scopeTick(){
 if(!scopeOn || millis()<scopeNext)return;
 scopeNext=millis()+25;
 if(scopePos==0)scopeCenter=curFreq();
 uint16_t center=scopeCenter;
 uint16_t lo=(center>scopeSpan)?center-scopeSpan:(mode==MODE_FM?6400:150);
 uint16_t hi=center+scopeSpan;
 if(mode==MODE_FM){lo=max((uint16_t)6400,lo);hi=min((uint16_t)10800,hi);}
 else hi=min((uint16_t)30000,hi);
 uint16_t f=lo+((uint32_t)(hi-lo)*scopePos)/(SCOPE_PTS-1);
 rx.setFrequency(f);
 delay(1);
 rx.getCurrentReceivedSignalQuality();
 scopeVals[scopePos]=rx.getCurrentRSSI();
 rx.setFrequency(center);
 scopePos++;
 if(scopePos>=SCOPE_PTS){scopePos=0;drawScopeFrame();}
}
static void modeBox(int x,int y,int w,const char* name,RadioMode m){
  bool on=mode==m;
  rect(x,y,w,30,on?0x07E0:0x1082);
  rect(x,y,w,2,on?WHITE:DGREY);rect(x,y+28,w,2,on?WHITE:DGREY);
  int tw=textWidth(name,2);text(x+(w-tw)/2,y+7,name,2,on?BLACK:WHITE);
}
static void drawFooter(){
  rect(0,158,320,82,0x0008);
  if(scopeOn)drawScopeFrame();
  else {drawGlobe(18,176);text(34,167,"HAMTECH M0FXB",2,CYAN);char q[32];snprintf(q,sizeof(q),"BFO %d HZ",bfo);text(184,169,q,1,(control==CTRL_BFO)?YELLOW:WHITE);}
  modeBox(4,204,72,"AM",MODE_AM);modeBox(82,204,72,"FM",MODE_FM);
  modeBox(160,204,72,"LSB",MODE_LSB);modeBox(238,204,78,"USB",MODE_USB);
}static void drawUI(){rect(0,0,320,240,0x0008);drawHeader();drawStatus();drawMeter();drawFooter();}
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
static void applySsbBw(){
 if(mode==MODE_LSB||mode==MODE_USB){
   uint8_t code=ssbBwCode[ssbBwPos];
   rx.setSSBAudioBandwidth(code);
   rx.setSSBSidebandCutoffFilter((code==0||code==4||code==5)?0:1);
 }
}
static void applyBand(){
 if(mode==MODE_FM){ssbLoaded=false;rx.setFM(6400,10800,fmFreq,fmStep);}
 else if(mode==MODE_AM){ssbLoaded=false;rx.setAM(150,30000,amFreq,amStep);rx.setBandwidth(2,1);}
 else{
   if(!ssbLoaded)loadSSB();
   rx.setSSB(520,30000,amFreq,amStep,mode==MODE_USB?2:1);
   rx.setSSBAutomaticVolumeControl(1);rx.setSsbSoftMuteMaxAttenuation(0);rx.setSSBBfo(0);applySsbBw();
 }
 rx.setVolume(volume);
}
static void changeMode(int dir){
 int m=(int)mode+(dir>0?1:-1);if(m>3)m=0;if(m<0)m=3;mode=(RadioMode)m;applyBand();drawHeader();drawStatus();drawFooter();
 Serial.printf("MODE SELECTED: %s (%s)\n",modeName(),mode==MODE_LSB?"LOWER SIDEBAND":mode==MODE_USB?"UPPER SIDEBAND":modeName());
}
static void changeStep(int dir){
 if(mode==MODE_FM){fmStep=fmStep==10?1:10;}
 else{const uint16_t st[5]={1,5,9,10,100};int p=0;for(int i=0;i<5;i++)if(st[i]==amStep)p=i;p=(p+(dir>0?1:4))%5;amStep=st[p];}
 rx.setFrequencyStep(mode==MODE_FM?fmStep:amStep);drawStatus();
}
static void changeFilter(int dir){
 if(!(mode==MODE_LSB||mode==MODE_USB))return;
 ssbBwPos=(ssbBwPos+(dir>0?1:5))%6;applySsbBw();drawStatus();
}
static void changeBand(int dir){
 int n=sizeof(bands)/sizeof(bands[0]);bandPos=(bandPos+(dir>0?1:n-1))%n;
 const Band &bd=bands[bandPos];mode=bd.pref;
 if(mode==MODE_FM)fmFreq=bd.start;else amFreq=bd.start;
 applyBand();drawUI();
}
static void changeBfo(int dir){
 if(!(mode==MODE_LSB||mode==MODE_USB))return;
 bfo=constrain((int)bfo+dir*25,-2000,2000);rx.setSSBBfo(bfo);drawFooter();
}
static void changeVfoAB(){
 if(!vfoB){vfoA=amFreq;amFreq=vfoBfreq;vfoB=true;}else{vfoBfreq=amFreq;amFreq=vfoA;vfoB=false;}
 if(mode!=MODE_FM){rx.setFrequency(amFreq);drawHeader();}drawStatus();
}
static void changeScope(int dir){
 int v=(int)scopeSpan+(dir>0?5:-5);scopeSpan=constrain(v,5,50);drawScopeFrame();
}
static void rotateAction(int dir){
 if(control==CTRL_VFO){if(mode==MODE_FM)fmFreq=constrain((int)fmFreq+dir*fmStep,6400,10800);else amFreq=constrain((int)amFreq+dir*amStep,mode==MODE_AM?150:520,30000);rx.setFrequency(curFreq());drawHeader();}
 else if(control==CTRL_VOL){volume=constrain((int)volume+dir,0,63);rx.setVolume(volume);drawStatus();}
 else if(control==CTRL_STEP)changeStep(dir);
 else if(control==CTRL_FILTER)changeFilter(dir);
 else if(control==CTRL_MODE)changeMode(dir);
 else if(control==CTRL_BAND)changeBand(dir);
 else if(control==CTRL_BFO)changeBfo(dir);
 else if(control==CTRL_VFOAB){changeVfoAB();}
 else if(control==CTRL_SCOPE)changeScope(dir);
}
static void selectNext(){
 control=(Control)(((int)control+1)%9);
 drawStatus();drawFooter();
}

void setup(){
 Serial.begin(115200);delay(300);Serial.println("ATS-25 HamTech M0FXB Controller V1.7.1 BETA");
 initLCD();rect(0,0,320,240,BLACK);
 pinMode(ENC_A,INPUT_PULLUP);pinMode(ENC_B,INPUT_PULLUP);pinMode(ENC_SW,INPUT_PULLUP);
 attachInterrupt(ENC_A,encISR,CHANGE);attachInterrupt(ENC_B,encISR,CHANGE);
 Wire.begin(I2C_SDA,I2C_SCL);rx.setI2CFastModeCustom(100000);
 uint8_t addr=rx.getDeviceI2CAddress(RX_RST);Serial.printf("SI47XX I2C address: 0x%02X\n",addr);
 rx.setup(RX_RST,0);
 prefs.begin("hamtech",false);
 volume=prefs.getUChar("vol",35);amFreq=prefs.getUShort("amf",7100);fmFreq=prefs.getUShort("fmf",10000);
 applyBand();drawUI();
 Serial.println("V1.7.1 BETA READY");
}
void loop(){
 int d;noInterrupts();d=encDelta;encDelta=0;interrupts();if(d)rotateAction(d>0?1:-1);
 static bool was=false;static uint32_t downAt=0;bool down=digitalRead(ENC_SW)==LOW;
 if(down&&!was)downAt=millis();
 if(!down&&was&&millis()-downAt>25)selectNext();
 was=down;
 static uint32_t t=0;if(millis()-t>500){t=millis();rx.getCurrentReceivedSignalQuality();rssi=rx.getCurrentRSSI();snr=rx.getCurrentSNR();drawMeter();Serial.printf("V1.7.1 %s %u VOL %u BW %s RSSI %u SNR %u\n",modeName(),curFreq(),volume,(mode==MODE_LSB||mode==MODE_USB)?ssbBwName[ssbBwPos]:"3.0",rssi,snr);}
 scopeTick();
 static uint32_t sv=0;if(millis()-sv>5000){sv=millis();prefs.putUChar("vol",volume);prefs.putUShort("amf",amFreq);prefs.putUShort("fmf",fmFreq);}
}
