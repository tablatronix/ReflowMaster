/*
---------------------------------------------------------------------------
Reflow Master Control - v1.0.3 - 28/08/2018

AUTHOR/LICENSE:
Created by Seon Rozenblum - seon@unexpectedmaker.com
Copyright 2016 License: GNU GPL v3 http://www.gnu.org/licenses/gpl-3.0.html

LINKS:
Project home: github.com/unexpectedmaker/reflowmaster
Blog: unexpectedmaker.com

DISCLAIMER:
This software is furnished "as is", without technical support, and with no 
warranty, express or implied, as to its usefulness for any purpose.

PURPOSE:
This controller is the software that runs on the Reflow Master toaster oven controller made by Unexpected Maker

HISTORY:
01/08/2018 v1.0   - Initial release.
13/08/2018 v1.01  - Settings UI button now change to show SELECT or CHANGE depending on what is selected
27/08/2018 v1.02  - Added tangents to the curve for ESP32 support, Improved graph curves, fixed some UI glitches, made end graph time value be the end profile time
28/08/2018 v1.03  - Added some graph smoothig
---------------------------------------------------------------------------
*/

/*
 * NOTE: This is a work in progress...
 */
#include <ESP8266WiFi.h>
#include <ota.h>
#include <creds.h>

#include <SPI.h>
#include <spline.h> // http://github.com/kerinin/arduino-splines
#include "Adafruit_GFX.h" // Library Manager

// #define USE_ADAFRUIT_ST7735
// #define USE_ADAFRUIT_ST7789H

#include <neoindicator.h>

#ifdef USE_ADAFRUIT_ST7735
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#elif defined(USE_ADAFRUIT_ST7789)
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7735
#else
#include "Adafruit_ILI9341.h" // Library Manager
#endif

// #include "MAX31855.h" // by Rob Tillaart Library Manager
#include "Adafruit_MAX31855.h"

// #include "OneButton.h" // Library Manager
#include "ReflowMasterProfile.h" 
// #include <FlashStorage.h> // Library Manager

// #include <Fonts/FreeMono9pt7b.h>


/**
 * Encoder Library
 */
#include <ClickEncoder.h>

#define ENCODER_PINA     4
#define ENCODER_PINB     5
#define ENCODER_BTN      -1

#define ENCODER_STEPS_PER_NOTCH    1   // Change this depending on which encoder is used

ClickEncoder encoder = ClickEncoder(ENCODER_PINA,ENCODER_PINB,ENCODER_BTN,ENCODER_STEPS_PER_NOTCH);

// used to obtain the size of an array of any type
#define ELEMENTS(x)   (sizeof(x) / sizeof(x[0]))

// used to show or hide serial debug output
#define DEBUG
#define TRACELOG

// TFT SPI pins
// #define TFT_DC 0
// #define TFT_CS 3
// #define TFT_RESET 1

// @TODO use CS and get tft and max31855 working with hspi pins
#define TFT_DC     0 // D3
#define TFT_CS    -1 // Tied to ground
#define TFT_RST   -1 // tied to RST - ST7789 CS low, D2 not working
// Initialise the TFT screen
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RESET);
// Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
// Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#ifdef USE_ADAFRUIT_ST7735
using RM_tft = Adafruit_ST7735;
#elif defined(USE_ADAFRUIT_ST7789)
using RM_tft = Adafruit_ST7789;
#else 
using RM_tft = Adafruit_ILI9341;
#endif

RM_tft tft = RM_tft(TFT_CS, TFT_DC, TFT_RST);

// NODEMCU hspi
// HW scl      D5 14
// HW mosi/sda D7 13
// HW miso     D6 12
// HW cs       D8 15
// 
// ESPLED      D4 02
// boardLED    D0 16


// MAX 31855 Pins
// #define MAXDO   14
// #define MAXCS   13
// #define MAXCLK  12

// #define MAXDO   5 // HWMISO
#define MAXCS   15 // 2
// #define MAXCLK  4 // HWSLK 
Adafruit_MAX31855 tc(MAXCS);

#ifdef TC2
Adafruit_MAX31855 tcB(1);
#endif

// Adafruit_MAX31855 tc(MAXCLK, MAXCS, MAXDO);
// Initialise the MAX31855 IC for thermocouple tempterature reading
// MAX31855 tc(MAXCLK, MAXCS, MAXDO);

uint16_t rotation = 3;

#ifdef USE_ADAFRUIT_ST7735
uint16_t textsize_1 = 1;
uint16_t textsize_2 = 1;
uint16_t textsize_3 = 1;
uint16_t textsize_4 = 2;
uint16_t textsize_5 = 3;
#else
uint16_t textsize_1 = 1;
uint16_t textsize_2 = 2;
uint16_t textsize_3 = 3;
uint16_t textsize_4 = 4;
uint16_t textsize_5 = 5;
#endif

// #ifdef ESP8266
// #define A1 A0
// #define A2 A0
// #define A3 A0
// #endif

#define BUTTON0 0 // menu buttons
#define BUTTON1 A0 // menu buttons
#define BUTTON2 A0 // menu buttons
#define BUTTON3 A0 // menu buttons

#define BUZZER 0  // buzzer
#define FAN    0  // fan control
#define RELAY  16   // relay control

// #define PWMRANGE 255 // esp8266 1024

// Just a bunch of re-defined colours
#define BLUE      0x001F
#define TEAL      0x0438
#define GREEN     0x07E0
#define CYAN      0x07FF
#define RED       0xF800
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0
#define ORANGE    0xFC00
#define PINK      0xF81F
#define PURPLE    0x8010
#define GREY      0xC618
#define WHITE     0xFFFF
#define BLACK     0x0000
#define DKBLUE    0x000D
#define DKTEAL    0x020C
#define DKGREEN   0x03E0
#define DKCYAN    0x03EF
#define DKRED     0x6000
#define DKMAGENTA 0x8008
#define DKYELLOW  0x8400
#define DKORANGE  0x8200
#define DKPINK    0x9009
#define DKPURPLE  0x4010
#define DKGREY    0x4A49

// theme
#define grid_color DKBLUE
#define axis_color BLUE
#define axis_text_color  WHITE
#define reflow_trace_color  TEAL
#define log_trace_color  PINK

#define ButtonCol1 GREEN
#define ButtonCol2 RED
#define ButtonCol3 BLUE
#define ButtonCol4 YELLOW

// Save data struct
typedef struct {
  boolean valid = false;
  boolean useFan = true;
  byte paste = 0;
  float power = 1;
  int lookAhead = 12;
  int lookAheadWarm = 12;
  int tempOffset = 0;
  bool startFullBlast = false;
} Settings;

const String ver = "1.03_bboy";
bool newSettings = false;

unsigned long nextTempRead;
unsigned long nextTempAvgRead;
int avgReadCount = 0; // running avg
int avgSamples = 6; // 1 to disable averaging
int tempSampleRate = 1000; // how often to sample temperature when idle

int hotTemp  = 80; // C burn temperature for HOT indication, 0=disable
int coolTemp = 50; // C burn temperature for HOT indication, 0=disable
int shutDownTemp = 210; // degrees C

double timeX = 0;
unsigned long nextTimeX = 0;
int timescale = 1000;
// int timescale = 60*1000;

byte state; // 0 = ready, 1 = warmup, 2 = reflow, 3 = finished, 10 Menu, 11+ settings
byte state_settings = 0;
byte settings_pointer = 0;
byte stateStart = 10;

// Menus
// menuColorA
// menuColorB
// menuColorC
// menuColorD
// menuColorActive
// menuColorInactive

int menuSel = 0; // current selected menu item, 1 indexed
int menuCount = 3; // how many menus currently to scroll select
int menuSelX = 0; // hook into printlncenter for now
int menuSelY = 0; // hook into printlncenter for now

// Initialise an array to hold 4 profiles
// Increase this array if you plan to add more
ReflowGraph solderPaste[5];
// Index into the current profile
int currentGraphIndex = 0;

// Calibration data - currently diabled in this version
int calibrationState = 0;
long calibrationSeconds = 0;
long calibrationStatsUp = 0;
long calibrationStatsDown = 300;
bool calibrationUpMatch = false;
bool calibrationDownMatch = false;
float calibrationDropVal = 0;
float calibrationRiseVal = 0;

// Runtime reflow variables
float currentDuty = 0;
float currentTemp = 0;
float currentTempB = 0; // alt thermocouple
float currentTempAvg = 0;
float lastTemp = -1;
float currentDetla = 0;
unsigned int currentPlotColor = GREEN;
bool isCuttoff = false;
bool isFanOn = false;
float lastWantedTemp = -1;
int buzzerCount = 5;

// - tablatronix
bool tcWarn = false; // stop and show error if a tc issue is detected
bool skipWarmup = false;
bool settingWarn = false;
bool useInternal = false;

// Graph Size for UI
int graphRangeMin_X = 0;
int graphRangeMin_Y = 30;
int graphRangeMax_X = -1;
int graphRangeMax_Y = 165;
int graphRangeStep_X = 30;
int graphRangeStep_Y = 15;

float maxT;
float minT;


// grapher
int16_t graph_w = 240-30;
int16_t graph_h = 240-80; // @todo add overhead to height max to allow for graphing overshoot
int graphOff_x = 30;
int graphOff_y = 240-20;

// Create a spline reference for converting profile values to a spline for the graph
Spline baseCurve;

// Initialise the buttons using OneButton library
// OneButton button0(BUTTON0, false);
// OneButton button1(BUTTON1, false);
// OneButton button2(BUTTON2, false);
// OneButton button3(BUTTON3, false);

// Initiliase a reference for the settings file that we store in flash storage
Settings set;
// Initialise flash storage
// FlashStorage(flash_store, Settings);

