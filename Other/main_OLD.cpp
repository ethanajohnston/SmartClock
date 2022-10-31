
/**
 * OnDemandNonBlocking.ino
 * example of running the webportal or configportal manually and non blocking
 * trigger pin will start a webportal for 120 seconds then turn it off.
 * startAP = true will start both the configportal AP and webportal
 */
#include "Arduino.h"

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

// include MDNS
//#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>

#define PIN        27
#define NUMPIXELS 15

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// select which pin will trigger the configuration portal when set to LOW
#define TRIGGER_PIN 0

WiFiManager wm;

unsigned int  timeout   = 120; // seconds to run for
unsigned int  startTime = millis();
unsigned int  timer1 = millis();
bool portalRunning      = false;
bool startAP            = true; // start AP and webserver if true, else start only webserver


void doWiFiManager(){
  // is auto timeout portal running
  if(portalRunning)
  {
    wm.process(); // do processing

    // check for timeout
    if((millis()-startTime) > (timeout*1000))
    {
        Serial.println("portaltimeout");
        portalRunning = false;
        if(startAP)
        {
            wm.stopConfigPortal();
        }
        else
        {
            wm.stopWebPortal();
        } 
   }
  }

  // is configuration portal requested?
  if(digitalRead(TRIGGER_PIN) == LOW && (!portalRunning)) {
    if(startAP){
        Serial.println("Button Pressed, Starting Config Portal");
        wm.setConfigPortalBlocking(false);
        wm.startConfigPortal();
    }  
    else{
        Serial.println("Button Pressed, Starting Web Portal");
        wm.startWebPortal();
    }  
    portalRunning = true;
    startTime = millis();
  }
}


void setup() 
{
    pixels.begin();
    //pixels.setBrightness(50);
    pixels.clear();
    pixels.show();
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP  
    // put your setup code here, to run once
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(1000);
    Serial.println("\n Starting");

    pinMode(TRIGGER_PIN, INPUT_PULLUP);

    wm.resetSettings();
    // wm.setEnableConfigPortal(false);
    wm.setConfigPortalBlocking(false);
    wm.autoConnect("SmartClock");
}

void loop() {

    //doWiFiManager();
    // if((millis()-timer1) > (1*1000))
    // {
    //     Serial.print("RUNNING ");
    //     Serial.println(portalRunning);

    //     timer1 = millis();
    // }
  // put your main code here, to run repeatedly:
}

