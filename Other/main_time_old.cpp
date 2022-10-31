#include "Arduino.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "time.h"

//sudo chmod ugo+w /dev/ttyUSB0

// For when the USB is not detected.
//sudo chmod a+rw /dev/ttyUSB0
//https://forum.arduino.cc/t/solved-cant-open-device-dev-ttyusb0-perm-denied-linux-permissions/581358

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;
bool initialSync = false;

#define PIN        27
#define NUMPIXELS 15

// select which pin will trigger the configuration portal when set to LOW
#define TRIGGER_PIN 0

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

//WiFiManager intialize
WiFiManager wm; 

IPAddress apIP4 = IPAddress(10,0,1,1);

void IRAM_ATTR isr() 
{
    Serial.println("Button Pressed. Resetting.");
    wm.resetSettings();
    WiFi.disconnect(false,true);
    ESP.restart();
    delay(1000);
}

void
syncTime()
{
    while(!getLocalTime(&timeinfo))
    {
        //pixels.clear();
        Serial.println("Failed to obtain time");
        //print on screen that no time server
        //pixels.setPixelColor(4, pixels.Color(0, 150, 150));
        //pixels.show();
    }
    Serial.println("Time Synced");
}

void
updateClockLEDS()
{
    if(!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }

    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    pixels.clear();
    int sec = timeinfo.tm_sec;
    pixels.setPixelColor(sec, pixels.Color(150, 150, 150));
    pixels.show();
}

void setup() 
{
    pixels.begin();
    //pixels.setBrightness(50);
    pixels.clear();
    pixels.show();
    // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.

    // put your setup code here, to run once:
    Serial.begin(115200);
    pinMode(2, OUTPUT);

    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    attachInterrupt(TRIGGER_PIN, isr, FALLING);

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    wm.setAPStaticIPConfig(apIP4, apIP4, IPAddress(255,255,255,0));
    //wm.resetSettings();
    wm.setTitle("SmartClock Setup");
    wm.setShowInfoErase(false);    
    wm.setConfigPortalBlocking(false);
    //wm.setConfigPortalTimeout(60);
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    bool res = wm.autoConnect("SmartClock");
    if(res)
    {
        Serial.print("Config Loaded. Already setup. Connected to: ");
        Serial.print(wm.getWiFiSSID());
        Serial.print(" with IP: ");
        Serial.println(WiFi.localIP());
    }
    else 
    {
         Serial.println("Not able to connect to network/ No network available. ");
    }

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() 
{
    // Setup AP first if not already setup
    wm.process();
    while(WiFi.status() != 3)
    {
        wm.process();
        //Serial.print("Please setup wifi connection at: ");
        //Serial.println(apIP4);
        for(int i=0; i<NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(0, 0, 150));
        }
        pixels.show();
    }

        //Intial time syuc sync
    if(initialSync == false)
    {
        syncTime();
        initialSync = true;
    }

    //Serial.println("Running...");
    updateClock();
    delay(500);
}