// This is where we initialise each of the profiles that will get loaded into the Reflkow Master
void LoadPaste()
{
#ifdef DEBUG
  Serial.println("Load Paste");
#endif

/*
  Each profile is initialised with the follow data:

      Paste name ( String )
      Paste type ( String )
      Paste Reflow Temperature ( int )
      Profile graph X values - time
      Profile graph Y values - temperature
      Length of the graph  ( int, how long if the graph array )
      Fan kick in time if using a fan ( int, seconds )
      Open door message time ( int, seconds ) // REFLOW END! OFF TIME!!!
 */
  float baseGraphX[7] = { 1, 90, 180, 210, 240, 270, 300 }; // time
  float baseGraphY[7] = { 27, 90, 130, 138, 165, 138, 27 }; // value

  solderPaste[0] = ReflowGraph( "CHIPQUIK", "No-Clean Sn42/Bi57.6/Ag0.4", 138, baseGraphX, baseGraphY, ELEMENTS(baseGraphX), 240, 270 );

  float baseGraphX2[7] = { 1, 90, 180, 225, 240, 270, 300 }; // time
  float baseGraphY2[7] = { 25, 150, 175, 190, 210, 125, 50 }; // value

  solderPaste[1] = ReflowGraph( "CHEMTOOLS L", "No Clean 63CR218 Sn63/Pb37", 183, baseGraphX2, baseGraphY2, ELEMENTS(baseGraphX2), 240, 270 );

  float baseGraphX3[6] = { 1, 75, 130, 180, 210, 250 }; // time
  float baseGraphY3[6] = { 25, 150, 175, 210, 150, 50 }; // value

  solderPaste[2] = ReflowGraph( "CHEMTOOLS S", "No Clean 63CR218 Sn63/Pb37", 183, baseGraphX3, baseGraphY3, ELEMENTS(baseGraphX3), 180, 210 );

  float baseGraphX4[7] = { 1, 60, 120, 160, 210, 260, 310 }; // time
  float baseGraphY4[7] = { 25, 105, 150, 150, 220, 150, 20 }; // value

  solderPaste[3] = ReflowGraph( "DOC SOLDER", "No Clean Sn63/Pb36/Ag2", 187, baseGraphX4, baseGraphY4, ELEMENTS(baseGraphX4), 210, 260 );

  float baseGraphX5[6] = { 1,   90,  100,  160, 170, 200  }; // time
  float baseGraphY5[6] = { 25, 100, 110, 110, 100, 25 }; // value

  solderPaste[4] = ReflowGraph( "PLA", "Anneal 110C/30min", 110, baseGraphX5, baseGraphY5, ELEMENTS(baseGraphX5), 160, 170 );

  //TODO: Think of a better way to initalise these baseGraph arrays to not need unique array creation
}

// Obtain the temp value of the current profile at time X
int GetGraphValue( int x )
{
  return ( CurrentGraph().reflowGraphY[x] );
}

int GetGraphTime( int x )
{
  return ( CurrentGraph().reflowGraphX[x] );
}

// Obtain the current profile
ReflowGraph& CurrentGraph()
{
  return solderPaste[ currentGraphIndex ];
}

// Set the current profile via the array index
void SetCurrentGraph( int index )
{
  #ifdef DEBUG
  Serial.println("SetCurrentGraph");
  #endif
  currentGraphIndex = index;
  graphRangeMax_X = solderPaste[ currentGraphIndex ].MaxTime();
  graphRangeMax_Y = solderPaste[ currentGraphIndex ].MaxValue() + 5; // extra padding
  graphRangeMin_Y = solderPaste[ currentGraphIndex ].MinValue();

#ifdef DEBUG
  Serial.print("Setting Paste: ");
  Serial.println( currentGraphIndex );
  Serial.println( CurrentGraph().n );
  Serial.println( CurrentGraph().t );
#endif

  // Initialise the spline for the profile to allow for smooth graph display on UI
  timeX = 0;
  // Serial.println(CurrentGraph().reflowGraphX,2);
  // Serial.println();
  // Serial.print(CurrentGraph().reflowGraphY,4);
  // Serial.println();
  // Serial.print(CurrentGraph().reflowTangents,4);
  // Serial.println();
  // Serial.print(CurrentGraph().len,4);
  // Serial.println();
  baseCurve.setPoints(solderPaste[ currentGraphIndex ].reflowGraphX, solderPaste[ currentGraphIndex ].reflowGraphY, solderPaste[ currentGraphIndex ].reflowTangents, solderPaste[ currentGraphIndex ].len);
  // baseCurve.setPoints(CurrentGraph().reflowGraphX, CurrentGraph().reflowGraphY, CurrentGraph().reflowTangents, CurrentGraph().len);
  baseCurve.setDegree( Hermite );
  // double x[7] = {-1,0,1,2,3,4, 5};
  // double y[7] = { 0,0,8,5,2,10,10};
  // baseCurve.setPoints(x,y,7);

  // float baseGraphX[7] = { 1, 90, 180, 210, 240, 270, 300 }; // time
  // float baseGraphY[7] = { 27, 90, 130, 138, 165, 138, 27 }; // value
  // baseCurve.setPoints(baseGraphX, baseGraphY, CurrentGraph().reflowTangents, CurrentGraph().len);

  Serial.println("re interpolate splines, rangemax: " + (String)graphRangeMax_X);
  // Re-interpolate data based on spline
  for( int ii = 0; ii <= graphRangeMax_X; ii+= 1 )
  {
    // Serial.println(solderPaste[ currentGraphIndex ].wantedCurve[ii]);
    solderPaste[ currentGraphIndex ].wantedCurve[ii] = baseCurve.value(ii);
    delay(0);
  }

  // calculate the biggest graph movement delta
  float lastWanted = -1;
  for ( int i = 0; i < solderPaste[ currentGraphIndex ].offTime; i++ )
  {
      float wantedTemp = solderPaste[ currentGraphIndex ].wantedCurve[ i ];

      if ( lastWanted > -1 )
      {
        float wantedDiff = (wantedTemp - lastWanted );
  
        if ( wantedDiff > solderPaste[ currentGraphIndex ].maxWantedDelta ){
          solderPaste[ currentGraphIndex ].maxWantedDelta = wantedDiff;
        }
      }
      lastWanted  = wantedTemp;
      delay(0);   
  }
}

void checkButtonAnalog(){
  static int lastanalog = 0;
  // @todo this will have to be reworked and it is very voltage drop dependant
  int level;
  // level = analogRead(A0);
  if(lastanalog + 200 < micros()){
    // Serial.println("micros: " + String(micros()-lastanalog));
    // Serial.println(String(micros()-lastanalog));
    level = analogRead(A0);
    lastanalog = micros();
  }
  else return;
  // else level = 255;

  level = 255;
  int th = 20;
  int base = 5; // base noise
  int debounce = 100; //ms

  // only check for single button
  if(level < th && level > base){
    Serial.println("level:" + (String)level);
    delay(debounce);
    if(level > base) return; // debounce
    selectMenu();
    delay(500); // @todo add state anti repeat..
  }

  return;

  int button0 = 65;
  int button1 = 195;
  int button2 = 625;
  int button3 = 820; // 1+2
  int buttonmax = 1024;

  button0 = 32;
  button1 = 170;
  button2 = 550;
  button3 = 733; // 1+2

  // int offset = 30;
  button0 = 215;
  button1 = button0+210; // 212
  button2 = button1+210; // 205
  button3 = button2+210; // 213

  if(level > base){
    Serial.println("BUTTON PRESS level:" + (String)level);
    delay(debounce);
    if(level < base) return; // debounce
    // delay(100); // a button was pressed delay for multi touch
  }
       if(level > button0-th && level < button0+th) button0Press();
  else if(level > button1-th && level < button1+th) button1Press();
  else if(level > button2-th && level < button2+th) button2Press();
  else if(level > button3-th && level < button3+th) button3Press();
  else if(level > buttonmax-th && level < buttonmax+th) button0Press();
  else return;
}

void safetyCheck(){
  // provides some sort of thermal runaway protection,
  // @todo add sanity checking for temperature readings, >0 etc
  // Serial.println("tc status: " + getTcStatus());
  // Serial.println("tc internal: " + (String)tc.readInternal());

  if(!tcWarn) return;
  // tc.read(); // @todo we probably do not want to read sensor too often, will need to check lastread timer
  if(tc.readError() != 0){
    tft.setTextColor(WHITE);
    tft.fillScreen(RED);
    println_Center( tft, "Thermocouple ERROR", tft.width() / 2, ( tft.height() / 2 ) + 10 );
    delay(5000); // show warning for 5 seconds
    state = 0;
  }
  else if((shutDownTemp>0) && currentTemp >= shutDownTemp && (state != 10 && state != 3)){
    SetRelayFrequency(0);
    tft.setTextColor(WHITE);
    tft.fillScreen(RED);
    tft.setTextSize(textsize_3);
    println_Center( tft, "HIGH TEMP ERROR", tft.width() / 2, ( tft.height() / 2 ) + 10 );
    tft.setTextSize(textsize_2);
    println_Center( tft, "Aborting", tft.width() / 2, ( tft.height() / 2 ) + 40 );
    delay(5000); // show warning for 5 seconds
    // if(state == 2) EndReflow(); // @todo cannot redraw last graph, which would be nice
    state = 0;
  }
}

#define STATUS_OK               0x00
#define STATUS_OPEN_CIRCUIT     0x01
#define STATUS_SHORT_TO_GND     0x02
#define STATUS_SHORT_TO_VCC     0x04
#define STATUS_NOREAD           0x80

String getTcStatus(){
  // tc.read();
  uint8_t tcStatus = tc.readError();
  if(tcStatus == STATUS_OK) return "OK";
  else if(tcStatus == STATUS_OPEN_CIRCUIT) return "Open";
  else if(tcStatus == STATUS_SHORT_TO_GND) return "GND Short";
  else if(tcStatus == STATUS_SHORT_TO_VCC) return "VCC Short";
  else if(tcStatus == STATUS_NOREAD) return "Read Failed";
  else return "ERROR";
}

#ifdef TC2
String getTcStatusB(){
  // tc.read();
  uint8_t tcStatus = tcB.readError();
  if(tcStatus == STATUS_OK) return "OK";
  else if(tcStatus == STATUS_OPEN_CIRCUIT) return "Open";
  else if(tcStatus == STATUS_SHORT_TO_GND) return "GND Short";
  else if(tcStatus == STATUS_SHORT_TO_VCC) return "VCC Short";
  else if(tcStatus == STATUS_NOREAD) return "Read Failed";
  else return "ERROR";
}
#endif

