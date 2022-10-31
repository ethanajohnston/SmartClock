
/**************** Libraries ******************************/
#include <Arduino.h>
#include <WiFi.h>                                     // needed to connect to WiFi
#include <WebServer.h>                                // needed to create a simple webserver
#include <WebSocketsServer.h>                         // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>                              // needed for JSON encapsulation (send multiple variables with one string)


/************ Global Variables **********/

// Website
String webpage = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Page Title</title></head><body style='background-color: #EEEEEE;'><span style='color: #002466;'><h1>Smart Clock Configurator</h1><p>Set Timezone: <input type='number'placeholder='timezone'id='set_timezone'/></p><button type='button' id='btn_update' onclick='passive_angle()'>Update Settings</button></p></body><script> var Socket; const TimeZone = document.getElementById('set_timezone'); const Angle_Btn = document.getElementById('btn_update'); let btn_press = false; let delayed = false; const sleep = (milliseconds) => { return new Promise(reslove => setTimeout(reslove, milliseconds)) }; function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function(event) { processCommand(event); }; }; function processCommand(event) { var obj = JSON.parse(event.data); document.getElementById('angle').innerHTML = obj.angle; }; function passive_angle(){ send_tz(); btn_press = btn_press == false; }; function send_tz(){ let msg={ \"type\": \"tz\", \"tz\": TimeZone.value }; btn_press = false; Socket.send(JSON.stringify(msg)); }; window.onload = function(event) { init(); };</script></html>";
StaticJsonDocument<200> doc_tx;                       // provision memory for about 200 characters
StaticJsonDocument<200> doc_rx;
WebServer server(80);                                 // the server uses port 80 (standard port for websites
WebSocketsServer webSocket = WebSocketsServer(81);    // the websocket uses port 81 (standard port for websockets
// AP
const char* ssid = "ServoTesting";
const char* password = "123456789";
float timezone = 0;

IPAddress IP;
/********* Local Function Definitions ********/

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
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
          timezone = doc_rx["tz"];
          Serial.println(String(timezone));
          
        } else if (String(msg_type) == "speed") {
          Serial.println("speed msg");
        }

      }
      Serial.println("");
      break;
  }
}

void Readmsg(String msg) 
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


/********* External Functions ***********/
void setup() {
  Serial.begin(115200);                               // init serial port for debugging
  WiFi.softAP(ssid, password);
  IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  Serial.println("Establishing connection to WiFi with SSID: " + String(ssid));     // print SSID to the serial interface for debugging
  server.on("/", []() {                               // define here wat the webserver needs to do
    server.send(200, "text\html", webpage);         //    -> it needs to send out the HTML string "webpage" to the client
  });
  server.begin();                                     // start server

  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent);                  // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"


}
void loop() 
{
  server.handleClient();                              // Needed for the webserver to handle all clients
  webSocket.loop();                                   // Update function for the webSockets

}