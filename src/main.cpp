#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <Preferences.h>
#include <ESP32Servo.h>
#include "LittleFS.h"
#include <ArduinoJson.h>
#include <cstring>
#include <string>
#include <esp_task_wdt.h>
#include <smartButton.h>

// Copy of sample_defaults.h to defaults.h to customise any defaults
// defaults.h is ignored by GIT

// @Todo add defaults version 1/1/2024
#if __has_include ("defaults.h")
  #include <defaults.h>
#else
  #warning header file defaults.h missing using sample_defaults.h
  #include <sample_defaults.h>
#endif


// Servo Object
Servo myservo;

// Smart button object
smartButton up_button(PIN_BUTTON_UP,INPUT_PULLUP,BUTTON_DEBOUNCE_MS);
smartButton down_button(PIN_BUTTON_DOWN,INPUT_PULLUP,BUTTON_DEBOUNCE_MS);


// NVM / Preferences
Preferences preferences;

// Forward declerations
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void wsOnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

const char* wifi_ssid = WIFI_SSID;
const char* wifi_password = WIFI_PASSWORD;
IPAddress ip_address;
DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Buffer space for JSON data processing
StaticJsonDocument<2048> json_rx;
StaticJsonDocument<2048> json_tx;

struct shifterState_t {
  bool hasFormUpdated = false;
  bool hasFromDefaults = false;
  const uint16_t defaultUpDegrees = DEFAULT_UpDegrees;
  const uint16_t defaultNeutralDegrees = DEFAULT_NeutralDegrees;
  const uint16_t defaultMidPointDegrees = DEFAULT_MidPointDegrees;
  const uint16_t defaultDownDegrees = DEFAULT_DownDegrees;
  const uint16_t defaultHoldDelay = DEFAULT_HoldDelay;
  const uint16_t defaultNeutralPressTime = DEFAULT_NeutralPressTime;

  uint16_t upDegrees;
  uint16_t neutralDegrees;
  uint16_t midPointDegrees;
  uint16_t downDegrees;
  uint16_t holdDelay;
  uint16_t neutralPressTime;
  uint8_t currentGearPositonId = 2; // Gear positon #2 = neutral
};
shifterState_t volatile shifterState;

String templateProcessor(const String& paramName){
  // Process template
  if(paramName == "valueServoUpDegrees"){
    return String(shifterState.upDegrees);
  }
  if(paramName == "valueServoNeutralDegrees"){
    return String(shifterState.neutralDegrees);
  }
  if(paramName == "valueHoldDelay"){
    return String(shifterState.holdDelay);
  }
  if(paramName == "wsGatewayAddr"){
    // normally ws://192.168.4.1/ws
    return String("ws://") + ip_address.toString() + String("/ws");
  }
  if(paramName == "valueServoMidPointDegrees"){
    return String(shifterState.midPointDegrees);
  }
  if(paramName == "valueServoDownDegrees"){
    return String(shifterState.downDegrees);
  }
  if( paramName == "hasFormUpdated" ){
    String hasFormUpdated = shifterState.hasFormUpdated?"true":"false";
    shifterState.hasFormUpdated = false;
    return hasFormUpdated;
  }
  if( paramName == "hasFromDefaults" ){
    String hasFromDefaults = shifterState.hasFromDefaults?"true":"false";
    shifterState.hasFromDefaults = false;
    return hasFromDefaults;
  }
  // We didn't find anything return empty string
  return String();
}

void processFormParamater( const String& fieldName, const String& fieldValue ){
  Serial.printf("field: %s = %s\n",fieldName,fieldValue);
  uint16_t intFieldValue = (uint16_t) fieldValue.toInt();
  if( fieldName == "servoUpDegrees" ){
    shifterState.upDegrees = intFieldValue;
  }
  if( fieldName == "servoNeutralDegrees" ){
    shifterState.neutralDegrees = intFieldValue;
  }
  if( fieldName == "servoMidPointDegrees" ){
    shifterState.midPointDegrees = intFieldValue;
  }
  if( fieldName == "servoDownDegrees" ){
    shifterState.downDegrees = intFieldValue;
  }
  if( fieldName == "holdDelay" ){
    shifterState.holdDelay = intFieldValue;
  }
}