void loop()
{
  processEncoder();
  serialLoop();
  safetyCheck();
  checkButtonAnalog();

  if(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
  }

  delay(200); // service wifi

  // delay(10);
  // return;
  //
  // Used by OneButton to poll for button inputs
  // button0.tick();
  // button1.tick();
  // button2.tick();
  // button3.tick();
  // Serial.println(state);
  // Serial.println(ESP.getFreeHeap());
  // Serial.println(ESP.getHeapFragmentation());
  // Serial.println(ESP.getMaxFreeBlockSize());
  // Serial.println(millis());

  // Current activity state machine
  if ( state == 0 ) // BOOT
  {
    // Load up the profiles
    LoadPaste();
    // Set the current profile based on last selected
    SetCurrentGraph( set.paste ); // this causes the bug, 
    // Show the main menu
    ShowMenu();
    // delay(1000);
    // state = 20;
    // tft.fillScreen(BLACK);
    // state = stateStart;
    Serial.println("back in loop");
  }
  else if ( state == 1 ) // WARMUP - We sit here until the probe reaches the starting temp for the profile
  {
    if(skipWarmup){state == 2; return;}
    // Serial.println("warmup");
    if ( nextTempRead < millis() ) // we only read the probe every second
    {
      nextTempRead = millis() + tempSampleRate;
  
      ReadCurrentTemp();
      MatchTemp();

      if ( currentTemp >= GetGraphValue(0) )
      {
        // We have reached the starting temp for the profile, so lets start baking our boards!
        lastTemp = currentTemp;
        avgReadCount = 0;
        currentTempAvg = 0;
        
        StartReflow();
      }
      else if ( currentTemp > 0 )
      {
        // Show the current probe temp so we can watch as it reaches the minimum starting value
        tft.setTextColor( YELLOW, BLACK );
        tft.setTextSize(textsize_5);
        println_Center( tft, "  "+String( round_f( currentTemp ) )+"c  ", tft.width() / 2, ( tft.height() / 2 ) + 10 );
      }
    }
    return;
  }
  else if ( state == 3 ) // FINISHED
  {
    // Currently not used
    return;
  }
  else if ( state == 11 || state == 12 || state == 13 ) // SETTINGS
  {
    // Currently not used
    return;
  }
  else if ( state == 10 ) // MENU
  {
    handleOTA();
    if ( nextTempRead < millis() )
    {
      // Serial.println("loop menu");
      Serial.print(".");
      nextTempRead = millis() + tempSampleRate;
      // We show the current probe temp in the men screen just for info
      ReadCurrentTemp();
      if(currentTemp > 0) Serial.println((String)currentTemp);
      Serial.println(millis());
      #ifdef TC2
      ReadCurrentTempB();
      Serial.println((String)currentTempB);
      #endif

      String wifi = WiFi.status() == WL_CONNECTED ? (String)char(0x02) : (String)char(0x01);

      if ( currentTemp > 0 )
      {
        Serial.println("deviation: " + (String)(maxT-minT));
        // color code temperature
        if(currentTemp > hotTemp) tft.setTextColor( ORANGE, BLACK ); // hottemp color
        else if(currentTemp < coolTemp) tft.setTextColor( GREEN, BLACK ); // cooltemp color
        else tft.setTextColor( YELLOW, BLACK ); // caution color

        tft.setTextSize(textsize_2);
        int third = tft.width()/4;
        // println_Center( tft, "  "+String( round_f( currentTemp ) )+"c  ", tft.width() / 2, ( tft.height() / 2 ) + 10 );
        println_Center( tft, wifi + "  "+String( ( currentTemp ) )+"c  +/-" + (String)(maxT-minT), tft.width() / 2, ( tft.height() / 2 ) + 10 );
        
        #ifdef TC2
        if(currentTempB > 0) println_Center( tft, "  "+String( ( currentTempB ) )+"c", tft.width() / 2, ( tft.height() / 2 ) + 30 );
        else println_Center( tft, "errorB " + getTcStatusB(),tft.width() / 2,( tft.height() / 2 ) + 30 );
        #endif

        maxT = minT = currentTemp;
      }
      else{
        tft.setTextSize(textsize_2);
        tft.setTextColor( RED, BLACK );
        // println_Center( tft, "error " + getTcStatus(),tft.width() / 2,( tft.height() / 2 ) + 10 );
        println_Center( tft, wifi + " error " + (String)millis(),tft.width() / 2,( tft.height() / 2 ) -5 );
      }
    }
    ReadCurrentTemp();
    if(currentTemp>maxT) maxT = currentTemp;
    if(currentTemp<minT) minT = currentTemp;
    // Serial.println(minT);
    // Serial.println(maxT);
    return;
  }
  else if ( state == 15 )
  {
    // Currently not used
    return;
  }
  else if ( state == 16 ) // Oven Check
  {
    if ( nextTempRead < millis() )
    {
      nextTempRead = millis() + tempSampleRate;
  
      ReadCurrentTemp();

      MatchCalibrationTemp();
  
      if ( calibrationState < 2 )
      {
        tft.setTextColor( CYAN, BLACK );
        tft.setTextSize(textsize_2);

        if ( calibrationState == 0 )
        {
          if ( currentTemp < GetGraphValue(0) )
            println_Center( tft, "WARMING UP", tft.width() / 2, ( tft.height() / 2 ) - 15 );
          else
            println_Center( tft, "HEAT UP SPEED", tft.width() / 2, ( tft.height() / 2 ) - 15 );
            
          println_Center( tft, "TARGET "+String( GetGraphValue(1) )+"c in " + String( GetGraphTime(1) ) +"s", tft.width() / 2, ( tft.height() -18 ) );
        }
        else if ( calibrationState == 1 )
        {
          println_Center( tft, "COOL DOWN LEVEL", tft.width() / 2, ( tft.height() / 2 ) - 15 );
          tft.fillRect( 0, tft.height()-30, tft.width(), 30, BLACK );
        }


        // only show the timer when we have hit the profile starting temp
        if (currentTemp >= GetGraphValue(0) )
        {
          // adjust the timer colour based on good or bad values
          if ( calibrationState == 0 )
          {
            if ( calibrationSeconds <= GetGraphTime(1) )
              tft.setTextColor( WHITE, BLACK );
            else
              tft.setTextColor( ORANGE, BLACK );
          }
          else
          {
            tft.setTextColor( WHITE, BLACK );
          }
          
          tft.setTextSize(textsize_4);
          println_Center( tft, " "+String( calibrationSeconds )+" secs ", tft.width() / 2, ( tft.height() / 2 ) +20 );
        }
        tft.setTextSize(textsize_5);
        tft.setTextColor( YELLOW, BLACK );
        println_Center( tft, " "+String( round_f( currentTemp ) )+"c ", tft.width() / 2, ( tft.height() / 2 ) + 65 );

        
      }
      else if ( calibrationState == 2 )
      {
        calibrationState = 3;
        
        tft.setTextColor( GREEN, BLACK );
        tft.setTextSize(textsize_2);
        tft.fillRect( 0, (tft.height() / 2 ) - 45, tft.width(), (tft.height() / 2 ) + 45, BLACK );
        println_Center( tft, "RESULTS!", tft.width() / 2, ( tft.height() / 2 ) - 45 );

        tft.setTextColor( WHITE, BLACK );
        tft.setCursor( 20, ( tft.height() / 2 ) - 10 );
        tft.print( "RISE " );
        if ( calibrationUpMatch )
        {
          tft.setTextColor( GREEN, BLACK );
          tft.print( "PASS" );
        }
        else
        {
          tft.setTextColor( ORANGE, BLACK );
          tft.print( "FAIL " );
          tft.setTextColor( WHITE, BLACK );
          tft.print( "REACHED " + String( round_f(calibrationRiseVal * 100) ) +"%") ;
        }

        tft.setTextColor( WHITE, BLACK );
        tft.setCursor( 20, ( tft.height() / 2 ) + 20 );
        tft.print( "DROP " );
        if ( calibrationDownMatch )
        {
          tft.setTextColor( GREEN, BLACK );
          tft.print( "PASS" );
          tft.setTextColor( WHITE, BLACK );
          tft.print( "DROPPED " + String( round_f(calibrationDropVal * 100) ) +"%") ;
        }
        else
        {
          tft.setTextColor( ORANGE, BLACK );
          tft.print( "FAIL " );
          tft.setTextColor( WHITE, BLACK );
          tft.print( "DROPPED " + String( round_f(calibrationDropVal * 100) ) +"%") ;

          tft.setTextColor( WHITE, BLACK );
          tft.setCursor( 20, ( tft.height() / 2 ) + 40 );
          tft.print( "RECOMMEND ADDING FAN") ;
        }
      }
    }
  }
  else if(state == 20){
    testTC();
  }
  else // state is 2 - REFLOW
  {
    if ( nextTempAvgRead < millis() )
    {
      nextTempAvgRead = millis() + (tempSampleRate/avgSamples); // 10 avgs per second
      ReadCurrentTempAvg();
    }
    if ( nextTempRead < millis() )
    {
      nextTempRead = millis() + tempSampleRate;

      // Set the temp from the average
      currentTemp = ( currentTempAvg / avgReadCount );
      // clear the variables for next run
      avgReadCount = 0;
      currentTempAvg = 0;

      // Control the SSR
      MatchTemp();

      if ( currentTemp > 0 )
      {

        #ifdef TRACELOG 
        Serial.println(currentTemp);
        #endif

        bool useTimeXTimer = false;
        if(useTimeXTimer){
          // scale timer, update temperature display
          if(nextTimeX > millis()){
            float wantedTemp = CurrentGraph().wantedCurve[ (int)timeX+1 ];
            DrawHeading( "HOLD " + String( round_f( currentTemp ) ) + "/" + String( (int)wantedTemp )+"c", currentPlotColor, BLACK );          
            // Serial.println("Holding..");
            #ifdef DEBUG
                        tft.setCursor( 60, 40 );
                        tft.setTextSize(textsize_2);
                        tft.fillRect( 60, 40, 60, 20, BLACK );
                        tft.println( String( round_f((currentDuty/256) * 100 )) +"%" );
            #endif
            return; // HOLD timeX
          }

          timeX++; // increment timescale
          if(abs(timeX) >= GetGraphTime(2) && abs(timeX) < GetGraphTime(3)){ // timeX is a float?
            Serial.println("time scale is now minutes");
            nextTimeX = millis()+60000;
          }
          else nextTimeX = millis()+1000;
        }
        else timeX++; // increment timescale

        if ( timeX > CurrentGraph().completeTime )
        {
          EndReflow();
        }
        else 
        {
          Graph(tft, timeX, currentTemp, graphOff_x, graphOff_y, graph_w, graph_h ); // reflow - graph points same dimensions

          if ( timeX < CurrentGraph().fanTime )
          {
            float wantedTemp = CurrentGraph().wantedCurve[ (int)timeX ];
            DrawHeading( String( round_f( currentTemp ) ) + "/" + String( (int)wantedTemp )+"c", currentPlotColor, BLACK );

#ifdef DEBUG
            tft.setCursor( 60, 40 );
            tft.setTextSize(textsize_2);
            tft.fillRect( 60, 40, 80, 20, BLACK );
            tft.println( String( round_f((currentDuty/256) * 100 )) +"%" );
#endif
          }
        }
      }
    }
  }
}

