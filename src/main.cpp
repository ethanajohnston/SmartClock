//https://www.makeuseof.com/network-time-synchronization-using-the-esp32/
//https://github.com/BertoldVdb/ZoneDetect
//https://www.arduino.cc/reference/en/libraries/wifilocation/

//timezones with DST acounted: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

//web interface should have: rbgw color selection, enable ambient light sensor, ambient light sensor trigger threshold, 
//dark mode brightness, normal mode brightness, manual time entry, location entry (to determine timezone and dst),
//time server selection, setting alarms manually, enable light fade up time, max intensity, link to pdf instructions, location for network setup


//Smart oled screen on vs off. On bootup it stays on for a couple minutes. turns off after. Only shows AP ip and status on finding wifi. 
//No saved AP found. Connect to SmartClock network to configure wifi. Connected. Local webserver at IPADDRESS.

//Save all user paramters to nvm.



#include "Arduino.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WebServer.h>                                // needed to create a simple webserver
#include <WebSocketsServer.h>                         // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>                              // needed for JSON encapsulation (send multiple variables with one string)
#include "time.h"
#include "time_zones.h"

//sudo chmod a+rw /dev/ttyUSB0

// select which pin will trigger the configuration portal when set to LOW
#define TRIGGER_PIN 0

//LED INFO
#define PIN        27
#define NUMPIXELS 60

#define PINHOUR 33
#define NUMPIXELSHOUR 24

// Website once on network
String webpage = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Page Title</title></head><body style='background-color: #EEEEEE;'><span style='color: #002466;'><h1>Smart Clock Configurator</h1><p>Set Timezone: <input type='number'placeholder='timezone'id='set_timezone'/></p><button type='button' id='btn_update' onclick='passive_angle()'>Update Settings</button></p></body><script> var Socket; const TimeZone = document.getElementById('set_timezone'); const Angle_Btn = document.getElementById('btn_update'); let btn_press = false; let delayed = false; const sleep = (milliseconds) => { return new Promise(reslove => setTimeout(reslove, milliseconds)) }; function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { processCommand(event); }; }; function processCommand(event) { var obj = JSON.parse(event.data); document.getElementById('angle').innerHTML = obj.angle; }; function passive_angle(){ send_tz(); btn_press = btn_press == false; }; function send_tz(){ let msg={ \"type\": \"tz\", \"tz\": TimeZone.value }; btn_press = false; Socket.send(JSON.stringify(msg)); }; window.onload = function(event) { init(); };</script></html>";
StaticJsonDocument<200> doc_tx;                       // provision memory for about 200 characters
StaticJsonDocument<200> doc_rx;
WebServer server(80);                                 // the server uses port 80 (standard port for websites
WebSocketsServer webSocket = WebSocketsServer(81);    // the websocket uses port 81 (standard port for websockets

//Global Settings
float timeZone = 0; // for json webserver testing

const char* location = "America/Toronto";
const char* ntpServer = "pool.ntp.org";
int clockMode = 1; //0 = 12hr (Single uses half of the 24 leds), 1 = 12hr (double (uses all 24 leds)), 2 = 24hr mode.
// const long  gmtOffset_sec = 0;
// const int   daylightOffset_sec = 3600;

unsigned int  startTime = millis();
IPAddress apIP4 = IPAddress(10,0,1,1);

int timeout = 120; // seconds to run for
WiFiManager wm;
std::vector<const char *> menu = {"wifi","update"};

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelsHour(NUMPIXELSHOUR, PINHOUR, NEO_GRB + NEO_KHZ800);


void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void initTime(String timezone){
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println("  Failed to obtain time");
    return;
  }
  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}


void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tm;

  tm.tm_year = yr - 1900;   // Set date
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0
  time_t t = mktime(&tm);
  Serial.printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void
updateUserSettings()
{
  //check json values vs old settings values
  //if there is change, update nvm
  //use Preferences.h Library
  //https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
}

void
updateOLED()
{
  //check state global variable for the different states that is happening
  //
}

void 
webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  Serial.println("Socket Write");
  switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      // optionally you can add code here what to do when connected
      break;
    case WStype_TEXT:                                 // if a client has sent data, then type == WStype_TEXT
      // try to decipher the JSON string received
      DeserializationError error = deserializeJson(doc_rx, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        // JSON string was received correctly, so information can be retrieved:
        const char * msg_type = doc_rx["type"];
        Serial.println(String(msg_type));
        if (String(msg_type) == "ctl") {
          Serial.println("ctl msg");
          
        } else if (String(msg_type) == "tz") {
          Serial.println("timezone msg");
          timeZone = doc_rx["tz"];
          Serial.println(String(timeZone));
          
        } else if (String(msg_type) == "speed") {
          Serial.println("speed msg");
        }

      }
      Serial.println("");
      break;
  }
}

