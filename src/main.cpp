#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include <SI4735.h>

// ATS-25 HamTech M0FXB Controller V1.0
// Clean-room UI/controller. Candidate MAX-Decoder mapping established during project.
static constexpr int PIN_BL=14, PIN_RX_RST=12, PIN_ENC_A=16, PIN_ENC_B=17, PIN_ENC_SW=33;
static constexpr int PIN_BAT=35, PIN_MUTE=27;
static constexpr int I2C_SDA=21, I2C_SCL=22;

TFT_eSPI tft;
SI4735 rx;
WebServer web(80);

enum Mode { FM, AM, LSB, USB, CW };
Mode mode=USB;
uint16_t freq=14074; // kHz except FM uses 10 kHz units
uint16_t stepK=1;
uint8_t volume=35;
int16_t bfo=0;
uint8_t rssi=0,snr=0;
volatile int encDelta=0;
uint32_t lastUi=0,lastMeter=0;

const char* modeName(){ switch(mode){case FM:return "FM";case AM:return "AM";case LSB:return "LSB";case USB:return "USB";case CW:return "CW";} return "?"; }

void IRAM_ATTR encISR(){ static uint8_t old=0; uint8_t n=(digitalRead(PIN_ENC_A)<<1)|digitalRead(PIN_ENC_B); uint8_t x=(old<<2)|n; if(x==0b1101||x==0b0100||x==0b0010||x==0b1011) encDelta++; if(x==0b1110||x==0b0111||x==0b0001||x==0b1000) encDelta--; old=n; }

void radioApply(){
  if(mode==FM){ if(freq<6400||freq>10800)freq=10000; rx.setFM(6400,10800,freq,10); }
  else { if(freq<150||freq>30000)freq=14074; rx.setAM(150,30000,freq,stepK); }
  rx.setVolume(volume);
}

void drawMeter(){
  int w=map(constrain(rssi,0,80),0,80,0,300);
  tft.fillRect(10,76,300,18,TFT_DARKGREY); tft.fillRect(10,76,w,18,TFT_GREEN);
  tft.setTextColor(TFT_WHITE,TFT_BLACK); tft.setTextSize(1); tft.setCursor(10,98); tft.printf("S1  3  5  7  9  +20  +40     RSSI %u  SNR %u",rssi,snr);
}

void drawSpectrum(){
  tft.fillRect(0,112,320,82,TFT_BLACK); tft.drawFastHLine(0,193,320,TFT_BLUE);
  static uint8_t p[64]={};
  for(int i=0;i<64;i++){ p[i]=(p[i]*3 + random(8,70))/4; int x=i*5; tft.drawFastVLine(x,193-p[i],p[i], i==32?TFT_RED:TFT_CYAN); }
  tft.drawFastVLine(160,112,82,TFT_RED);
}

void drawWaterfall(){
  static int row=0; row=(row+1)%30; int y=198+row;
  for(int x=0;x<320;x+=4){ uint8_t v=random(0,100); uint16_t c=v>85?TFT_WHITE:v>65?TFT_RED:v>45?TFT_YELLOW:v>25?TFT_GREEN:TFT_BLUE; tft.fillRect(x,y,4,1,c); }
}

void drawUI(){
  tft.fillScreen(TFT_BLACK); tft.fillRect(0,0,320,34,TFT_BLUE);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE,TFT_BLUE); tft.setTextSize(3);
  char f[24]; if(mode==FM) snprintf(f,sizeof(f),"%u.%02u MHz",freq/100,freq%100); else snprintf(f,sizeof(f),"%u.%03u MHz",freq/1000,freq%1000);
  tft.drawString(f,160,17);
  tft.setTextDatum(TL_DATUM); tft.setTextSize(2); tft.setTextColor(TFT_WHITE,TFT_BLACK); tft.setCursor(6,42); tft.printf("%s   STEP %u kHz   VOL %u",modeName(),stepK,volume);
  tft.setTextSize(1); tft.setCursor(6,62); tft.print("HamTech M0FXB   WiFi: ATS25-HamTech");
  drawMeter(); drawSpectrum();
  tft.fillRect(0,230,320,10,TFT_BLUE);
}

String page(){
  String s="<!doctype html><meta name=viewport content='width=device-width'><style>body{background:#05070b;color:#fff;font-family:Arial;text-align:center}.f{font-size:44px;color:#4db8ff}.m{font-size:22px;color:#5f5}.box{border:1px solid #168cff;padding:14px;margin:12px;border-radius:10px}</style>";
  s+="<h2>ATS-25 HamTech M0FXB Controller</h2><div class=f>";
  if(mode==FM) s+=String(freq/100.0,2)+" MHz"; else s+=String(freq/1000.0,3)+" MHz";
  s+="</div><div class=m>"+String(modeName())+"</div><div class=box>RSSI "+String(rssi)+" &nbsp; SNR "+String(snr)+"<br>Volume "+String(volume)+"</div><small>V1.0</small>"; return s;
}

void setup(){
  Serial.begin(115200); delay(300); Serial.println("ATS-25 HamTech M0FXB Controller V1.0");
  pinMode(PIN_BL,OUTPUT); digitalWrite(PIN_BL,LOW); pinMode(PIN_MUTE,OUTPUT); digitalWrite(PIN_MUTE,HIGH);
  pinMode(PIN_ENC_A,INPUT_PULLUP); pinMode(PIN_ENC_B,INPUT_PULLUP); pinMode(PIN_ENC_SW,INPUT_PULLUP);
  attachInterrupt(PIN_ENC_A,encISR,CHANGE); attachInterrupt(PIN_ENC_B,encISR,CHANGE);
  Wire.begin(I2C_SDA,I2C_SCL); tft.init(); tft.setRotation(1);
  rx.setI2CFastModeCustom(100000); rx.setup(PIN_RX_RST,0); radioApply();
  WiFi.mode(WIFI_AP); WiFi.softAP("ATS25-HamTech");
  web.on("/",[]{web.send(200,"text/html",page());}); web.on("/status",[]{web.send(200,"application/json",String("{\"freq\":")+freq+",\"mode\":\""+modeName()+"\",\"rssi\":"+rssi+",\"snr\":"+snr+"}");}); web.begin();
  drawUI();
}

void loop(){
  web.handleClient();
  int d; noInterrupts(); d=encDelta; encDelta=0; interrupts();
  if(d){ int q=d>0?1:-1; if(mode==FM) freq=constrain((int)freq+q*10,6400,10800); else freq=constrain((int)freq+q*stepK,150,30000); radioApply(); drawUI(); }
  static bool was=HIGH; bool sw=digitalRead(PIN_ENC_SW); if(was==HIGH&&sw==LOW){ stepK=(stepK==1?5:stepK==5?10:stepK==10?100:1); drawUI(); delay(180); } was=sw;
  if(millis()-lastMeter>600){ lastMeter=millis(); rx.getCurrentReceivedSignalQuality(); rssi=rx.getCurrentRSSI(); snr=rx.getCurrentSNR(); drawMeter(); drawWaterfall(); }
}