// This is where the SSR is controlled via PWM
void SetRelayFrequency( int duty )
{
  // calculate the wanted duty based on settings power override
  currentDuty = ((float)duty * set.power );
  currentDuty = constrain( round_f( currentDuty ), 0, 255);
  currentDuty = 255-currentDuty; // invert

  #ifdef DEBUG
  if(duty>0) Serial.println("\n[SetRelayFrequency] " + (String)currentDuty);
  #endif
  // Write the clamped duty cycle to the RELAY GPIO
  analogWrite( RELAY, currentDuty );

  if(duty==0) return;

  #ifdef DEBUG
    // Serial.print("RELAY Duty Cycle: ");
    // Serial.println( "write: " + (String)currentDuty + " " + String( ( currentDuty / 256.0 ) * 100) + "%" +" Using Power: " + String( round_f( set.power * 100 )) + "%" );
  #endif
}

/*
 * SOME CALIBRATION CODE THAT IS CURRENTLY USED FOR THE OVEN CHECK SYSTEM
 * Oven Check currently shows you hoe fast your oven can reach the initial pre-soak temp for your selected profile
 */
 
void MatchCalibrationTemp()
{
  if ( calibrationState == 0 ) // temp speed
  {
    // Set SSR to full duty cycle - 100%
    SetRelayFrequency( 255 );

    // Only count seconds from when we reach the profile starting temp
    if ( currentTemp >= GetGraphValue(0) )
        calibrationSeconds ++;

    if ( currentTemp >= GetGraphValue(1) )
    {
#ifdef DEBUG
      Serial.println("Cal Heat Up Speed " + String( calibrationSeconds ) );
#endif

      calibrationRiseVal =  ( (float)currentTemp / (float)( GetGraphValue(1) ) );
      calibrationUpMatch = ( calibrationSeconds <= GetGraphTime(1) );


      calibrationState = 1; // cooldown
      SetRelayFrequency( 0 );
      StartFan( false );
      Buzzer( 2000, 50 );
    }  
  }
  else if ( calibrationState == 1 )
  {
    calibrationSeconds --;
    SetRelayFrequency( 0 );

    if ( calibrationSeconds <= 0 )                                                                                               
    {
      Buzzer( 2000, 50 );
#ifdef DEBUG
      Serial.println("Cal Cool Down Temp " + String( currentTemp ) );
#endif

      // calc calibration drop percentage value
      calibrationDropVal = ( (float)( GetGraphValue(1) - currentTemp ) / (float)GetGraphValue(1) );
      // Did we drop in temp > 33% of our target temp? If not, recomment using a fan!
      calibrationDownMatch = ( calibrationDropVal > 0.33 );

      calibrationState = 2; // finished
      StartFan( true );
    }
  }
}

/*
 * END
 * SOME CALIBRATION CODE THAT IS CURRENTLY USED FOR THE OVEN CHECK SYSTEM
 */


void ReadCurrentTempAvg()
{
  int status = tc.readError();  
  float internal = tc.readInternal();
  if(useInternal) currentTempAvg += internal + set.tempOffset;
  else currentTempAvg += tc.readCelsius() + set.tempOffset;
  avgReadCount++;

  #ifdef DEBUG
  // Serial.print(" avgtot: ");
  // Serial.print( currentTempAvg );
  // Serial.print(" avg count: ");
  // Serial.println( avgReadCount );
  // Serial.print(" avg: ");
  // Serial.println( currentTempAvg/avgReadCount );  
  #endif  
}

// Read the temp probe
void ReadCurrentTemp()
{
  int status = tc.readError();
  #ifdef DEBUG
  // Serial.print("tc status: ");
  // Serial.println( status );
  #endif
  float internal = tc.readInternal();
  if(useInternal) currentTemp = internal + set.tempOffset;
  else currentTemp = tc.readCelsius() + set.tempOffset;
}

#ifdef TC2
// Read the temp probe
void ReadCurrentTempB()
{
  int status = tcB.readError();
  #ifdef DEBUG
  // Serial.print("tc alt status: ");
  // Serial.println( status );
  #endif
  float internal = tcB.readInternal();
  if(useInternal) currentTempB = internal + set.tempOffset;
  else currentTempB = tcB.readCelsius() + set.tempOffset;
}
#endif

// This is where the magic happens for temperature matching
void MatchTemp()
{
  float duty = 0;
  float wantedTemp = 0;
  float wantedDiff = 0;
  float tempDiff = 0;
  float perc = 0;

  // if we are still before the main flow cut-off time (last peak)
  if ( timeX < CurrentGraph().offTime )
  {
    // We are looking XXX steps ahead of the ideal graph to compensate for slow movement of oven temp
    if ( timeX < CurrentGraph().reflowGraphX[2] ) // this assumes PEAK relow is idx 2 ?
      wantedTemp = CurrentGraph().wantedCurve[ (int)timeX + set.lookAheadWarm ];
    else
      wantedTemp = CurrentGraph().wantedCurve[ (int)timeX + set.lookAhead ];
    
    wantedDiff = (wantedTemp - lastWantedTemp );
    lastWantedTemp = wantedTemp;
    
    tempDiff = ( currentTemp - lastTemp ); 
    lastTemp = currentTemp;
    
    // prevent perc less than 1 and negative
    perc = wantedDiff - tempDiff;
    // perc = constrain(wantedDiff - tempDiff,0,100);
    if(perc < 1) perc = 0;

#ifdef DEBUG
    Serial.print( "T: " );
    Serial.print( timeX );
    
    Serial.print( "  Current: " );
    Serial.print( currentTemp );
    
    Serial.print( "  Wanted: " );
    Serial.print( wantedTemp );
    
    Serial.print( "  T Diff: " );
    Serial.print( tempDiff  );
    
    Serial.print( "  W Diff: " );
    Serial.print( wantedDiff );
    
    Serial.print( "  Perc: " );
    Serial.print( perc );
#endif
    
    isCuttoff = false;

    // have to passed the fan turn on time?
    if ( !isFanOn && timeX >= CurrentGraph().fanTime )
    {
#ifdef DEBUG
      Serial.print( "COOLDOWN: " );
#endif
      // If we are usng the fan, turn it on 
      if ( set.useFan )
      {
        isFanOn = true;
        DrawHeading( "COOLDOWN!", GREEN, BLACK );
        Buzzer( 1000, 2000 );
  
        StartFan ( true );
      }
      else // otherwise YELL at the user to open the oven door
      {
        if ( buzzerCount > 0 )
        {
          DrawHeading( "OPEN OVEN", RED, BLACK );
          Buzzer( 1000, 2000 );
          buzzerCount--;
        }
      }
    }
  }
  else
  {
    // YELL at the user to open the oven door
    if ( !isCuttoff && set.useFan )
    {
      DrawHeading( "OPEN OVEN", GREEN, BLACK );
      Buzzer( 1000, 2000 );

#ifdef DEBUG
      Serial.print( "CUTOFF: " );
#endif      
    }

    isFanOn = false;
    isCuttoff = true;
    StartFan ( false );
  }
  
  currentDetla = (wantedTemp - currentTemp);

#ifdef DEBUG
  Serial.print( "  Delta: " );
  Serial.print( currentDetla );
#endif
  
  float base = 128;
  
  if ( currentDetla >= 0 )
  {
      base = 128 + ( currentDetla * 5 ); // 128? swing?
  }
  else if ( currentDetla < 0 )
  {
      base = 32 + ( currentDetla * 15 ); // 32? swing?
  }

  base = constrain( base, 0, 256 );

#ifdef DEBUG
  Serial.print("  Base: ");
  Serial.print( base );
  // Serial.print( " -> " );
#endif
  
  duty = base + ( 172 * perc ); // 172?
#ifdef DEBUG
  Serial.print("  Duty: ");
  Serial.print( duty );
  Serial.print( " -> " );
#endif

  // if(duty<0)duty = 0;
  duty = constrain( duty, 0, 256 );

  // override for full blast at start
  // Serial.println("startFullBlast");
  // Serial.println(timeX);
  // Serial.println(CurrentGraph().reflowGraphX[1]);
  // if ( set.startFullBlast && (timeX < CurrentGraph().reflowGraphX[1]) ) duty = 256;
  if ( set.startFullBlast && timeX < CurrentGraph().reflowGraphX[1] && currentTemp < wantedTemp ) duty = 256;
  
  currentPlotColor = GREEN;

  SetRelayFrequency( duty );
}

void StartFan ( bool start )
{
#ifdef DEBUG
  Serial.print("***** FAN Use? ");
  Serial.print( set.useFan );
  Serial.print( " start? ");
  Serial.println( start );
#endif
  
  if ( set.useFan )
  {
    // digitalWrite ( FAN, ( start ? HIGH : LOW ) );
  }
}

void DrawHeading( String lbl, unsigned int acolor, unsigned int bcolor )
{
    tft.setTextSize(textsize_3);
    tft.setTextColor(acolor , bcolor);
    tft.setCursor(0,0);
    tft.fillRect( 0, 0, 130, 30, BLACK );
    tft.println( String(lbl) );
}

// buzz he buzzer
void Buzzer( int hertz, int len )
{
   // tone( BUZZER, hertz, len); // @todo fix, causes a problem somehow
}


double ox , oy ; // @todo what are these?

void DrawBaseGraph()
{
  oy = 220;
  ox = 30;
  timeX = 0;
  
  for( int ii = 0; ii <= graphRangeMax_X; ii+= 5 )
  {
    GraphDefault(tft, ii, CurrentGraph().wantedCurve[ii], graphOff_x, graphOff_y, graph_w, graph_h, PINK ); // graph reflow curve
    delay(0);
  }

  ox = 30;
  oy = 220;
  timeX = 0;
}

void BootScreen()
{
  tft.fillScreen(BLACK);

  tft.setTextColor( GREEN, BLACK );
  tft.setTextSize(textsize_1);
  println_Center( tft, "REFLOW MASTER", tft.width() / 2, ( tft.height() / 2 ) - 8 );

  tft.setTextColor( WHITE, BLACK );
  println_Center( tft, "unexpectedmaker.com", tft.width() / 2, ( tft.height() / 2 ) + 20 );
  println_Center( tft, "PCB v2018-2, Code v" + ver, tft.width() / 2, tft.height() - 20 );

  state = 10;
}

