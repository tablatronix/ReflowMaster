# 1 "/var/folders/0_/8qr6jgcx3qsgjcrpsgz2c9w00000gn/T/tmpv3eq84"
#include <Arduino.h>
# 1 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
# 31 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
#include <ESP8266WiFi.h>

#include <SPI.h>
#include <spline.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "MAX31855.h"
#include "OneButton.h"
#include "ReflowMasterProfile.h"





#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))


#define DEBUG 







#define TFT_DC 15
#define TFT_CS -1
#define TFT_RESET D1






#define MAXDO D3
#define MAXCS D4
#define MAXCLK D0

uint16_t rotation = 3;

uint16_t textsize_1 = 1;
uint16_t textsize_2 = 2;
uint16_t textsize_3 = 3;
uint16_t textsize_4 = 4;
uint16_t textsize_5 = 5;







#define BUTTON0 0
#define BUTTON1 A0
#define BUTTON2 A0
#define BUTTON3 A0

#define BUZZER 5
#define FAN 5
#define RELAY 4




#define BLUE 0x001F
#define TEAL 0x0438
#define GREEN 0x07E0
#define CYAN 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define ORANGE 0xFC00
#define PINK 0xF81F
#define PURPLE 0x8010
#define GREY 0xC618
#define WHITE 0xFFFF
#define BLACK 0x0000
#define DKBLUE 0x000D
#define DKTEAL 0x020C
#define DKGREEN 0x03E0
#define DKCYAN 0x03EF
#define DKRED 0x6000
#define DKMAGENTA 0x8008
#define DKYELLOW 0x8400
#define DKORANGE 0x8200
#define DKPINK 0x9009
#define DKPURPLE 0x4010
#define DKGREY 0x4A49


#define grid_color DKBLUE
#define axis_color BLUE
#define axis_text WHITE

#define ButtonCol1 GREEN
#define ButtonCol2 RED
#define ButtonCol3 BLUE
#define ButtonCol4 YELLOW


typedef struct {
  boolean valid = false;
  boolean useFan = true;
  byte paste = 0;
  float power = 1;
  int lookAhead = 12;
  int lookAheadWarm = 1;
  int tempOffset = 0;
  bool startFullBlast = false;
} Settings;

const String ver = "1.03";
bool newSettings = false;

unsigned long nextTempRead;
unsigned long nextTempAvgRead;
int avgReadCount = 0;
int avgSamples = 10;
int tempSampleRate = 1000;

int hotTemp = 80;
int coolTemp = 50;
int shutDownTemp = 50;

double timeX = 0;

byte state;
byte state_settings = 0;
byte settings_pointer = 0;



ReflowGraph solderPaste[4];

int currentGraphIndex = 0;


int calibrationState = 0;
long calibrationSeconds = 0;
long calibrationStatsUp = 0;
long calibrationStatsDown = 300;
bool calibrationUpMatch = false;
bool calibrationDownMatch = false;
float calibrationDropVal = 0;
float calibrationRiseVal = 0;


float currentDuty = 0;
float currentTemp = 0;
float currentTempAvg = 0;
float lastTemp = -1;
float currentDetla = 0;
unsigned int currentPlotColor = GREEN;
bool isCuttoff = false;
bool isFanOn = false;
float lastWantedTemp = -1;
int buzzerCount = 5;



int graphRangeMin_X = 0;
int graphRangeMin_Y = 30;
int graphRangeMax_X = -1;
int graphRangeMax_Y = 165;
int graphRangeStep_X = 30;
int graphRangeStep_Y = 15;


Spline baseCurve;


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC,TFT_RESET);




MAX31855 tc(MAXCLK, MAXCS, MAXDO);


OneButton button0(BUTTON0, false);