// @Todo have NVS version number to detect firmware / NVS are in sync 1/1/2024
void nvsWrite(){
  preferences.putUShort("upDegrees", shifterState.upDegrees);
  preferences.putUShort("neutralDegrees", shifterState.neutralDegrees);
  preferences.putUShort("midPointDegrees", shifterState.midPointDegrees);
  preferences.putUShort("downDegrees", shifterState.downDegrees);
  preferences.putUShort("holdDelay", shifterState.holdDelay);
  preferences.putUShort("neutralPressTime", shifterState.neutralPressTime);
}

void nvsRead(){
  if( preferences.isKey("upDegrees") == false ){
    // Empty name space ?, push defaults
    shifterState.upDegrees = shifterState.defaultUpDegrees;
    shifterState.midPointDegrees = shifterState.defaultMidPointDegrees;
    shifterState.neutralDegrees = shifterState.defaultNeutralDegrees;
    shifterState.downDegrees = shifterState.defaultDownDegrees;
    shifterState.holdDelay = shifterState.defaultHoldDelay;
    shifterState.neutralPressTime = shifterState.defaultNeutralPressTime;
    shifterState.hasFromDefaults = true;
    nvsWrite();
    return;
  }
  shifterState.upDegrees = preferences.getUShort("upDegrees");
  shifterState.neutralDegrees = preferences.getUShort("neutralDegrees");
  shifterState.midPointDegrees = preferences.getUShort("midPointDegrees");
  shifterState.downDegrees = preferences.getUShort("downDegrees");
  shifterState.holdDelay = preferences.getUShort("holdDelay");
  shifterState.neutralPressTime = preferences.getUShort("neutralPressTime");
}

String getGearPosText( uint8_t gearPosId ){
  String gearPosText = String("1N234567").substring(gearPosId-1,gearPosId);
  return gearPosText;
}

void wsSendGearUpdate(uint16_t pressedTime){
  char buffer[1000];
  uint8_t gearPosId = shifterState.currentGearPositonId;
  String gearPosText = getGearPosText(gearPosId);
  json_tx.clear();
  json_tx["messageType"] = "gearPosition";
  json_tx["payload"]["currentGearPosition"] = gearPosText;
  json_tx["payload"]["gearPosId"] = gearPosId;
  json_tx["payload"]["pressedTime"] = pressedTime;
  size_t lenJson = serializeJson(json_tx, buffer);
  ws.textAll(buffer,lenJson);
  Serial.printf("WS TX-> JSON %s\n",buffer);
  if( PIN_NEUTRAL_LED>0 ) digitalWrite( PIN_NEUTRAL_LED, gearPosText=="N" );
}

void checkGearChange(uint16_t upPressed,uint16_t downPressed){
  if( downPressed>0 ){
    // Serial.printf("Change counter:%d Down Presstime:%d\n", down_button.changeCount, downPressed );
    myservo.write(shifterState.downDegrees);
    delay(shifterState.holdDelay);
    myservo.write(shifterState.midPointDegrees);
    if( shifterState.currentGearPositonId > 1 ){
      shifterState.currentGearPositonId--;
    }
    if( shifterState.currentGearPositonId == 2){
      Serial.println("Shift from 2nd to 1st skip neutral");
      shifterState.currentGearPositonId--;
    }
    Serial.printf("Change down to %s button press time:%d servo pos:%d\n",getGearPosText(shifterState.currentGearPositonId),downPressed,shifterState.downDegrees);
    wsSendGearUpdate(downPressed);
    return;
  }

  // Check to see if we are in first and need to shift to neutral
  if( upPressed>shifterState.neutralPressTime && shifterState.currentGearPositonId==1 ){
    myservo.write(shifterState.neutralDegrees);
    delay(shifterState.holdDelay);
    myservo.write(shifterState.midPointDegrees);
    shifterState.currentGearPositonId++;
    Serial.printf("Change from 1st to neutral, do half shift button press time:%d servo pos:%d\n",upPressed,shifterState.neutralDegrees);
    wsSendGearUpdate(upPressed);
    return;
  }

  if( upPressed>0 ){
    // Serial.printf("Change counter:%d UP Presstime:%d\n", up_button.changeCount, upPressed );
    myservo.write(shifterState.upDegrees);
    delay(shifterState.holdDelay);
    myservo.write(shifterState.midPointDegrees);
    if( shifterState.currentGearPositonId <= 6 ){
      if( shifterState.currentGearPositonId == 1 ){
        // We shift pass neutral
        shifterState.currentGearPositonId++;
        Serial.println("Change up, skip neutral");
      }
      shifterState.currentGearPositonId++;
    }
    Serial.printf("Change up to %s button press time:%d servo pos:%d\n",getGearPosText(shifterState.currentGearPositonId),upPressed,shifterState.upDegrees);
    wsSendGearUpdate(upPressed);
    return;
  }
}

