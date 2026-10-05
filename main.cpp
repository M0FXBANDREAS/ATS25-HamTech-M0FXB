#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SI4735.h>

// ATS-25 HamTech M0FXB Controller V1.1 BETA
// Experimental private build. Direct ILI9341 driver avoids generic TFT_eSPI setup mismatch.
static constexpr int TFT_SCLK=18,TFT_MOSI=23,TFT_MISO=19,TFT_CS=15,TFT_DC=2,TFT_RST=4,TFT_LED=14;
static constexpr int RX_RST=12,I2C_SDA=21,I2C_SCL=22;
static constexpr int ENC_A=17,ENC_B=16,ENC_SW=33,AUDIO_MUTE=27;
SPIClass lcd(VSPI); SI4735 rx;
uint16_t freq=14074, stepK=1; uint8_t volume=35,rssi=0,snr=0; volatile int encDelta=0;

static void sel(bool v){digitalWrite(TFT_CS,v?LOW:HIGH);} static void cmd(uint8_t c){digitalWrite(TFT_DC,LOW);sel(1);lcd.transfer(c);sel(0);} static void d8(uint8_t d){digitalWrite(TFT_DC,HIGH);sel(1);lcd.transfer(d);sel(0);} static void d16(uint16_t d){d8(d>>8);d8(d);}
static void addr(int x0,int y0,int x1,int y1){cmd(0x2A);d16(x0);d16(x1);cmd(0x2B);d16(y0);d16(y1);cmd(0x2C);} static void rect(int x,int y,int w,int h,uint16_t c){if(w<=0||h<=0)return;addr(x,y,x+w-1,y+h-1);digitalWrite(TFT_DC,HIGH);sel(1);for(uint32_t i=0;i<(uint32_t)w*h;i++)lcd.transfer16(c);sel(0);} 
static void initLCD(){pinMode(TFT_CS,OUTPUT);pinMode(TFT_DC,OUTPUT);pinMode(TFT_RST,OUTPUT);pinMode(TFT_LED,OUTPUT);digitalWrite(TFT_CS,HIGH);digitalWrite(TFT_LED,LOW);digitalWrite(TFT_RST,HIGH);delay(10);digitalWrite(TFT_RST,LOW);delay(30);digitalWrite(TFT_RST,HIGH);delay(150);lcd.begin(TFT_SCLK,TFT_MISO,TFT_MOSI,TFT_CS);lcd.beginTransaction(SPISettings(20000000,MSBFIRST,SPI_MODE0));cmd(0x01);delay(120);cmd(0x28);cmd(0x3A);d8(0x55);cmd(0x36);d8(0x28);cmd(0x11);delay(120);cmd(0x29);delay(20);digitalWrite(TFT_LED,LOW);}
// Tiny 3x5 digits for a dependency-free frequency display.
const uint8_t dg[10]={0x7B,0x48,0x6D,0x6C,0x5E,0x36,0x37,0x68,0x7F,0x7E};
static void digit(int x,int y,int n,int s,uint16_t c){uint8_t b=dg[n];for(int yy=0;yy<5;yy++)for(int xx=0;xx<3;xx++)if(b&(1<<(yy*3+xx)%8))rect(x+xx*s,y+yy*s,s,s,c);} 
static void drawFreq(){rect(0,0,320,58,0x001F);char b[12];snprintf(b,sizeof(b),"%05u",freq);int x=35;for(int i=0;i<5;i++){digit(x,10,b[i]-'0',7,0xFFFF);x+=29;}rect(180,42,4,4,0xF800);}
static void meter(){rect(10,78,300,18,0x4208);int w=map(constrain(rssi,0,80),0,80,0,300);rect(10,78,w,18,rssi>55?0xF800:(rssi>30?0xFFE0:0x07E0));}
static void spectrum(){rect(0,110,320,84,0x0000);for(int x=0;x<320;x+=5){int h=map((x*13+rssi*7)%61,0,60,5,70);rect(x,194-h,3,h,0x07FF);}rect(159,110,2,84,0xF800);}
static void waterfall(){static int row=0;row=(row+1)%40;int y=199+row;for(int x=0;x<320;x+=5){uint8_t v=(x*7+row*11+rssi*5)%100;uint16_t c=v>80?0xFFFF:v>60?0xF800:v>40?0xFFE0:v>20?0x07E0:0x001F;rect(x,y,5,1,c);}}
void IRAM_ATTR encISR(){static uint8_t old=0;uint8_t n=(digitalRead(ENC_A)<<1)|digitalRead(ENC_B);uint8_t q=(old<<2)|n;if(q==0xD||q==4||q==2||q==0xB)encDelta++;if(q==0xE||q==7||q==1||q==8)encDelta--;old=n;}
static void radioApply(){rx.setAM(150,30000,freq,stepK);rx.setVolume(volume);} 
void setup(){Serial.begin(115200);delay(300);Serial.println("ATS-25 HamTech M0FXB Controller V1.1 BETA");pinMode(AUDIO_MUTE,OUTPUT);digitalWrite(AUDIO_MUTE,HIGH);initLCD();rect(0,0,320,240,0);drawFreq();rect(0,60,320,8,0xFFFF);meter();spectrum();pinMode(ENC_A,INPUT_PULLUP);pinMode(ENC_B,INPUT_PULLUP);pinMode(ENC_SW,INPUT_PULLUP);attachInterrupt(ENC_A,encISR,CHANGE);attachInterrupt(ENC_B,encISR,CHANGE);Wire.begin(I2C_SDA,I2C_SCL);rx.setI2CFastModeCustom(100000);rx.setup(RX_RST,0);radioApply();Serial.println("V1.1 BETA READY");}
void loop(){int d;noInterrupts();d=encDelta;encDelta=0;interrupts();if(d){freq=constrain((int)freq+(d>0?stepK:-stepK),150,30000);radioApply();drawFreq();}static bool was=1;bool sw=digitalRead(ENC_SW);if(was&&!sw){stepK=stepK==1?5:stepK==5?10:stepK==10?100:1;delay(180);}was=sw;static uint32_t t=0;if(millis()-t>600){t=millis();rx.getCurrentReceivedSignalQuality();rssi=rx.getCurrentRSSI();snr=rx.getCurrentSNR();meter();spectrum();waterfall();Serial.printf("V1.1 %u kHz RSSI %u SNR %u heap %u\n",freq,rssi,snr,ESP.getFreeHeap());}}