Settings set;
void LoadPaste();
int GetGraphValue( int x );
int GetGraphTime( int x );
ReflowGraph& CurrentGraph();
void SetCurrentGraph( int index );
void checkButtonAnalog();
void safetyCheck();
void doLoop();
void SetRelayFrequency( int duty );
void MatchCalibrationTemp();
void ReadCurrentTempAvg();
void ReadCurrentTemp();
void MatchTemp();
void StartFan ( bool start );
void DrawHeading( String lbl, unsigned int acolor, unsigned int bcolor );
void Buzzer( int hertz, int len );
void DrawBaseGraph();
void BootScreen();
void ShowMenu();
void ShowSettings();
void ShowPaste();
void ShowMenuOptions( bool clearAll );
void UpdateSettingsPointer();
void StartWarmup();
void StartReflow();
void AbortReflow();
void EndReflow();
void SetDefaults();
void ResetSettingsToDefault();
void StartOvenCheck();
void ShowOvenCheck();
void ShowResetDefaults();
void UpdateSettingsFan( int posY );
void UpdateSettingsStartFullBlast( int posY );
void UpdateSettingsPower( int posY );
void UpdateSettingsLookAhead( int posY );
void UpdateSettingsTempOffset( int posY );
void button0Press();
void button1Press();
void button2Press();
void button3Press();
void SetupGraph(Adafruit_ILI9341 &d, double x, double y, double gx, double gy, double w, double h, double xlo, double xhi, double xinc, double ylo, double yhi, double yinc, String title, String xlabel, String ylabel, unsigned int gcolor, unsigned int acolor, unsigned int tcolor, unsigned int bcolor );
void Graph(Adafruit_ILI9341 &d, double x, double y, double gx, double gy, double w, double h );
void GraphDefault(Adafruit_ILI9341 &d, double x, double y, double gx, double gy, double w, double h, unsigned int pcolor );
char* string2char(String command);
void println_Center( Adafruit_ILI9341 &d, String heading, int centerX, int centerY );
void setFault(Adafruit_ILI9341 &d);
void println_Right( Adafruit_ILI9341 &d, String heading, int centerX, int centerY );
void setup();
void loop();
int round_f(float x);
#line 221 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
void LoadPaste()
{
#ifdef DEBUG
  Serial.println("Load Paste");
#endif
# 239 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
  float baseGraphX[7] = { 1, 90, 180, 210, 240, 270, 300 };
  float baseGraphY[7] = { 27, 90, 130, 138, 165, 138, 27 };

  solderPaste[0] = ReflowGraph( "CHIPQUIK", "No-Clean Sn42/Bi57.6/Ag0.4", 138, baseGraphX, baseGraphY, ELEMENTS(baseGraphX), 240, 270 );

  float baseGraphX2[7] = { 1, 90, 180, 225, 240, 270, 300 };
  float baseGraphY2[7] = { 25, 150, 175, 190, 210, 125, 50 };

  solderPaste[1] = ReflowGraph( "CHEMTOOLS L", "No Clean 63CR218 Sn63/Pb37", 183, baseGraphX2, baseGraphY2, ELEMENTS(baseGraphX2), 240, 270 );

  float baseGraphX3[6] = { 1, 75, 130, 180, 210, 250 };
  float baseGraphY3[6] = { 25, 150, 175, 210, 150, 50 };

  solderPaste[2] = ReflowGraph( "CHEMTOOLS S", "No Clean 63CR218 Sn63/Pb37", 183, baseGraphX3, baseGraphY3, ELEMENTS(baseGraphX3), 180, 210 );

  float baseGraphX4[7] = { 1, 60, 120, 160, 210, 260, 310 };
  float baseGraphY4[7] = { 25, 105, 150, 150, 220, 150, 20 };

  solderPaste[3] = ReflowGraph( "DOC SOLDER", "No Clean Sn63/Pb36/Ag2", 187, baseGraphX4, baseGraphY4, ELEMENTS(baseGraphX4), 210, 260 );


}


int GetGraphValue( int x )
{
  return ( CurrentGraph().reflowGraphY[x] );
}

int GetGraphTime( int x )
{
  return ( CurrentGraph().reflowGraphX[x] );
}


ReflowGraph& CurrentGraph()
{
  return solderPaste[ currentGraphIndex ];
}


