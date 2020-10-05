#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <XPT2046_Touchscreen.h>

// Define STASSID, STAPSK and PROXYHOST in a config.h file
#include "config.h"

//for D1 mini or TFT I2C Connector Shield (V1.1.0 or later)
#define TFT_CS D0  
#define TFT_DC D8  
#define TFT_RST -1 
#define TS_CS D3 

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TS_CS);

typedef struct
{
  unsigned int x;
  unsigned int y;
} point;

const char *ssid = STASSID;
const char *password = STAPSK;
const char *proxyhost = PROXYHOST;

// variable to store the last refresh moment
unsigned long previousMillis = 0;

// refresh the states every 30 seconds
const long interval = 30000;

// number of rectangles on a row
const short rectanglesPerRow = 5;

// screen in pixels
const int width = 240;
const int height = 320;

// size of a rectagle
const int size = width / rectanglesPerRow;

const short rectanglesPerColumn = height / size;

// the selected rectangle
int selectedRectangle = -1;

// background color of the display
const uint16_t backgroundColor = ILI9341_BLACK;

// the sceen does not seem to be perfectly calibrated, so we guestimate its bounds
const unsigned int minX = 350;
const unsigned int maxX = 3775;
const unsigned int minY = 330;
const unsigned int maxY = 3260;

void setup()
{
  Serial.begin(115200);
  ts.begin();
  ts.setRotation(0);

  tft.begin();

  tft.fillScreen(ILI9341_NAVY);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(4);
  tft.println(WiFi.localIP());

  delay(2500);
  tft.fillScreen(backgroundColor);

  refreshStates();
}

void loop()
{
  unsigned long currentMillis = millis();

  // check the interval and also if the millis did overflow
  if (currentMillis - previousMillis >= interval || currentMillis < previousMillis)
  {
    previousMillis = currentMillis;
    refreshStates();
  }

  if (ts.touched())
  {
    handleTouch();
  }
}

void handleTouch()
{
  TS_Point tp = ts.getPoint();
  Serial.print(tp.x);
  Serial.print(" x ");
  Serial.println(tp.y);

  int rect = getSelectedRectangle(ts.getPoint());
  if (rect != selectedRectangle)
  {
    // clear
    point p = getPoint(selectedRectangle);
    tft.drawRect(p.x, p.y, size, size, backgroundColor);
    selectedRectangle = rect;
    p = getPoint(selectedRectangle);
    tft.drawRect(p.x, p.y, size, size, ILI9341_WHITE);

    displaySelection();
  }
}

int getSelectedRectangle(TS_Point tp)
{
  // Maybe check pressure to ignore accidental touches?
  int tx = constrain(tp.x, minX, maxX);
  int ty = constrain(4095 - tp.y, minY, maxY);

  int nx = map(tx, minX, maxX, 0, rectanglesPerRow - 1);
  int ny = map(ty, minY, maxY, 0, rectanglesPerColumn - 1);

  return (ny * rectanglesPerRow) + nx;
}

point getPoint(int i)
{
  point p;
  p.x = (i * size) % width;
  p.y = (i / (width / size)) * size;
  return p;
}

void refreshStates()
{
  // put your main code here, to run repeatedly:
  if (WiFi.status() == WL_CONNECTED)
  {
    StaticJsonDocument<500> root;

    HTTPClient http; //Object of class HTTPClient
    http.begin(proxyhost);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      DeserializationError error = deserializeJson(root, http.getString());

      // Test if parsing succeeds.
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        tft.fillScreen(ILI9341_NAVY);
        tft.setCursor(0, 20);
        tft.setTextSize(4);
        tft.setTextColor(ILI9341_RED);
        tft.println("error");
        return;
      }

      uint16_t color = ILI9341_GREEN;

      JsonArray offByOne = root["offByOne"];
      for (int i = 0; i < offByOne.size(); i++)
      {
        String state = offByOne[i];

        if (state.compareTo("FAILURE") == 0)
        {
          color = ILI9341_RED;
        }
        else if (state.compareTo("BUILDING") == 0)
        {
          color = ILI9341_BLUE;
        }
        else
        {
          color = ILI9341_GREEN;
        }

        point p = getPoint(i);

        tft.fillRect(p.x + 1, p.y + 1, size - 2, size - 2, color);
      }

      displaySelection();
    }
    http.end(); //Close connection
  }
}

void displaySelection()
{
  tft.fillRect(0, 300, width, 20, backgroundColor);

  if (selectedRectangle != -1)
  { 
    tft.setCursor(0, 300);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("Selected: ");
    tft.print(selectedRectangle);
  }
}