void 
Readmsg(String msg) 
{
  String jsonString = "";                           // create a JSON string for sending data to the client
  JsonObject object = doc_tx.to<JsonObject>();      // create a JSON Object
  object["angle"] = "12";                   // write data into the JSON object -> I used "rand1" and "rand2" here, but you can use anything else
  object["speed"] = "2";
  serializeJson(doc_tx, jsonString);                // convert JSON object to string
  Serial.println("****** JSON ******");
  Serial.println(jsonString);                       // print JSON string to console for debug purposes (you can comment this out)
  webSocket.broadcastTXT(jsonString);
  return;
}

void
updateClockLEDS()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
      Serial.println("Failed to obtain time");
      return;
  }

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  pixels.clear();
  pixelsHour.clear();

  int min = timeinfo.tm_min;
  int hour = timeinfo.tm_hour;
  for(int i = 0; i <= min; i++)
  {
    pixels.setPixelColor(i, pixels.Color(0, 0, 1));
  }
  
  // 12 Hour 12 leds
  if(clockMode == 0)
  {
    // Convert to 12 hour time
    if(hour >= 12)
    {
      hour = hour - 12;
    }

    for(int n = 0; n <= hour*2; n = n+2)
    {
      pixelsHour.setPixelColor(n, pixelsHour.Color(0, 0, 10));
    }
  }
  // 12 Hour 24 leds
  else if(clockMode == 1)
  {
    // Convert to 12 hour time
    if(hour >= 12)
    {
      hour = hour - 12;
    }

    for(int n = 0; n <= hour*2; n = n++)
    {
      pixelsHour.setPixelColor(n, pixelsHour.Color(0, 0, 10));
    }
  }
  // 24 hour
  else if(clockMode == 2)
  {
    for(int n = 0; n <= hour; n++)
    {
      pixelsHour.setPixelColor(n, pixelsHour.Color(0, 0, 10));
    }
  }
  pixels.show();
  pixelsHour.show();
}

void
checkConfigResetButton()
{
  // is configuration portal requested?
  if (digitalRead(TRIGGER_PIN) == LOW) 
  {
    delay(200);
    if (digitalRead(TRIGGER_PIN) != LOW)
    {
      //DO SOMETHING! SHOW IP ON OLED? SO... Change global state variable to show ip if the state is not something else (not connecting)
      return;
    }

    delay(4800);
    if (digitalRead(TRIGGER_PIN) == LOW) 
    {
        Serial.println("BUTTON PRESSED. RESETTING. ERASING WIFI SETTINGS.");
        wm.resetSettings();
        ESP.restart();
    }
  }
}

void 
setup() 
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("\n Starting");
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pixels.clear();
  pixelsHour.clear();
  pixels.show();
  pixelsHour.show();

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
          pixels.setPixelColor(i, pixels.Color(0, 100, 0));
          wm.process();
          checkConfigResetButton();
      }
      pixels.show();
  }

  //if you get here you have connected to the WiFi
  //init and get the time
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //configTimeWithTz(getTzByLocation(location), ntpServer);
  initTime(getTzByLocation(location));

  Serial.println("connected...yeey :)");
  Serial.println(WiFi.localIP());
  server.on("/", []()
  {                               // define here wat the webserver needs to do
      server.send(200, "text\html", webpage);         //    -> it needs to send out the HTML string "webpage" to the client
  });
  server.begin();                                     // start server

  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent);                  // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"

  //delay(2000);
}

void 
loop() 
{
  //wm.process();
  server.handleClient();                              // Needed for the webserver to handle all clients
  webSocket.loop();                                   // Update function for the webSockets 
  checkConfigResetButton();
  updateClockLEDS();
  updateUserSettings();
}