void SetCurrentGraph( int index )
{
  #ifdef DEBUG
  Serial.println("SetCurrentGraph");
  #endif
  currentGraphIndex = index;
  graphRangeMax_X = solderPaste[ currentGraphIndex ].MaxTime();
  graphRangeMax_Y = solderPaste[ currentGraphIndex ].MaxValue() + 5;
  graphRangeMin_Y = solderPaste[ currentGraphIndex ].MinValue();

#ifdef DEBUG
  Serial.print("Setting Paste: ");
  Serial.println( currentGraphIndex );
  Serial.println( CurrentGraph().n );
  Serial.println( CurrentGraph().t );
#endif


  timeX = 0;
# 307 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
  baseCurve.setPoints(solderPaste[ currentGraphIndex ].reflowGraphX, solderPaste[ currentGraphIndex ].reflowGraphY, solderPaste[ currentGraphIndex ].reflowTangents, solderPaste[ currentGraphIndex ].len);

  baseCurve.setDegree( Hermite );
# 318 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
  Serial.println("re interpolate splines");

  for( int ii = 0; ii <= graphRangeMax_X; ii+= 1 )
  {

    solderPaste[ currentGraphIndex ].wantedCurve[ii] = baseCurve.value(ii);
    delay(0);
  }


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
      lastWanted = wantedTemp;
      delay(0);
  }
}

void checkButtonAnalog(){
  int level = analogRead(A0);
  int th = 10;
  int base = 30;
  int debounce = 300;
  int button0 = 65;
  int button1 = 195;
  int button2 = 625;
  int button3 = 820;

  button0 = 57;
  button1 = 170;
  button2 = 550;
  button3 = 733;

  if(level > base){
    Serial.println("BUTTON PRESS level:" + (String)level);
    delay(debounce);
    if(level < base) return;
    delay(100);
  }
       if(level > button0-th && level < button0+th) button0Press();
  else if(level > button1-th && level < button1+th) button1Press();
  else if(level > button2-th && level < button2+th) button2Press();
  else if(level > button3-th && level < button3+th) button3Press();
  else return;
}

void safetyCheck(){

  tc.read();
  if(tc.getStatus() != STATUS_OK){
    tft.setTextColor(WHITE);
    tft.fillScreen(RED);
    println_Center( tft, "Thermocouple ERROR", tft.width() / 2, ( tft.height() / 2 ) + 10 );
    delay(5000);
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
    delay(5000);

    state = 0;
  }
}

