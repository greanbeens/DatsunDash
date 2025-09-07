#include <CANSAME5x.h>
#include "header.h"

CANSAME5x CANS;

int canRecieved;
int can_id;

int16_t motorRpm;
int16_t motorTemp;
int16_t inverterTemp;
int16_t packSOC;

float minCellVoltage;
int maxCellTemp;
int16_t packVoltage;
int16_t packCurrent;

float ceiling;
bool update = false;

unsigned long oldMillis = 0;
unsigned long oldMs = 0;

bool DispVoltage = false;
bool flashRed = false;
bool oldFlashRed = false;

const float conversion = 4096 / 3.3;
float battV = 0;

#define MIN_CELL_VOLTAGE 0x0716
#define SOC 0x712
#define PACK_VOLTAGE 0x06D0
#define MOTORTEMPS 0x0401
#define MOTORRPM 0x0402

#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_HX8357.h"

#ifdef ESP8266
   #define TFT_CS   0
   #define TFT_DC   15
#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32C6)
   #define TFT_CS   7
   #define TFT_DC   8
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S3)
   #define TFT_CS   15
   #define TFT_DC   33
#elif defined(TEENSYDUINO)
   #define TFT_DC   10
   #define TFT_CS   4
#elif defined(ARDUINO_STM32_FEATHER)
   #define TFT_DC   PB4
   #define TFT_CS   PA15
#elif defined(ARDUINO_NRF52832_FEATHER)  /* BSP 0.6.5 and higher! */
   #define TFT_DC   11
   #define TFT_CS   31
#elif defined(ARDUINO_MAX32620FTHR) || defined(ARDUINO_MAX32630FTHR)
   #define TFT_DC   P5_4
   #define TFT_CS   P5_3
#else
    // Anything else, defaults!
   #define TFT_CS   9
   #define TFT_DC   10
#endif


#define TFT_RST -1

// Use hardware SPI and the above for CS/DC
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);


// setup placed here for readability
void setup() {
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(3);

  tft.fillScreen(HX8357_BLACK);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false); // turn off STANDBY
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true); // turn on booster


  // start the CAN bus at 250 kbps
  if (!CANS.begin(250000)) {
    Serial.println("Starting CAN failed!");
    while (1) delay(10);
  }
  Serial.println("Starting CAN!");


  screenSetup();
}


