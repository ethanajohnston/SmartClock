#include "Arduino.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "time.h"
//sudo chmod a+rw /dev/ttyUSB0

// select which pin will trigger the configuration portal when set to LOW
#define TRIGGER_PIN 0

//LED INFO
#define PIN        27
#define NUMPIXELS 60

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;
unsigned int  startTime = millis();
IPAddress apIP4 = IPAddress(10,0,1,1);


int gpio16Value; 
int gpio17Value;

#define gpio16LEDPin 2 /* One LED connected to GPIO16 - RX2 */
#define gpio17LEDPin 4 /* One LED connected to GPIO17 - TX2 */


int timeout = 120; // seconds to run for
WiFiManager wm;
std::vector<const char *> menu = {"wifi","update"};

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void
updateClockLEDS()
{
    if(!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }

    //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    pixels.clear();
    int sec = timeinfo.tm_sec;
    for(int i = 0; i <= sec; i++)
    {
        pixels.setPixelColor(i, pixels.Color(0, 0, 1));
    }
    pixels.show();
}

void
checkConfigResetButton()
{
    // is configuration portal requested?
    if (digitalRead(TRIGGER_PIN) == LOW) 
    {
        delay(500);
        if (digitalRead(TRIGGER_PIN) == LOW) 
        {
            Serial.println("BUTTON PRESSED. RESETTING. ERASING WIFI SETTINGS.");
            wm.resetSettings();
            ESP.restart();
        }
    }
}

void setup() 
{
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("\n Starting");
    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    pixels.clear();
    pixels.show();

    //reset settings - for testing
    //wm.resetSettings();

    // set configportal timeout
    wm.setConfigPortalTimeout(timeout);
    wm.setTitle("SmartClock Setup");
    wm.setMenu(menu); 
    wm.setConfigPortalBlocking(false);   
    wm.setAPStaticIPConfig(apIP4, apIP4, IPAddress(255,255,255,0));
    wm.autoConnect("SmartClock");

    while(WiFi.status() != WL_CONNECTED)
    {
        wm.process();
        checkConfigResetButton();
        // check for timeout
        if((millis()-startTime) > (timeout*1000))
        {
            Serial.println("failed to connect and hit timeout");
            delay(3000);
            //reset and try again, or maybe put it to deep sleep
            ESP.restart();
            delay(5000);
        }
        for(int i=0; i<NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(0, 0, 200));
            wm.process();
            checkConfigResetButton();
        }
        pixels.show();
    }

    //if you get here you have connected to the WiFi
    //init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("connected...yeey :)");
    Serial.println(WiFi.localIP());
    
    delay(2000);
}

void loop() 
{
    wm.process();
    checkConfigResetButton();
    updateClockLEDS();
}