void ShowMenu()
{
  #ifdef DEBUG
  Serial.println("ShowMenu");
  #endif

  state = 10;
  SetRelayFrequency( 0 );

  // set = flash_store.read();
  #ifdef DEBUG
    Serial.println("TFT Show Menu");
  #endif  
  tft.fillScreen(BLACK);
  Serial.println("clear display");
  
  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(textsize_2);
  tft.setCursor( 20, 20 );
  tft.println( "CURRENT PASTE" );
  Serial.println("disp curr paste");
  // delay(1000);

  tft.setTextColor( YELLOW, BLACK );
  tft.setCursor( 20, 40 ); 
  tft.println( CurrentGraph().n );
  tft.setCursor( 20, 60 );
  tft.println( String(CurrentGraph().tempDeg) +"deg");
  Serial.println("disp temperature");

  // causes crashes also, this makes no sense
  if ( newSettings && settingWarn )
  {
    tft.setTextColor( CYAN, BLACK );
    println_Center( tft, "Settings Stomped!!", tft.width() / 2, tft.height() - 80 );
  }

  tft.setTextSize(textsize_1);
  tft.setTextColor( WHITE, BLACK );
    tft.setCursor( 54,152 );
    // tft.println( "Settings Stomped!!" );
    // tft.setCursor( 45,216 );
    // tft.println( "Reflow Master - PCB v2018-2, Code v1.03" );  
    println_Center( tft, "Reflow Master - PCB v2018-2, Code v" + ver, tft.width() / 2, tft.height() - 20 );
    // println_Center( tft, "Reflow Master - PCB v2018-2, Code v" + (String)ver, 80,110);
    // Serial.println("SET FAULT");
    // Serial.flush();
    // setFault(tft);

  ShowMenuOptions( true ); // causes crashes why!!!!!
  Serial.println("showmenu done");
}

void ShowSettings()
{
  state = 11;
  SetRelayFrequency( 0 );

  newSettings = false;

  int posY = 50;
  int incY = 20;

  tft.setTextColor( BLUE, BLACK );
  tft.fillScreen(BLACK);


  tft.setTextColor( BLUE, BLACK );
  tft.setTextSize(textsize_2);
  tft.setCursor( 20, 20 );
  tft.println( "SETTINGS" );

  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, posY );
  tft.print( "SWITCH PASTE" );

  posY += incY;

  // Fan is y 70
  UpdateSettingsFan( posY );

  posY += incY;

  // Power is 90
  UpdateSettingsPower( posY );

  posY += incY;

  UpdateSettingsLookAhead( posY );
  
  posY += incY;

  UpdateSettingsTempOffset( posY );
  
  posY += incY;

  UpdateSettingsStartFullBlast( posY );
  
  posY += incY;

  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, posY );
  tft.print( "RESET TO DEFAULTS" );

  posY += incY;
  
  ShowMenuOptions( true );
}

void ShowPaste()
{
  state = 12;
  SetRelayFrequency( 0 );

  tft.fillScreen(BLACK);

  tft.setTextColor( BLUE, BLACK );
  tft.setTextSize(textsize_2);
  tft.setCursor( 20, 20 );
  tft.println( "SWITCH PASTE" );

  tft.setTextColor( WHITE, BLACK );

  int y = 50;

  for( int i = 0; i < ELEMENTS(solderPaste); i++ )
  {
      if ( i == set.paste )
        tft.setTextColor( YELLOW, BLACK );
      else
        tft.setTextColor( WHITE, BLACK );

      tft.setTextSize(textsize_2);
      tft.setCursor( 20, y );

      tft.println( String( solderPaste[i].tempDeg) +"d " + solderPaste[i].n );
      tft.setTextSize(textsize_1);
      tft.setCursor( 20, y + 17 );
      tft.println( solderPaste[i].t );
      tft.setTextColor( GREY, BLACK );
      
      y+= 40;
      delay(0);
  }

   ShowMenuOptions( true );
}

void selectMenu(){
  Serial.println("menuSelect: " + (String)menuSel);
       if(menuSel == 1) button0Press();
  else if(menuSel == 2) button1Press();
  else if(menuSel == 3) button2Press();
  else if(menuSel == 4) button3Press();
  menuSel = 0; // reset
}

void ShowMenuOptions( bool clearAll )
{
    #ifdef DEBUG
    Serial.println("ShowMenuOptions");
    #endif  
  int buttonPosY[] = { 19, 74, 129, 184 }; // @todo use auto height 55px
  int buttonHeight = 16;
  int buttonWidth = 4;

  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(textsize_2);

  if ( clearAll )
  {
    // Clear All
    for ( int i = 0; i < 4; i++ ){
      tft.fillRect( tft.width()-100,  buttonPosY[i]-2, 100, buttonHeight+4, BLACK );
      delay(0);
    }
    // menuSel = 0; 
  }
  if ( state == 10 )
  {
    menuCount = 4;
    if(menuSel > 0){
      Serial.println("menusel: " + (String)menuSel);
      menuSelY = buttonPosY[menuSel-1] + 8;
    }

    // button 0

    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "START", tft.width()- 27, buttonPosY[0] + 8 );

     // button 1
    tft.fillRect( tft.width()-5,  buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "SETTINGS", tft.width()- 27, buttonPosY[1] + 8 );

     // button 2
    tft.fillRect( tft.width()-5,  buttonPosY[2], buttonWidth, buttonHeight, BLUE );
    println_Right( tft, "ITEM 3", tft.width()- 27, buttonPosY[2] + 8 );

    // button 3
    tft.fillRect( tft.width()-5,  buttonPosY[3], buttonWidth, buttonHeight, YELLOW );
    println_Right( tft, "OVEN CHECK", tft.width()- 27, buttonPosY[3] + 8 );

    return;
  }
  else if ( state == 11 )
  {
    menuCount += 4;
    // button 0
    tft.fillRect( tft.width()-100,  buttonPosY[0]-2, 100, buttonHeight+4, BLACK );
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    switch ( settings_pointer )
      {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
          println_Right( tft, "CHANGE", tft.width()- 27, buttonPosY[0] + 8 );
          break;

        default:
          println_Right( tft, "SELECT", tft.width()- 27, buttonPosY[0] + 8 );
          break;
      }
  
     // button 1
    tft.fillRect( tft.width()-5,  buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "BACK", tft.width()- 27, buttonPosY[1] + 8 );

    // button 2
    tft.fillRect( tft.width()-5,  buttonPosY[2], buttonWidth, buttonHeight, BLUE );
    println_Right( tft, "/\\", tft.width()- 27, buttonPosY[2] + 8 );

    // button 3
    tft.fillRect( tft.width()-5,  buttonPosY[3], buttonWidth, buttonHeight, YELLOW );
    println_Right( tft, "\\/", tft.width()- 27, buttonPosY[3] + 8 );

    UpdateSettingsPointer();
  }
  else if ( state == 12 )
  {
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "SELECT", tft.width()- 27, buttonPosY[0] + 8 );

     // button 1
    tft.fillRect( tft.width()-5,  buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "BACK", tft.width()- 27, buttonPosY[1] + 8 );

    // button 2
    tft.fillRect( tft.width()-5,  buttonPosY[2], buttonWidth, buttonHeight, BLUE );
    println_Right( tft, "/\\", tft.width()- 27, buttonPosY[2] + 8 );

    // button 3
    tft.fillRect( tft.width()-5,  buttonPosY[3], buttonWidth, buttonHeight, YELLOW );
    println_Right( tft, "\\/", tft.width()- 27, buttonPosY[3] + 8 );

    UpdateSettingsPointer();
  }
  else if ( state == 13 ) // restore settings to default
  {
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "YES", tft.width()- 27, buttonPosY[0] + 8 );
  
     // button 1
    tft.fillRect( tft.width()-5,  buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "NO", tft.width()- 27, buttonPosY[1] + 8 );
  }
  else if ( state == 1 || state == 2 || state == 16 ) // warmup, reflow, calibration
  {
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "ABORT", tft.width()- 27, buttonPosY[0] + 8 );
  }
  else if ( state == 3 ) // Finished
  {
     tft.fillRect( tft.width()-100,  buttonPosY[0]-2, 100, buttonHeight+4, BLACK );
    
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "MENU", tft.width()- 27, buttonPosY[0] + 8 );
  }
  else if ( state == 15 )
  {
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "START", tft.width()- 27, buttonPosY[0] + 8 );
  
     // button 1
    tft.fillRect( tft.width()-5,  buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "BACK", tft.width()- 27, buttonPosY[1] + 8 );
  } 
}

void UpdateSettingsPointer()
{
  if ( state == 11 )
  {
      tft.setTextColor( BLUE, BLACK );
      tft.setTextSize(textsize_2);
      tft.fillRect( 0, 20, 20, tft.height()-20, BLACK );
      tft.setCursor( 5, ( 50 + ( 20 * settings_pointer ) ) );
      tft.println(">");

      tft.setTextSize(textsize_1);
      tft.setTextColor( GREEN, BLACK );
      tft.fillRect( 0, tft.height()-40, tft.width(), 40, BLACK );
      switch ( settings_pointer )
      {
          case 0:
          println_Center( tft, "Select which profile to reflow", tft.width() / 2, tft.height() - 20 );
          break;

          case 1:
          println_Center( tft, "Enable fan for end of reflow, requires 5V DC fan", tft.width() / 2, tft.height() - 20 );
          break;

          case 2:
          println_Center( tft, "Adjust the power boost", tft.width() / 2, tft.height() - 20 );
          break;

          case 3:
          println_Center( tft, "Soak and Reflow look ahead for rate change speed", tft.width() / 2, tft.height() - 20 );
          break;

          case 4:
          println_Center( tft, "Adjust temp probe reading offset                                                                                               ", tft.width() / 2, tft.height() - 20 );
          break;

          case 5:
          println_Center( tft, "Force full power on initial ramp-up - be careful!", tft.width() / 2, tft.height() - 20 );
          break;

          case 6:
          println_Center( tft, "Reset to default settings", tft.width() / 2, tft.height() - 20 );
          break;

          default:
          //println_Center( tft, "", tft.width() / 2, tft.height() - 20 );
          break;
      }
      tft.setTextSize(textsize_2);
  }
  else if ( state == 12 )
  {
      tft.setTextColor( BLUE, BLACK );
      tft.setTextSize(textsize_2);
      tft.fillRect( 0, 20, 20, tft.height()-20, BLACK );
      tft.setCursor( 5, ( 50 + ( 20 * ( settings_pointer * 2 ) ) ) );
      tft.println(">");
  }
}

