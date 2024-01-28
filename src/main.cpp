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
#include <pButton.h>

// Copy of sample_defaults.h to defaults.h to customise any defaults
// defaults.h is ignored by GIT

// @Todo add defaults version 1/1/2024
#if __has_include ("defaults.h")
  #include <defaults.h>
#else
  #warning header file defaults.h missing using sample_defaults.h
  #include <sample_defaults.h>
#endif

// Used to track structure of Preferences name space.
#define PREFERENCES_NAME_SPACE "AUTOSHIFTER"
#define PREFERENCES_VERSION 4

// Servo Object
Servo myservo;

// Smart button object
pButton up_button(PIN_BUTTON_UP,INPUT_PULLUP,BUTTON_SAMPLES);
pButton down_button(PIN_BUTTON_DOWN,INPUT_PULLUP,BUTTON_SAMPLES);


// NVM / Preferences
Preferences preferences;

// Forward declerations
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void wsOnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void taskButtonLoop( void *pvParamaters );
void servo_move(uint16_t position );
void servo_attach();
void servo_detach();
TaskHandle_t Task1;

const char* wifi_ssid = WIFI_SSID;
#ifdef WIFI_PASSWORD
  const char* wifi_password = WIFI_PASSWORD;
#endif

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
  time_t lastServoMoveEpoch = 0;
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

void preferencesWrite(){
  preferences.putUShort("version", PREFERENCES_VERSION);
  preferences.putUShort("uDegrees", shifterState.upDegrees);
  preferences.putUShort("nDegrees", shifterState.neutralDegrees);
  preferences.putUShort("mPDegrees", shifterState.midPointDegrees);
  preferences.putUShort("dDegrees", shifterState.downDegrees);
  preferences.putUShort("holdDelay", shifterState.holdDelay);
  preferences.putUShort("nPTime", shifterState.neutralPressTime);
}

void preferencesRead(){
  if( preferences.isKey("version") == false || preferences.getUShort("version") != PREFERENCES_VERSION ){
    // Empty name space or name space structure has changed, push defaults
    shifterState.upDegrees = shifterState.defaultUpDegrees;
    shifterState.midPointDegrees = shifterState.defaultMidPointDegrees;
    shifterState.neutralDegrees = shifterState.defaultNeutralDegrees;
    shifterState.downDegrees = shifterState.defaultDownDegrees;
    shifterState.holdDelay = shifterState.defaultHoldDelay;
    shifterState.neutralPressTime = shifterState.defaultNeutralPressTime;
    shifterState.hasFromDefaults = true;
    preferencesWrite();
    return;
  }
  shifterState.upDegrees = preferences.getUShort("uDegrees");
  shifterState.neutralDegrees = preferences.getUShort("nDegrees");
  shifterState.midPointDegrees = preferences.getUShort("mPDegrees");
  shifterState.downDegrees = preferences.getUShort("dDegrees");
  shifterState.holdDelay = preferences.getUShort("holdDelay");
  shifterState.neutralPressTime = preferences.getUShort("nPTime");
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
  // Check to see if we are in second and need to shift to neutral
  if( downPressed>shifterState.neutralPressTime && shifterState.currentGearPositonId==3 ){
    servo_move(shifterState.neutralDegrees);
    delay(shifterState.holdDelay);
    servo_move(shifterState.midPointDegrees);
    shifterState.currentGearPositonId--;
    Serial.printf("Change from 2nd to neutral, do half shift button press time:%d servo pos:%d\n",downPressed,shifterState.neutralDegrees);
    wsSendGearUpdate(downPressed);
    return;
  }
  if( downPressed>DEFAULT_MINIMUM_BUTTON_PRESS_TIME ){
    servo_move(shifterState.downDegrees);
    delay(shifterState.holdDelay);
    servo_move(shifterState.midPointDegrees);
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
    servo_move(shifterState.neutralDegrees);
    delay(shifterState.holdDelay);
    servo_move(shifterState.midPointDegrees);
    shifterState.currentGearPositonId++;
    Serial.printf("Change from 1st to neutral, do half shift button press time:%d servo pos:%d\n",upPressed,shifterState.neutralDegrees);
    wsSendGearUpdate(upPressed);
    return;
  }

  if( upPressed>DEFAULT_MINIMUM_BUTTON_PRESS_TIME ){
    
    servo_move(shifterState.upDegrees);
    delay(shifterState.holdDelay);
    servo_move(shifterState.midPointDegrees);
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
      preferencesWrite();
    }
    // Send response page using index.html via templateProcessor
    Serial.printf("Send index.html page\n");
    request->send(LittleFS,"/html/index.html","text/html",false,templateProcessor);
  });

  server.on("/setdefaults", HTTP_GET, [](AsyncWebServerRequest * request){
    preferences.clear();
    preferencesRead();
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

void taskButtonLoop( void *pvParamaters ){
  while(true){ // Tasks should never exit.
    delay(10);
    up_button.poll();
    down_button.poll();
  }
}

void servo_attach(){
    // Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(SERVO_FREQUENCY_HZ);    // standard 50 hz servo
	myservo.attach(PIN_SHIFTER_SERVO, SERVO_MINIMUM_PULSE_WIDTH, SERVO_MAXIMUM_PULSE_WIDTH); // 500 - 2400 for 9G SG90
}

void servo_detach(){
  if( myservo.attached() ){
    myservo.detach();
  }
}

void servo_move(uint16_t position ){
  shifterState.lastServoMoveEpoch = millis();
  if( !myservo.attached() ){
    servo_attach();
  }
  myservo.write(position);
}



void setup(){
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.begin(115200);
  xTaskCreatePinnedToCore(taskButtonLoop,"button",8096,NULL,1,&Task1,1);
  preferences.begin( PREFERENCES_NAME_SPACE, false);
  preferencesRead();

  if( PIN_NEUTRAL_LED > 0 ) pinMode(PIN_NEUTRAL_LED,OUTPUT);

  // Start file system  
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  servo_move(shifterState.midPointDegrees);
  shifterState.currentGearPositonId = 2; // Assume neutral on start

  wsSendGearUpdate(0);
  #ifdef WIFI_PASSWORD
    WiFi.softAP(wifi_ssid,wifi_password);
  #else
    WiFi.softAP(wifi_ssid);
  #endif
  
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
  if( upPressed>0 || downPressed>0 ){
    Serial.printf("Hardware button press time button_up:%d  button_down:%d\n",upPressed,downPressed);
  }
  esp_task_wdt_reset();
  dnsServer.processNextRequest();
  checkGearChange(upPressed,downPressed);
  if( DEFAULT_SERVO_IDLE_TIMER > 0 ){
    //
    time_t servoRunTime = millis() - shifterState.lastServoMoveEpoch;
    if( servoRunTime >  DEFAULT_SERVO_IDLE_TIMER ){
      servo_detach();
    }
  }
  delay(50);
}