void server_routes(){
  // https://github.com/me-no-dev/ESPAsyncWebServer#handlers-and-how-do-they-work
  server.onNotFound([](AsyncWebServerRequest *request){
    request->redirect("/index.html");
  });

  server.on("/index.html", HTTP_ANY, [](AsyncWebServerRequest * request){
    if( request->method() == HTTP_POST ){
      int paramCount = request->params();
      if( paramCount> 0 ) shifterState.hasFormUpdated = true;
      Serial.printf("Number of params %d\n",paramCount);
      for( int i=0; i<paramCount; i++ ){
        AsyncWebParameter * param = request->getParam(i);
        processFormParamater(param->name(), param->value());
      }
      nvsWrite();
    }
    // Send response page using index.html via templateProcessor
    Serial.printf("Send index.html page\n");
    request->send(LittleFS,"/html/index.html","text/html",false,templateProcessor);
  });

  server.on("/setdefaults", HTTP_GET, [](AsyncWebServerRequest * request){
    preferences.clear();
    nvsRead();
    request->redirect("/index.html");
  });

  // Serve static files
  server.serveStatic("/", LittleFS,"/html/");

    // Websockets init.
  ws.onEvent(wsOnEvent);
  server.addHandler(&ws);
}

void wsOnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
 void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      wsSendGearUpdate(0);
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  char buffer[1000];
  
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    Serial.printf("WS RX rawstring:%s\n",data);
    DeserializationError err = deserializeJson(json_rx,data);
    if (err) {
      Serial.printf("ERROR: deserializeJson() failed with: %s\n",err.c_str() );
      Serial.printf("Can not process websocket request\n");
      return;
    }
    const char* message = json_rx["message"];
    Serial.printf("WS RX->message:%s\n",message);

    // Route websocket message based on messageType
    if( strcmp(message,"gearDown") == 0 ){
      uint16_t pressedTime = json_rx["pressedTime"];
      checkGearChange(0,pressedTime);
    }

    if( strcmp(message,"gearUp") == 0 ){
      uint16_t pressedTime = json_rx["pressedTime"];
      checkGearChange(pressedTime,0);
    }
  }
}

void setup(){
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.begin(115200);
  preferences.begin( NVM_NAME_SPACE, false);
  nvsRead();

  if( PIN_NEUTRAL_LED > 0 ) pinMode(PIN_NEUTRAL_LED,OUTPUT);

  // Start file system  
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(PIN_SHIFTER_SERVO, SERVO_MINIMUM_PULSE_WIDTH, SERVO_MAXIMUM_PULSE_WIDTH); // 500 - 2400 for 9G SG90
  up_button.begin();
  down_button.begin();
  myservo.write(shifterState.midPointDegrees);
  shifterState.currentGearPositonId = 2; // Assume neutral on start
  wsSendGearUpdate(0);
  
  WiFi.softAP(wifi_ssid,wifi_password);
  ip_address = WiFi.softAPIP();
  Serial.println(ip_address);
  dnsServer.start(53, "*", ip_address );
  server_routes();

  // Start web server
  server.begin();
  Serial.print("Webserver started on IP:");
  Serial.println(ip_address);
}

void loop(){
  uint16_t upPressed = up_button.pressTime();
  uint16_t downPressed = down_button.pressTime();
  esp_task_wdt_reset();
  dnsServer.processNextRequest();
  checkGearChange(upPressed,downPressed);
  delay(50);
}