void ProcessReflow(){
// @todo move loop process here
}

void StartWarmup()
{
  tft.fillScreen(BLACK);

  state = 1;
  timeX = 0;
  ShowMenuOptions( true );
  lastWantedTemp = -1;
  buzzerCount = 5;

  tft.setTextColor( GREEN, BLACK );
  tft.setTextSize(textsize_3);
  println_Center( tft, "WARMING UP", tft.width() / 2, ( tft.height() / 2 ) - 30 );

  tft.setTextColor( WHITE, BLACK );
  println_Center( tft, "START @ " + String( GetGraphValue(0) ) +"c", tft.width() / 2, ( tft.height() / 2 ) + 50 );
}

void StartReflow()
{
  tft.fillScreen(BLACK);

  state = 2;
   ShowMenuOptions( true );
  
  timeX = 0;
  // draw grapher, no title
  SetupGraph(tft, 0, 0, graphOff_x, graphOff_y, graph_w, graph_h, graphRangeMin_X, graphRangeMax_X, graphRangeStep_X, graphRangeMin_Y, graphRangeMax_Y, graphRangeStep_Y, "", " Time (s)", "deg (C)", grid_color, axis_color, axis_text_color, BLACK );
  
  DrawHeading( "READY", WHITE, BLACK );
  DrawBaseGraph(); // draw reflow graph
}

void AbortReflow()
{
  if ( state == 1 || state == 2 || state == 16 ) // if we are in warmup or reflow states
  {
    SetRelayFrequency(0);
    state = 99;
    DrawHeading( "ABORT", RED, BLACK );

    StartFan( false );

    delay(1000);

    state = 10;
    ShowMenu();
  }
}

void EndReflow()
{
  if ( state == 2 )
  {
    SetRelayFrequency( 0 );
    state = 3;

    Buzzer( 1000, 1000 );

    DrawHeading( "DONE!", WHITE, BLACK );

    ShowMenuOptions( false );

    StartFan( false );
  }
}


void SetDefaults()
{
  // Default settings values
  set.valid = true;
  set.power = 1;
  set.paste = 4;
  set.useFan = true;
  set.lookAhead = 2;
  set.lookAheadWarm = 2;
  set.startFullBlast = true;
  set.tempOffset = 0;
}

void ResetSettingsToDefault()
{
  // set default values again and save
  SetDefaults();
  // flash_store.write(set);

  // load the default paste
  SetCurrentGraph( set.paste );

  // show settings
  settings_pointer = 0;
  ShowSettings();
}


/*
 * OVEN CHECK CODE NOT CURRENTLY USED
 * START
 */
void StartOvenCheck()
{
#ifdef DEBUG
  Serial.println("Oven Check Start Temp " + String( currentTemp ) );
#endif
  state = 16;
  calibrationSeconds = 0;
  calibrationState = 0;
  calibrationStatsUp = 0;
  calibrationStatsDown = 300;
  calibrationUpMatch = false;
  calibrationDownMatch = false;
  calibrationDropVal = 0;
  calibrationRiseVal = 0;
  SetRelayFrequency( 0 );
  StartFan( false );

#ifdef DEBUG
  Serial.println("Running Oven Check");
#endif
  tft.fillScreen(BLACK);
  tft.setTextColor( CYAN, BLACK );
  tft.setTextSize(textsize_2);
  tft.setCursor( 20, 20 );
  tft.println( "OVEN CHECK" );

  tft.setTextColor( YELLOW, BLACK );
  tft.setCursor( 20, 40 ); 
  tft.println( CurrentGraph().n );
  tft.setCursor( 20, 60 );
  tft.println( String(CurrentGraph().tempDeg) +"deg");

  ShowMenuOptions( true );
}

void ShowOvenCheck()
{
  state = 15;
  SetRelayFrequency( 0 );
  StartFan( true );

  int posY = 50;
  int incY = 20;

#ifdef DEBUG
  Serial.println("Oven Check");
#endif
  
  tft.fillScreen(BLACK);
  tft.setTextColor( CYAN, BLACK );
  tft.setTextSize(textsize_2);
  tft.setCursor( 20, 20 );
  tft.println( "OVEN CHECK" );


  tft.setTextColor( WHITE, BLACK );

  ShowMenuOptions( true );

  tft.setTextSize(textsize_2);
  tft.setCursor( 0, 60 );
  tft.setTextColor( YELLOW, BLACK );

  tft.println( " Oven Check allows");
  tft.println( " you to measure your");
  tft.println( " oven's rate of heat");
  tft.println( " up & cool down to");
  tft.println( " see if your oven");
  tft.println( " is powerful enough");
  tft.println( " to reflow your ");
  tft.println( " selected profile.");
  tft.println( "");

  tft.setTextColor( YELLOW, RED );
  tft.println( " EMPTY YOUR OVEN FIRST!");
}

/*
 * END
 * OVEN CHECK CODE NOT CURRENTLY USED/WORKING
 */


/*
 * Lots of UI code here
 */

void ShowResetDefaults()
{
  tft.fillScreen(BLACK);
  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(textsize_2);
  tft.setCursor( 20, 90 );
      
  tft.setTextColor( WHITE, BLACK );
  tft.print( "RESET SETTINGS" );
  tft.setTextSize(textsize_3);
  tft.setCursor( 20, 120 );
  tft.println( "ARE YOU SURE?" );

  state = 13;
  ShowMenuOptions( false );

  tft.setTextSize(textsize_1);
  tft.setTextColor( GREEN, BLACK );
  tft.fillRect( 0, tft.height()-40, tft.width(), 40, BLACK );
  println_Center( tft, "Settings restore cannot be undone!", tft.width() / 2, tft.height() - 20 );
}
 
void UpdateSettingsFan( int posY )
{
  tft.fillRect( 15,  posY-5, 200, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, posY );

  tft.setTextColor( WHITE, BLACK );
  tft.print( "USE FAN " );
  tft.setTextColor( YELLOW, BLACK );
    
  if ( set.useFan )
  {
    tft.println( "ON" );
  }
  else
  {
    tft.println( "OFF" );
  }
  tft.setTextColor( WHITE, BLACK );
}


void UpdateSettingsStartFullBlast( int posY )
{
  tft.fillRect( 15,  posY-5, 240, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, posY );
  tft.print( "START RAMP 100% " );
  tft.setTextColor( YELLOW, BLACK );
    
  if ( set.startFullBlast )
  {
    tft.println( "ON" );
  }
  else
  {
    tft.println( "OFF" );
  }
  tft.setTextColor( WHITE, BLACK );
}

void UpdateSettingsPower( int posY )
{
  tft.fillRect( 15,  posY-5, 240, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );
  
  tft.setCursor( 20, posY );
  tft.print( "POWER ");
  tft.setTextColor( YELLOW, BLACK );
  tft.println( String( round_f((set.power * 100))) +"%");
  tft.setTextColor( WHITE, BLACK );
}

void UpdateSettingsLookAhead( int posY )
{
  tft.fillRect( 15,  posY-5, 260, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );
  
  tft.setCursor( 20, posY );
  tft.print( "GRAPH LOOK AHEAD ");
  tft.setTextColor( YELLOW, BLACK );
  tft.println( String( set.lookAhead) );
  tft.setTextColor( WHITE, BLACK );
}

//void UpdateSettingsLookAheadWarm( int posY )
//{
//  tft.fillRect( 15,  posY-5, 220, 20, BLACK );
//  tft.setTextColor( WHITE, BLACK );
//  
//  tft.setCursor( 20, posY );
//  tft.print( "GRAPH LOOK AHEAD  ");
//  tft.setTextColor( YELLOW, BLACK );
//  tft.println( String( set.lookAheadWarm) );
//  tft.setTextColor( WHITE, BLACK );
//}

void UpdateSettingsTempOffset( int posY )
{
  tft.fillRect( 15,  posY-5, 220, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );
  
  tft.setCursor( 20, posY );
  tft.print( "TEMP OFFSET ");
  tft.setTextColor( YELLOW, BLACK );
  tft.println( String( set.tempOffset) );
  tft.setTextColor( WHITE, BLACK );
}



/*
 * Button press code here
 */

long nextButtonPress = 0;

void button0Press()
{
  Serial.println("BUTTON PRESS 0");
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20; 
    Buzzer( 2000, 50 );
    
    if ( state == 10 )
    {
      // StartWarmup();
      StartReflow();
    }
    else if ( state == 1 || state == 2 || state == 16 )
    {
      AbortReflow();
    }
    else if ( state == 3 )
    {
      ShowMenu();
    }
    else if ( state == 11 )
    {
      if ( settings_pointer == 0 )  // change paste
      {
        settings_pointer = set.paste;
        ShowPaste();
      }
      else if ( settings_pointer == 1 )  // switch fan use
      {
        set.useFan = !set.useFan;

        UpdateSettingsFan( 70 );
      }
      else if ( settings_pointer == 2 ) // change power
      {
        set.power += 0.1;
        if ( set.power > 1.55 )
          set.power = 0.5;

        UpdateSettingsPower( 90 );
      }
      else if ( settings_pointer == 3 ) // change lookahead for reflow
      {
        set.lookAhead += 1;
        if ( set.lookAhead > 15 )
          set.lookAhead = 1;
        UpdateSettingsLookAhead( 110 );
      }
      else if ( settings_pointer == 4 ) // change temp probe offset
      {
        set.tempOffset += 1;
        if ( set.tempOffset > 15 )
          set.tempOffset = -15;

        UpdateSettingsTempOffset( 130 );
      }
      else if ( settings_pointer == 5 ) // change use full power on initial ramp
      {
        set.startFullBlast = !set.startFullBlast;

        UpdateSettingsStartFullBlast( 150 );
      }
      else if ( settings_pointer == 6 ) // reset defaults
      {
        ShowResetDefaults();
      }
    }
    else if ( state == 12 )
    {
      if ( set.paste != settings_pointer )
      {
        set.paste = settings_pointer;
        SetCurrentGraph( set.paste );
        ShowPaste();
      }
    }
    else if ( state == 13 )
    {
      ResetSettingsToDefault();
    }
    else if ( state == 15 )
    {
      StartOvenCheck();
    }
  }
}