void doLoop()
{

  safetyCheck();

  checkButtonAnalog();

  button0.tick();
# 416 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
  if ( state == 0 )
  {

    LoadPaste();

    SetCurrentGraph( set.paste );

    ShowMenu();

    Serial.println("back in loop");
  }
  else if ( state == 1 )
  {

    if ( nextTempRead < millis() )
    {
      nextTempRead = millis() + tempSampleRate;

      ReadCurrentTemp();
      MatchTemp();

      if ( currentTemp >= GetGraphValue(0) )
      {

        lastTemp = currentTemp;
        avgReadCount = 0;
        currentTempAvg = 0;

        StartReflow();
      }
      else if ( currentTemp > 0 )
      {

        tft.setTextColor( YELLOW, BLACK );
        tft.setTextSize(textsize_5);
        println_Center( tft, "  "+String( round_f( currentTemp ) )+"c  ", tft.width() / 2, ( tft.height() / 2 ) + 10 );
      }
    }
    return;
  }
  else if ( state == 3 )
  {

    return;
  }
  else if ( state == 11 || state == 12 || state == 13 )
  {

    return;
  }
  else if ( state == 10 )
  {

    if ( nextTempRead < millis() )
    {
      nextTempRead = millis() + tempSampleRate;


      ReadCurrentTemp();

      if ( currentTemp > 0 )
      {
        if(currentTemp > hotTemp) tft.setTextColor( RED, BLACK );
        else if(currentTemp < coolTemp) tft.setTextColor( GREEN, BLACK );
        else tft.setTextColor( YELLOW, BLACK );

        tft.setTextSize(textsize_5);
        int third = tft.width()/4;
        println_Center( tft, "  "+String( round_f( currentTemp ) )+"c  ", tft.width() / 2, ( tft.height() / 2 ) + 10 );
      }
    }
    delay(100);
    return;
  }
  else if ( state == 15 )
  {

    return;
  }
  else if ( state == 16 )
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



        if (currentTemp >= GetGraphValue(0) )
        {

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
  else
  {
    if ( nextTempAvgRead < millis() )
    {
      nextTempAvgRead = millis() + (tempSampleRate/avgSamples);
      ReadCurrentTempAvg();
    }
    if ( nextTempRead < millis() )
    {
      nextTempRead = millis() + tempSampleRate;


      currentTemp = ( currentTempAvg / avgReadCount );

      avgReadCount = 0;
      currentTempAvg = 0;


      MatchTemp();

      if ( currentTemp > 0 )
      {
        timeX++;

        if ( timeX > CurrentGraph().completeTime )
        {
          EndReflow();
        }
        else
        {
          Graph(tft, timeX, currentTemp, 30, 220, 270, 180 );

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


void SetRelayFrequency( int duty )
{
  #ifdef DEBUG
  Serial.println("SetRelayFrequency");
  #endif

  currentDuty = ((float)duty * set.power );
  currentDuty = constrain( round_f( currentDuty ), 0, 255);



  analogWrite( RELAY, currentDuty );

#ifdef DEBUG
  Serial.print("RELAY Duty Cycle: ");
  Serial.println( "write: " + (String)currentDuty + " " + String( ( currentDuty / 256.0 ) * 100) + "%" +" Using Power: " + String( round_f( set.power * 100 )) + "%" );
#endif
}






void MatchCalibrationTemp()
{
  if ( calibrationState == 0 )
  {

    SetRelayFrequency( 255 );


    if ( currentTemp >= GetGraphValue(0) )
        calibrationSeconds ++;

    if ( currentTemp >= GetGraphValue(1) )
    {
#ifdef DEBUG
      Serial.println("Cal Heat Up Speed " + String( calibrationSeconds ) );
#endif

      calibrationRiseVal = ( (float)currentTemp / (float)( GetGraphValue(1) ) );
      calibrationUpMatch = ( calibrationSeconds <= GetGraphTime(1) );


      calibrationState = 1;
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


      calibrationDropVal = ( (float)( GetGraphValue(1) - currentTemp ) / (float)GetGraphValue(1) );

      calibrationDownMatch = ( calibrationDropVal > 0.33 );

      calibrationState = 2;
      StartFan( true );
    }
  }
}







void ReadCurrentTempAvg()
{
  int status = tc.read();
  float internal = tc.getInternal();
  currentTempAvg += tc.getTemperature() + set.tempOffset;
  avgReadCount++;

  #ifdef DEBUG
  Serial.print(" avgtot: ");
  Serial.print( currentTempAvg );
  Serial.print(" avg count: ");
  Serial.println( avgReadCount );
  Serial.print(" avg: ");
  Serial.println( currentTempAvg/avgReadCount );
  #endif
}



void ReadCurrentTemp()
{
  int status = tc.read();
  #ifdef DEBUG


  #endif
  float internal = tc.getInternal();
  currentTemp = tc.getTemperature() + set.tempOffset;
}


void MatchTemp()
{
  float duty = 0;
  float wantedTemp = 0;
  float wantedDiff = 0;
  float tempDiff = 0;
  float perc = 0;


  if ( timeX < CurrentGraph().offTime )
  {

    if ( timeX < CurrentGraph().reflowGraphX[2] )
      wantedTemp = CurrentGraph().wantedCurve[ (int)timeX + set.lookAheadWarm ];
    else
      wantedTemp = CurrentGraph().wantedCurve[ (int)timeX + set.lookAhead ];

    wantedDiff = (wantedTemp - lastWantedTemp );
    lastWantedTemp = wantedTemp;

    tempDiff = ( currentTemp - lastTemp );
    lastTemp = currentTemp;

    perc = wantedDiff - tempDiff;

#ifdef DEBUG
    Serial.print( "T: " );
    Serial.print( timeX );

    Serial.print( "  Current: " );
    Serial.print( currentTemp );

    Serial.print( "  Wanted: " );
    Serial.print( wantedTemp );

    Serial.print( "  T Diff: " );
    Serial.print( tempDiff );

    Serial.print( "  W Diff: " );
    Serial.print( wantedDiff );

    Serial.print( "  Perc: " );
    Serial.print( perc );
#endif

    isCuttoff = false;


    if ( !isFanOn && timeX >= CurrentGraph().fanTime )
    {
#ifdef DEBUG
      Serial.print( "COOLDOWN: " );
#endif

      if ( set.useFan )
      {
        isFanOn = true;
        DrawHeading( "COOLDOWN!", GREEN, BLACK );
        Buzzer( 1000, 2000 );

        StartFan ( true );
      }
      else
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
      base = 128 + ( currentDetla * 5 );
  }
  else if ( currentDetla < 0 )
  {
      base = 32 + ( currentDetla * 15 );
  }

  base = constrain( base, 0, 256 );

#ifdef DEBUG
  Serial.print("  Base: ");
  Serial.print( base );
  Serial.print( " -> " );
#endif

  duty = base + ( 172 * perc );
  duty = constrain( duty, 0, 256 );


  Serial.println("startFullBlast");
  Serial.println(timeX);
  Serial.println(CurrentGraph().reflowGraphX[1]);

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

  }
}

void DrawHeading( String lbl, unsigned int acolor, unsigned int bcolor )
{
    tft.setTextSize(textsize_4);
    tft.setTextColor(acolor , bcolor);
    tft.setCursor(0,0);
    tft.fillRect( 0, 0, 220, 40, BLACK );
    tft.println( String(lbl) );
}


void Buzzer( int hertz, int len )
{
   tone( BUZZER, hertz, len);
}


double ox , oy ;

void DrawBaseGraph()
{
  oy = 220;
  ox = 30;
  timeX = 0;

  for( int ii = 0; ii <= graphRangeMax_X; ii+= 5 )
  {
    GraphDefault(tft, ii, CurrentGraph().wantedCurve[ii], 30, 220, 270, 180, PINK );
    delay(0);
  }

  ox = 30;
  oy = 220;
  timeX = 0;
}

void BootScreen()
{
  tft.setRotation(rotation);
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


  #ifdef DEBUG
    Serial.println("TFT Show Menu");
  #endif
  tft.fillScreen(ILI9341_BLACK);
  Serial.println("clear display");

  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(textsize_2);
  tft.setCursor( 20, 20 );
  tft.println( "CURRENT PASTE" );
  Serial.println("disp curr paste");


  tft.setTextColor( YELLOW, BLACK );
  tft.setCursor( 20, 40 );
  tft.println( CurrentGraph().n );
  tft.setCursor( 20, 60 );
  tft.println( String(CurrentGraph().tempDeg) +"deg");
  Serial.println("disp temperature");


  if ( newSettings )
  {
    tft.setTextColor( CYAN, BLACK );
    println_Center( tft, "Settings Stomped!!", tft.width() / 2, tft.height() - 80 );
  }

  tft.setTextSize(textsize_1);
  tft.setTextColor( WHITE, BLACK );





    println_Center( tft, "Reflow Master - PCB v2018-2, Code v" + ver, tft.width() / 2, tft.height() - 20 );





  ShowMenuOptions( true );
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


  UpdateSettingsFan( posY );

  posY += incY;


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

void ShowMenuOptions( bool clearAll )
{
    #ifdef DEBUG
    Serial.println("ShowMenuOptions");
    #endif
  int buttonPosY[] = { 19, 74, 129, 184 };
  int buttonHeight = 16;
  int buttonWidth = 4;

  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(textsize_2);

  if ( clearAll )
  {

    for ( int i = 0; i < 4; i++ )
      tft.fillRect( tft.width()-100, buttonPosY[i]-2, 100, buttonHeight+4, BLACK );
      delay(0);
  }
  if ( state == 10 )
  {

    tft.fillRect( tft.width()-5, buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "START", tft.width()- 27, buttonPosY[0] + 8 );


    tft.fillRect( tft.width()-5, buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "SETTINGS", tft.width()- 27, buttonPosY[1] + 8 );


    tft.fillRect( tft.width()-5, buttonPosY[3], buttonWidth, buttonHeight, YELLOW );
    println_Right( tft, "OVEN CHECK", tft.width()- 27, buttonPosY[3] + 8 );
    return;
  }
  else if ( state == 11 )
  {

    tft.fillRect( tft.width()-100, buttonPosY[0]-2, 100, buttonHeight+4, BLACK );
    tft.fillRect( tft.width()-5, buttonPosY[0], buttonWidth, buttonHeight, GREEN );
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


    tft.fillRect( tft.width()-5, buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "BACK", tft.width()- 27, buttonPosY[1] + 8 );


    tft.fillRect( tft.width()-5, buttonPosY[2], buttonWidth, buttonHeight, BLUE );
    println_Right( tft, "/\\", tft.width()- 27, buttonPosY[2] + 8 );


    tft.fillRect( tft.width()-5, buttonPosY[3], buttonWidth, buttonHeight, YELLOW );
    println_Right( tft, "\\/", tft.width()- 27, buttonPosY[3] + 8 );

    UpdateSettingsPointer();
  }
  else if ( state == 12 )
  {

    tft.fillRect( tft.width()-5, buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "SELECT", tft.width()- 27, buttonPosY[0] + 8 );


    tft.fillRect( tft.width()-5, buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "BACK", tft.width()- 27, buttonPosY[1] + 8 );


    tft.fillRect( tft.width()-5, buttonPosY[2], buttonWidth, buttonHeight, BLUE );
    println_Right( tft, "/\\", tft.width()- 27, buttonPosY[2] + 8 );


    tft.fillRect( tft.width()-5, buttonPosY[3], buttonWidth, buttonHeight, YELLOW );
    println_Right( tft, "\\/", tft.width()- 27, buttonPosY[3] + 8 );

    UpdateSettingsPointer();
  }
  else if ( state == 13 )
  {

    tft.fillRect( tft.width()-5, buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "YES", tft.width()- 27, buttonPosY[0] + 8 );


    tft.fillRect( tft.width()-5, buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "NO", tft.width()- 27, buttonPosY[1] + 8 );
  }
  else if ( state == 1 || state == 2 || state == 16 )
  {

    tft.fillRect( tft.width()-5, buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "ABORT", tft.width()- 27, buttonPosY[0] + 8 );
  }
  else if ( state == 3 )
  {
     tft.fillRect( tft.width()-100, buttonPosY[0]-2, 100, buttonHeight+4, BLACK );


    tft.fillRect( tft.width()-5, buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "MENU", tft.width()- 27, buttonPosY[0] + 8 );
  }
  else if ( state == 15 )
  {

    tft.fillRect( tft.width()-5, buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "START", tft.width()- 27, buttonPosY[0] + 8 );


    tft.fillRect( tft.width()-5, buttonPosY[1], buttonWidth, buttonHeight, RED );
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
  SetupGraph(tft, 0, 0, 30, 220, 270, 180, graphRangeMin_X, graphRangeMax_X, graphRangeStep_X, graphRangeMin_Y, graphRangeMax_Y, graphRangeStep_Y, "Reflow Temp", " Time (s)", "deg (C)", grid_color, axis_color, axis_text, BLACK );

  DrawHeading( "READY", WHITE, BLACK );
  DrawBaseGraph();
}

void AbortReflow()
{
  if ( state == 1 || state == 2 || state == 16 )
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

  set.valid = true;
  set.power = 1;
  set.paste = 0;
  set.useFan = false;
  set.lookAhead = 5;
  set.lookAheadWarm = 5;
  set.startFullBlast = true;
  set.tempOffset = 0;
}

void ResetSettingsToDefault()
{

  SetDefaults();



  SetCurrentGraph( set.paste );


  settings_pointer = 0;
  ShowSettings();
}






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
# 1480 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
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
  tft.fillRect( 15, posY-5, 200, 20, BLACK );
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
  tft.fillRect( 15, posY-5, 240, 20, BLACK );
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
  tft.fillRect( 15, posY-5, 240, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );

  tft.setCursor( 20, posY );
  tft.print( "POWER ");
  tft.setTextColor( YELLOW, BLACK );
  tft.println( String( round_f((set.power * 100))) +"%");
  tft.setTextColor( WHITE, BLACK );
}

void UpdateSettingsLookAhead( int posY )
{
  tft.fillRect( 15, posY-5, 260, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );

  tft.setCursor( 20, posY );
  tft.print( "GRAPH LOOK AHEAD ");
  tft.setTextColor( YELLOW, BLACK );
  tft.println( String( set.lookAhead) );
  tft.setTextColor( WHITE, BLACK );
}
# 1579 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
void UpdateSettingsTempOffset( int posY )
{
  tft.fillRect( 15, posY-5, 220, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );

  tft.setCursor( 20, posY );
  tft.print( "TEMP OFFSET ");
  tft.setTextColor( YELLOW, BLACK );
  tft.println( String( set.tempOffset) );
  tft.setTextColor( WHITE, BLACK );
}







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
      StartWarmup();
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
      if ( settings_pointer == 0 )
      {
        settings_pointer = set.paste;
        ShowPaste();
      }
      else if ( settings_pointer == 1 )
      {
        set.useFan = !set.useFan;

        UpdateSettingsFan( 70 );
      }
      else if ( settings_pointer == 2 )
      {
        set.power += 0.1;
        if ( set.power > 1.55 )
          set.power = 0.5;

        UpdateSettingsPower( 90 );
      }
      else if ( settings_pointer == 3 )
      {
        set.lookAhead += 1;
        if ( set.lookAhead > 15 )
          set.lookAhead = 1;

        UpdateSettingsLookAhead( 110 );
      }
      else if ( settings_pointer == 4 )
      {
        set.tempOffset += 1;
        if ( set.tempOffset > 15 )
          set.tempOffset = -15;

        UpdateSettingsTempOffset( 130 );
      }
      else if ( settings_pointer == 5 )
      {
        set.startFullBlast = !set.startFullBlast;

        UpdateSettingsStartFullBlast( 150 );
      }
      else if ( settings_pointer == 6 )
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
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    Buzzer( 2000, 50 );

    if ( state == 10 )
    {
      settings_pointer = 0;
      ShowSettings();
    }
    else if ( state == 11 )
    {


      ShowMenu();
    }
    else if ( state == 12 || state == 13 )
    {
      settings_pointer = 0;
      ShowSettings();
    }
    else if ( state == 15 )
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

    if ( state == 11 )
    {
      settings_pointer = constrain( settings_pointer -1, 0, 6 );
      ShowMenuOptions( false );

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

    }
    else if ( state == 12 )
    {
      settings_pointer = constrain( settings_pointer +1, 0, ELEMENTS(solderPaste)-1 );
      UpdateSettingsPointer();
    }
  }
}







void SetupGraph(Adafruit_ILI9341 &d, double x, double y, double gx, double gy, double w, double h, double xlo, double xhi, double xinc, double ylo, double yhi, double yinc, String title, String xlabel, String ylabel, unsigned int gcolor, unsigned int acolor, unsigned int tcolor, unsigned int bcolor )
{
  double ydiv, xdiv;
  double i;
  int temp;
  int rot, newrot;

    ox = (x - xlo) * ( w) / (xhi - xlo) + gx;
    oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

    for ( i = ylo; i <= yhi; i += yinc)
    {

      temp = (i - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

      if (i == ylo) {
        d.drawLine(gx, temp, gx + w, temp, acolor);
      }
      else {
        d.drawLine(gx, temp, gx + w, temp, gcolor);
      }

      d.setTextSize(textsize_1);
      d.setTextColor(tcolor, bcolor);
      d.setCursor(gx-25, temp);
      println_Right( d, String(round_f(i)), gx-25, temp );
      delay(0);
    }


    for (i = xlo; i <= xhi; i += xinc)
    {
      temp = (i - xlo) * ( w) / (xhi - xlo) + gx;
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
      d.setCursor(temp, gy + 10);

      if ( i <= xhi - xinc )
        println_Center(d, String(round_f(i)), temp, gy + 10 );
      else
        println_Center(d, String(round_f(xhi)), temp, gy + 10 );

      delay(0);
    }


    d.setTextSize(textsize_2);
    d.setTextColor(tcolor, bcolor);
    d.setCursor(gx , gy - h - 30);
    d.println(title);

    d.setTextSize(textsize_1);
    d.setTextColor(acolor, bcolor);
    d.setCursor(w - 25 , gy - 10);
    d.println(xlabel);

    tft.setRotation(rotation-1);
    d.setTextSize(textsize_1);
    d.setTextColor(acolor, bcolor);
    d.setCursor(w - 116, 34 );
    d.println(ylabel);
    tft.setRotation(rotation);
    delay(0);
}

void Graph(Adafruit_ILI9341 &d, double x, double y, double gx, double gy, double w, double h )
{

  x = (x - graphRangeMin_X) * ( w) / (graphRangeMax_X - graphRangeMin_X) + gx;
  y = (y - graphRangeMin_Y) * (gy - h - gy) / (graphRangeMax_Y - graphRangeMin_Y) + gy;

  if ( timeX < 2 )
    oy = min( oy, y );

  y = min( (int)y, 220 );



  d.drawLine(ox, oy + 1, x, y + 1, currentPlotColor);
  d.drawLine(ox, oy - 1, x, y - 1, currentPlotColor);
  d.drawLine(ox, oy, x, y, currentPlotColor );
  ox = x;
  oy = y;
}

void GraphDefault(Adafruit_ILI9341 &d, double x, double y, double gx, double gy, double w, double h, unsigned int pcolor )
{

  x = (x - graphRangeMin_X) * ( w) / (graphRangeMax_X - graphRangeMin_X) + gx;
  y = (y - graphRangeMin_Y) * (gy - h - gy) / (graphRangeMax_Y - graphRangeMin_Y) + gy;


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

void println_Center( Adafruit_ILI9341 &d, String heading, int centerX, int centerY )
{
    int x = 0;
    int y = 0;
    int16_t x1, y1;
    uint16_t ww, hh;

    d.getTextBounds( string2char(heading), x, y, &x1, &y1, &ww, &hh );
# 1911 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
    d.setCursor( centerX - ww/2 + 2, centerY - hh / 2);
    d.println( heading );
# 1923 "/Users/shawn/projects/microcontrollers/dev/libraries/ReflowMaster/Code/Reflow_Master/Reflow_Master.ino"
}

void setFault(Adafruit_ILI9341 &d){
  d.setCursor(45,216);
}

void println_Right( Adafruit_ILI9341 &d, String heading, int centerX, int centerY )
{
    int x = 0;
    int y = 0;
    int16_t x1, y1;
    uint16_t ww, hh;

    d.getTextBounds( string2char(heading), x, y, &x1, &y1, &ww, &hh );
    d.setCursor( centerX + ( 18 - ww ), centerY - hh / 2);
    d.println( heading );
}



void setup()
{


  pinMode( RELAY, OUTPUT );







  analogWriteRange(255);

  SetRelayFrequency( 0 );


  Serial.begin(115200);
  Serial.setDebugOutput(true);


  WiFi.mode(WIFI_OFF);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  Serial.println(ESP.getFreeHeap());
  Serial.println(ESP.getHeapFragmentation());
  Serial.println(ESP.getMaxFreeBlockSize());


  delay(500);





  if ( !set.valid )
  {
    SetDefaults();
    newSettings = true;

  }


  button0.attachClick(button0Press);




#ifdef DEBUG
  Serial.println("TFT Begin...");
#endif


  tft.begin();

  BootScreen();

#ifdef DEBUG
  Serial.println("Booted Spash Screen");
#endif



tc.begin();
tc.read();
#ifdef DEBUG
  Serial.println("Thermocouple Begin...");
  Serial.println((String)round(tc.getInternal()));
  Serial.println((String)round(tc.getTemperature()));
#endif


  delay(500);
  state = 0;
}


void loop(){
  doLoop();
}

int round_f(float x){
  return (int)round(x);
}