void loop() {
  int packetSize = CANS.parsePacket();

  if (packetSize) {
    canRecieved = 1;
    char buf[8];
    can_id = CANS.packetId();

    for (int i = 0; i < packetSize; i++) {
      buf[i] = CANS.read();
      
    }

    switch (can_id) {
      case MIN_CELL_VOLTAGE:
        if (packetSize) {
          minCellVoltage = (float)((buf[3]) + (buf[2] * 256))/10000;
          maxCellTemp = (buf[0]);
          Serial.print("minCellVoltage: ");
          Serial.println(minCellVoltage);
         //Serial.println(buf[3], HEX);
         //Serial.println(buf[2], HEX);
          
          Serial.print("max cell temp: ");
          Serial.println(maxCellTemp);
          updateScreen(TEXT_1_X, TEXT_1_Y, maxCellTemp, 3, "C", false);
          updateScreen(TEXT_3_X, TEXT_3_Y, minCellVoltage, 3, "V", true);
          
        }
        break;

      case PACK_VOLTAGE:
        if (packetSize) {
          packVoltage = ((buf[3]*256) + buf[2]/10000);
          packCurrent = (buf[1]*256) + buf[0];
          //Serial.println(packVoltage);
          //Serial.println(packCurrent);
        }
        break;

     case MOTORRPM:
        if (packetSize) {
          motorRpm =( buf[3]*256 )+ buf[2];
          updateScreen(TEXT_6_X, TEXT_6_Y, motorRpm, 4, "rpm", false);
          //Serial.println(motorRpm);
        }
      break;

      case SOC:
        if(packetSize){
          packSOC = buf[7];
          updateScreen(TEXT_2_X, TEXT_2_Y, packSOC, 3, "%", false);
        }

      case MOTORTEMPS:
        if(packetSize){
          motorTemp = buf[0];
          inverterTemp = buf[1];
          updateScreen(TEXT_4_X, TEXT_4_Y, motorTemp, 2, "%", false);
          updateScreen(TEXT_5_X, TEXT_5_Y, inverterTemp, 2, "%", false);
        }

      default:
        break;
    }
  }

 if(DispVoltage){
  ceiling = (minCellVoltage*(conversion));
    if(ceiling > 4095){
      ceiling = 4095;
    }
  analogWrite(A1, ceiling);
 } else {
   analogWrite(A1, (maxCellTemp*conversion/100));
 }

  if((millis() - oldMs) > 2700){
    oldMs = millis();
    flashRed = !flashRed;
    update = true;
  }

  if(update && maxCellTemp > 50){
    update = false;
    oldFlashRed = flashRed;
    if (flashRed){
      tft.fillScreen(HX8357_RED);    // fill black
    } else {
      tft.fillScreen(HX8357_BLACK);    // fill black
    }
      screenSetup();
      updateScreen(TEXT_1_X, TEXT_1_Y, maxCellTemp, 3, "C", false);
      updateScreen(TEXT_2_X, TEXT_2_Y, packSOC, 3, "%", false);
      updateScreen(TEXT_3_X, TEXT_3_Y, minCellVoltage, 3, "V", true);
      updateScreen(TEXT_6_X, TEXT_6_Y, motorRpm, 4, "rpm", false);
  }
}

#include "header.h"

void screenSetup() {
  tft.setTextSize(3);               // set default size
  tft.setTextColor(HX8357_WHITE, HX8357_BLACK);
  tft.setCursor(TITLE_1_X, TITLE_1_Y);    // set titles
  tft.print("MAX CELL TMP:");
  tft.setCursor(TITLE_2_X, TITLE_2_Y);
  tft.print("PACK SOC:");
  tft.setCursor(TITLE_6_X, TITLE_6_Y);
  tft.print("RPM:");
  tft.setCursor(TITLE_3_X, TITLE_3_Y);
  tft.print("MIN CELL V:");
  tft.setCursor(TITLE_4_X, TITLE_4_Y);
  tft.print("MOTOR TMP:");
  tft.setCursor(TITLE_5_X, TITLE_5_Y);
  tft.print("INVERTER TMP:");

}

/*
    update screen wherever needed
    for example: diplay 55 with 3 digits
         prints: _ 5 5
                 ^black space
*/
void updateScreen(int x, int y, float value, int digits, String unit, bool isFloat) {
  tft.setTextSize(5);
  tft.setCursor(x, y);
  tft.setTextColor(HX8357_WHITE, HX8357_BLACK);
  
  char str1[10] = {0};
  
  if (not isFloat){
    switch (digits) {
      case 1: sprintf(str1, "%1.0f", value); break;
      case 2: sprintf(str1, "%2d", value); break;
      case 3: sprintf(str1, "%3.0f", value); break;
      case 4: sprintf(str1, "%4.0f", value); break;
      case 5: sprintf(str1, "%5d", value); break;
      default: sprintf(str1, "fail bruh"); break;
    }
   }    else {
    switch (digits) {
      case 1: sprintf(str1, "%1.2f", value); break;
      case 2: sprintf(str1, "%2.2f", value); break;
      case 3: sprintf(str1, "%3.2f", value); break;
      case 4: sprintf(str1, "%4.2f", value); break;
      case 5: sprintf(str1, "%5.2f", value); break;
      default: sprintf(str1, "fail bruh"); break;
    }
  }

  tft.print(str1);
  
  tft.setTextSize(3);
  tft.print(unit);
}