void button1Press()
{
  Serial.println("BUTTON PRESS 1");
  bool res = anyButton();
  if(res) return;

  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    Buzzer( 2000, 50 );

    if ( state == 10 )
    {
      settings_pointer = 0;
      ShowSettings();
    }
    else if ( state == 11 ) // leaving settings so save
    {
      // save data in flash
      // flash_store.write(set);
      ShowMenu();
    }
    else if ( state == 12 || state == 13 )
    {
      settings_pointer = 0;
      ShowSettings();
    }
    else if ( state == 15 ) // cancel oven check
    {
      ShowMenu();
    }
  }
}

void button2Press()
{
  Serial.println("BUTTON PRESS 2");
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    Buzzer( 2000, 50 );
    if(state == 10) state = 20;
    if ( state == 11 )
    {
      settings_pointer = constrain( settings_pointer -1, 0, 6 );
      ShowMenuOptions( false );
      //UpdateSettingsPointer();
    }
    else if ( state == 12 )
    {
      settings_pointer = constrain( settings_pointer -1, 0, ELEMENTS(solderPaste)-1 );
      UpdateSettingsPointer();
    }
  }
}


void button3Press()
{
  Serial.println("BUTTON PRESS 3");
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    Buzzer( 2000, 50 );

    if ( state == 10 )
    {
      ShowOvenCheck();
    }
    else if ( state == 11 )
    {
      settings_pointer = constrain( settings_pointer +1, 0, 6 );
      ShowMenuOptions( false );
      //UpdateSettingsPointer();
    }
    else if ( state == 12 )
    {
      settings_pointer = constrain( settings_pointer +1, 0, ELEMENTS(solderPaste)-1 );
      UpdateSettingsPointer();
    }
  }
}


/*
 * Graph drawing code here
 * Special thanks to Kris Kasprzak for his free graphing code that I derived mine from
 * https://www.youtube.com/watch?v=YejRbIKe6e0
 */

/*

  function to draw a cartesian coordinate system and plot whatever data you want
  just pass x and y and the graph will be drawn

  huge arguement list
  &d name of your display object
  x = x data point
  y = y datapont
  gx = x graph location (lower left)
  gy = y graph location (lower left)
  w = width of graph
  h = height of graph
  xlo = lower bound of x axis
  xhi = upper bound of x asis
  xinc = division of x axis (distance not count)
  ylo = lower bound of y axis
  yhi = upper bound of y asis
  yinc = division of y axis (distance not count)
  title = title of graph
  xlabel = x asis label
  ylabel = y asis label
  gcolor = graph line colors
  acolor = axi ine colors
  pcolor = color of your plotted data
  tcolor = text color
  bcolor = background color
  &redraw = flag to redraw graph on fist call only
*/

// Set width and value size, so user does not have to use offsets to width and offset
// 

int graphW = 240;
int graphH = 240; //-headerheight
int labelXW = 30;
int labelYH = 10;

void SetupGraph(RM_tft &d, double x, double y, double gx, double gy, double w, double h, double xlo, double xhi, double xinc, double ylo, double yhi, double yinc, String title, String xlabel, String ylabel, unsigned int gcolor, unsigned int acolor, unsigned int tcolor, unsigned int bcolor )
{
  double ydiv, xdiv;
  double i;
  int temp;
  int rot, newrot;

    ox = (x - xlo) * ( w) / (xhi - xlo) + gx;
    oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
    // draw y scale
    for ( i = ylo; i <= yhi; i += yinc)
    {
      // compute the transform
      temp =  (i - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

      if (i == ylo) {
        d.drawLine(gx, temp, gx + w, temp, acolor);
      }
      else {
        d.drawLine(gx, temp, gx + w, temp, gcolor);
      }

      d.setTextSize(textsize_1);
      d.setTextColor(tcolor, bcolor);
      d.setCursor(gx-25, temp); // margin left + pading
      println_Right( d, String(round_f(i)), gx-25, temp );
      delay(0);
    }
    
    // draw x scale
    for (i = xlo; i <= xhi; i += xinc)
    {
      temp =  (i - xlo) * ( w) / (xhi - xlo) + gx;
      if (i == 0)
      {
        d.drawLine(temp, gy, temp, gy - h, acolor);
      }
      else
      {
        d.drawLine(temp, gy, temp, gy - h, gcolor);
      }

      d.setTextSize(textsize_1);
      d.setTextColor(tcolor, bcolor);
      d.setCursor(temp, gy + 10); // margin bottom + padding

      if ( i <= xhi - xinc )
        println_Center(d, String(round_f(i)), temp, gy + 10 );
      else
        println_Center(d, String(round_f(xhi)), temp, gy + 10 );
        
      delay(0);
    }

    //now draw the labels
    d.setTextSize(textsize_2);
    d.setTextColor(tcolor, bcolor);
    d.setCursor(gx , gy - h - 30); // y axis label
    d.println(title);

    d.setTextSize(textsize_1);
    d.setTextColor(acolor, bcolor);
    d.setCursor(w - 25 , gy - 10); // x axis label
    d.println(xlabel);

    tft.setRotation(rotation-1); // fix this proper
    d.setTextSize(textsize_1);
    d.setTextColor(acolor, bcolor);
    d.setCursor(w - 116, 34 );
    d.println(ylabel);
    tft.setRotation(rotation);
    delay(0);
}

void Graph(RM_tft &d, double x, double y, double gx, double gy, double w, double h )
{
  // recall that ox and oy are initialized as static above
  x =  (x - graphRangeMin_X) * ( w) / (graphRangeMax_X - graphRangeMin_X) + gx;
  y =  (y - graphRangeMin_Y) * (gy - h - gy) / (graphRangeMax_Y - graphRangeMin_Y) + gy;
  
  if ( timeX < 2 )
    oy = min( oy, y );
    
  y = min( (int)y, 220 ); // bottom of graph

 d.fillRect( ox-1, oy-1, 3, 3, currentPlotColor );

  d.drawLine(ox, oy + 1, x, y + 1, currentPlotColor);
  d.drawLine(ox, oy - 1, x, y - 1, currentPlotColor);
  d.drawLine(ox, oy, x, y, currentPlotColor );
  ox = x;
  oy = y;
}

void GraphDefault(RM_tft &d, double x, double y, double gx, double gy, double w, double h, unsigned int pcolor )
{
  // recall that ox and oy are initialized as static above
  x =  (x - graphRangeMin_X) * ( w) / (graphRangeMax_X - graphRangeMin_X) + gx;
  y =  (y - graphRangeMin_Y) * (gy - h - gy) / (graphRangeMax_Y - graphRangeMin_Y) + gy;

  //Serial.println( oy );
  d.drawLine(ox, oy, x, y, pcolor);
  d.drawLine(ox, oy + 1, x, y + 1, pcolor);
  d.drawLine(ox, oy - 1, x, y - 1, pcolor);
  ox = x;
  oy = y;
}

char* string2char(String command)
{
    if(command.length()!=0)
    {
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
    return 0;
}

void println_Center( RM_tft &d, String heading, int centerX, int centerY )
{
    int x = 0;
    int y = 0;
    int16_t  x1, y1;
    uint16_t ww, hh;

    d.getTextBounds( string2char(heading), x, y, &x1, &y1, &ww, &hh );
    // Serial.println("println_center");
    // Serial.println(heading);
    // Serial.println(centerX);
    // Serial.println(centerY);
    // Serial.println(x1);
    // Serial.println(y1);
    // Serial.println(ww);
    // Serial.println(hh);

    // Serial.println( centerX - ww/2 + 2);
    // Serial.println( centerY - hh / 2);

    d.setCursor( centerX - ww/2 + 2, centerY - hh / 2);
    d.println( heading );
    // d.setCursor( 160 - 216/2 + 2, 160 - 16 / 2);
    // Serial.println(160 - 216/2 + 2);
    // Serial.println(160 - 16 / 2);
    
    // d.setCursor( 54,152 );
    // d.println( "Settings Stomped!!" );
    
    // d.setCursor( 45,216 );
    // d.println( "1234567890" );
    // d.println( "Reflow Master - PCB v2018-2, Code v1.03" );
}

void setFault(RM_tft &d){
  d.setCursor(45,216);
}

void println_Right( RM_tft &d, String heading, int centerX, int centerY )
{
    int x = 0;
    int y = 0;
    int16_t  x1, y1;
    uint16_t ww, hh;

    // Serial.println(centerY);
    // Serial.println(menuSelY);
    if(menuSel > 0 && menuSelY == centerY) heading = (String)char(0x10)+(String)" "+(String)heading;
    else heading = "  " + heading; // hack space
    // Serial.println(heading);
    d.getTextBounds( string2char(heading), x, y, &x1, &y1, &ww, &hh );
    d.setCursor( centerX + ( 18 - ww ), centerY - hh / 2);
    d.println( heading );
}

void initWiFi(){

}

void initTFT(){

  #ifdef DEBUG
    Serial.println("TFT Begin...");
  #endif

  #ifdef USE_ADAFRUIT_ST7735
    tft.initR(INITR_144GREENTAB); 
  #elif defined(USE_ADAFRUIT_ST7789)
    tft.init(240, 240, SPI_MODE3);           // Init ST7789 240x240
  #else
    // Start up the TFT and show the boot screen
    tft.begin();
  #endif
    tft.setRotation(rotation);
    // tft.setFont(&FreeMono9pt7b);
    
    BootScreen();

  #ifdef DEBUG
    Serial.println("Booted Spash Screen");
  #endif

}

void initTC(){
  // Start up the MAX31855
  // @todo sanity
  tc.begin();
  tc.readError();
  #ifdef DEBUG
    Serial.println("Thermocouple Begin...");
    Serial.println((String)round(tc.readInternal()));
    Serial.println((String)round(tc.readCelsius()));
  #endif
}

void initButtons(){

  // clickEncoder library
  // #define ENC_DECODER ENC_FLAKY
  // #define ENC_HALFSTEP 0 

  // encoder, not using button here
  // encoder.setButtonHeldEnabled(true);
  // encoder.setDoubleClickEnabled(true);
  // encoder.setButtonOnPinZeroEnabled(true);
  encoder.setAccelerationEnabled(false);

  Serial.print("Encoder Acceleration is ");
  Serial.println((encoder.getAccelerationEnabled()) ? "enabled" : "disabled");  
}

void setup()
{

// #ifdef DEBUG
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
// #endif

  init_indicator(2);

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(SSID,PASS);
  Serial.println("Connecting to wifi....");
  
  while(WiFi.status() != WL_CONNECTED || millis() < 5000){
    Serial.print(".");
    delay(200);
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    indSetColor(0,255,0);
  }
  else{
    Serial.println("NOT CONNECTED");
    indSetColor(255,0,0);
  }
  delay(500);

  // pinMode(TFT_CS,OUTPUT);
  // digitalWrite(TFT_CS, HIGH);
  
  // pinMode(MAXCS,OUTPUT);
  // digitalWrite(MAXCS, HIGH);

  // Setup all GPIO
  // pinMode( BUZZER, OUTPUT );

  digitalWrite(RELAY,HIGH);
  pinMode( RELAY, OUTPUT );

  // pinMode( FAN, OUTPUT );
  
  // pinMode( BUTTON0, INPUT );
  // pinMode( BUTTON1, INPUT );
  // pinMode( BUTTON2, INPUT );
  // pinMode( BUTTON3, INPUT );

  analogWriteRange(255); // esp8266 
  analogWriteFreq(120); // min 100hz
  // Turn off the SSR - duty cycle of 0
  SetRelayFrequency( 255 ); // test pulse
  delay(1000);
  SetRelayFrequency( 0 );

  init_ota();

  Serial.println(ESP.getFreeHeap());
  Serial.println(ESP.getHeapFragmentation());
  Serial.println(ESP.getMaxFreeBlockSize());

  initTFT();  
  initTC();  
  initButtons();

  // just wait a bit before we try to load settings from FLASH
  delay(500);

  // load settings from FLASH
  // set = flash_store.read();

  // If no settings were loaded, initialise data and save
  if ( !set.valid )
  {
    SetDefaults();
    newSettings = true;
    // flash_store.write(set);
  }

  // Attatch button IO for OneButton
  // button0.attachClick(button0Press);
  // button1.attachClick(button1Press);
  // button2.attachClick(button2Press);
  // button3.attachClick(button3Press);
  
// digitalWrite(TFT_CS,HIGH);

  // delay for initial temp probe read to be garbage
  delay(500);
  state = 0;
}

void processEncoder(){
  static uint32_t lastService = 0;
  if (micros() - lastService >= 1000) {
    lastService = micros();                
    encoder.service();  
  }
 int numsteps = 4;

  static int16_t last, value;
  value += encoder.getValue();
  // if(value!=0)Serial.print(value);
  
  if (abs(value - last) >= numsteps) {
  // if (value != last) {
    Serial.print("Encoder Value: ");
    Serial.print(value);
    Serial.print(" Dir: ");
    Serial.print("\t");
    Serial.print(" Steps: ");
    Serial.print((value-last)/numsteps);
    Serial.print("\t");
    Serial.println((value > last) ? "10" : "20");

    if(value > last)menuSel++;
    if(value < last)menuSel--;
    // if(menuSel < 1) menuSel = 1; // end stops
    // if(menuSel > menuCount) menuSel = menuCount; // end stops
    if(menuSel < 1) menuSel = menuCount; // wrap
    if(menuSel > menuCount) menuSel = 1; //wrap
    last = value;

    ShowMenuOptions(false);

    // click noise?
    // analogWriteFreq(100);
    // analogWrite(BUZZER,512);
    // delay(5);
    // analogWrite(BUZZER,0);
  }
}

// void loop(){
//   doLoop();
// }

int round_f(float x){
  return (int)round(x);
}

float round_f_2(float x){
  return round(x);
}

void doWake(){
}

void doAbort(){
}

void testTC(){
      ReadCurrentTemp();
      if ( nextTempRead < millis() )
      {
        maxT = minT = currentTemp;
        nextTempRead = millis() + tempSampleRate;
      }
      // tft.fillScreen(BLACK);
      // Serial.println("loop testTC");
      // Serial.println((String)currentTemp);
      if ( currentTemp > 0 )
      {
        Serial.println("deviation: " + (String)(maxT-minT));
        // color code temperature
        if(currentTemp > hotTemp) tft.setTextColor( RED, BLACK );
        else if(currentTemp < coolTemp) tft.setTextColor( GREEN, BLACK );
        else tft.setTextColor( YELLOW, BLACK );

        tft.setTextSize(2);
        // println_Center( tft, "  "+String( round_f( currentTemp ) )+"c  ", tft.width() / 2, ( tft.height() / 2 ) + 10 );
        println_Center( tft, "  "+String( ( currentTemp ) )+"c",tft.width() / 2 - 20, ( tft.height() / 2 ) );
        tft.setTextSize(1);
        println_Center( tft,"+/-" + (String)(maxT-minT), tft.width() / 2, ( tft.height() / 2 ) + 30 );
      }
      else{
        tft.setTextSize(textsize_1);
        tft.setTextColor( RED, BLACK );
        println_Center( tft, "error " + (String)millis(),tft.width() / 2,( tft.height() / 2 ) + 10 );
      }
    if(currentTemp>maxT) maxT = currentTemp;
    if(currentTemp<minT) minT = currentTemp;
    // Serial.println(minT);
    // Serial.println(maxT);
}

// any button press handler
// if sleeping wake up
// if reflow any button abort
bool anyButton(){
  return false;
  bool sleep = false;
  if(state == 1 || state == 2 || state == 15){
    doAbort();
    return true;
  }
  else {
    if(sleep) doWake();
    return true;
  }  
  return false;
}

// states GUI
// @TODO using byte definitions for states but also using second level for reflow 
// states warmup etc, neds to be refactored a bit
// 
// byte state; // 0 = ready, 1 = warmup, 2 = reflow, 3 = finished, 10 Menu, 11+ settings

typedef enum {
    READY    = 0 ,
    WARMUP   = 1 ,
    REFLOW   = 2 ,
    FINISHED = 3 ,
    MENU     = 0 ,
    SETTING  = 11, // above 11 is settings menu options
    PASTE    = 12,
    RESET    = 13,
    OVENCHECK = 15,

} rm_states_t;


// debugging levels
typedef enum {
    DEBUG_ERROR     = 0,
    DEBUG_NOTIFY    = 1, // default
    DEBUG_VERBOSE   = 2,
    DEBUG_DEV       = 3,
    DEBUG_MAX       = 4
} wm_debuglevel_t;


void debugPin(uint8_t pin){
    Serial.print("pinmode: ");
    Serial.println(getPinMode(pin),HEX);
    Serial.println("pinstate: " + (String)digitalRead(pin));  
}

int getPinMode(uint8_t pin)
{
  if (pin >= NUM_DIGITAL_PINS) return (-1);

  uint8_t bit = digitalPinToBitMask(pin);
  uint32_t port = digitalPinToPort(pin);
  volatile uint32_t *reg = portModeRegister(port);
  if (*reg & bit) return (OUTPUT); // 0x01

  volatile uint32_t *out = portOutputRegister(port);
  return ((*out & bit) ? INPUT_PULLUP : INPUT); // 0x00
}


// SERIAL CONSOLE
#define MAX_NUM_CHARS 16 // maximum number of characters read from the Serial Monitor
char cmd[MAX_NUM_CHARS];       // char[] to store incoming serial commands
boolean cmd_complete = false;  // whether the command string is complete


/*
 * Prints a usage menu.
 */
const char usageText[] PROGMEM = R"=====(
Usage:
m <n> : select mode <n>

b+    : increase brightness
b-    : decrease brightness
b <n> : set brightness to <n>

s+    : increase speed
s-    : decrease speed
s <n> : set speed to <n>

c 0x007BFF : set color to 0x007BFF

Have a nice day.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
)=====";

void printUsage() {
  Serial.println((const __FlashStringHelper *)usageText);
}

void process_command(){
  if (strncmp(cmd,"b ",2) == 0) {
    uint32_t arg = (uint32_t)atoi(cmd + 2);
    Serial.print(F("Set freq to: ") );
    Serial.println(arg);
    analogWriteFreq(arg); // confirm ?
  }

  if (strncmp(cmd,"A ",2) == 0) {
    uint8_t arg = (uint8_t)atoi(cmd + 2);
    Serial.print(F("Abort") );
    Serial.println(arg);
    AbortReflow();
  }

  if (strncmp(cmd,"R ",2) == 0) {
    uint8_t arg = (uint8_t)atoi(cmd + 2);
    Serial.print(F("REFLOW") );
    Serial.println(arg);
    StartReflow();
  }

  cmd[0] = '\0';         // reset the commandstring
  cmd_complete = false;  // reset command complete 
}


  void recvChar(void) {
  static byte index = 0;
  while (Serial.available() > 0 && cmd_complete == false) {
    char rc = Serial.read();
    if (rc != '\n') {
      if(index < MAX_NUM_CHARS) cmd[index++] = rc;
    } else {
      cmd[index] = '\0'; // terminate the string
      index = 0;
      cmd_complete = true;
      Serial.print("received '"); Serial.print(cmd); Serial.println("'");
    }
  }
}

void serialLoop(){
  recvChar(); // read serial comm
  if(cmd_complete) {
    process_command();
  }
}

// analogWrite is limited to 100hz-40000 because of clamps on analogWriteFreq
// int startWaveform(uint8_t pin, uint32_t timeHighUS, uint32_t timeLowUS, uint32_t runTimeUS) {

// extern void analogWriteAnything(uint8_t pin, int val) {
//   if (pin > 16) {
//     return;
//   }
//   uint32_t analogPeriod = 1000000L / 6;
//   if (val < 0) {
//     val = 0;
//   } else if (val > analogScale) {
//     val = analogScale;
//   }

//   analogMap &= ~(1 << pin);
//   uint32_t high = (analogPeriod * val) / analogScale;
//   uint32_t low = analogPeriod - high;
//   pinMode(pin, OUTPUT);
//   if (low == 0) {
//     stopWaveform(pin);
//     digitalWrite(pin, HIGH);
//   } else if (high == 0) {
//     stopWaveform(pin);
//     digitalWrite(pin, LOW);
//   } else {
//     if (startWaveform(pin, high, low, 0)) {
//       analogMap |= (1 << pin);
//     }
//   }
// }
// 
void scannetworks(){
  Serial.print("Scan start ... ");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println(" network(s) found");
  for (int i = 0; i < n; i++)
  {
    Serial.println(WiFi.SSID(i));
  }
}