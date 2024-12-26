#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "LedControl.h"

WiFiClient wifiClient;

const char* ssid = "DIGI-MA4c";
const char* password = "2N9UCsjDFk";
const char* motorIP = "192.168.1.138";

int ambientLight = 0;             // [0-100]

int required_ambientLight = 0;    // [0-100]
String motor_command = "halt";    // open, close or halt
int LEDintensity = 0;             // [0-100]
int curtainPosition = 0;          // [0-100], real data from motor
int required_curtainPosition = 0; // [0-100], data from android app
String operatingMode = "none";    // none, manual, automatic, night

// internal control and state indicator for the LED
int led_intensity = 0;            // [-1 - 15]
// how close we neet get to the desired value, to halt the engine
const int TOLERANCE = 10;


/* Create a new LedControl variable.
 * We use pins D0,D2 and D1 on the NodeMCU for the SPI interface
 * Pin D0 is connected to the DATA IN-pin of the first MAX7221
 * Pin D2 is connected to the CLK-pin of the first MAX7221
 * Pin D1 is connected to the LOAD(/CS)-pin of the first MAX7221
 * There will only be a single MAX7221 attached to the arduino 
 */ 
LedControl lc = LedControl(D0,D2,D1,1);
byte full[8] = {B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111,B11111111};

ESP8266WebServer server(80);

void connect_to_wifi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void led_setup(){
  lc.shutdown(0,false);
  lc.setIntensity(0,1);
  lc.clearDisplay(0);
  for (int j=0; j<8; j++)
  {
    lc.setRow(0,j,full[j]);
  }

  led_intensity = 0;
}

void send_motor_command(String new_motor_command) {
  if (new_motor_command != motor_command){
    motor_command = new_motor_command;
    
    // Send the command, this is for automatic mode
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url =  "http://" + String(motorIP) + "/motor?" +
              "command=" + String(motor_command) +
              "&percentage=" + String(-1);
              
      http.begin(wifiClient, url);

      int httpCode = http.GET();

      if (httpCode > 0) {
        String payload = http.getString();
        Serial.println(payload);
      } else {
        Serial.println("Error on HTTP request");
        Serial.println(httpCode);
      }
    http.end();
    }
  }
}

void send_motor_command(int percentage) {
    
  // Send the percentagem this is for manual mode
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url =  "http://" + String(motorIP) + "/motor?" +
            "command=" + String(motor_command) +
            "&percentage=" + String(percentage);
            
    http.begin(wifiClient, url);

    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.println("Error on HTTP request");
      Serial.println(httpCode);
    }
  http.end();
  }
}

void setup() {
  Serial.begin(115200);
  connect_to_wifi();
  Serial.print("NodeMCU IP Address: ");
  Serial.println(WiFi.localIP());

  led_setup();

  // Data receive from android app
  server.on("/mode", HTTP_GET, []() {
    if (server.hasArg("newMode")) {
    operatingMode = server.arg("newMode");
    Serial.println("Mode changed to: " + operatingMode);
    server.send(200, "text/plain", "Mode received");
    } else {
    server.send(400, "text/plain", "Bad Request");
    }
  });
  server.on("/slider", HTTP_GET, []() {
    if (server.hasArg("slider") && server.hasArg("value")) {
    String slider = server.arg("slider");
    String value = server.arg("value");
    Serial.print("Slider ");
    Serial.print(slider);
    Serial.print(" set to: ");
    Serial.println(value);
    server.send(200, "text/plain", "Slider value received");
    
    // handle new value
    if (slider == "curtain"){
      required_curtainPosition = value.toInt();
    } else if (slider == "lightIntensity"){
      LEDintensity = value.toInt();
    } else if (slider == "roomIntensity"){
      required_ambientLight = value.toInt();
    }
    
    } else {
    server.send(400, "text/plain", "Bad Request");
    }
  });

  // Data send to android app
  server.on("/sensors", HTTP_GET, []() {
    String jsonResponse = "{\"lightIntensity\": " + String(LEDintensity) + 
                          ", \"curtainPosition\": " + String(curtainPosition) + "}";
    server.send(200, "application/json", jsonResponse);
  });

  // Data receive from light sensor
  server.on("/ambientLighting", []() {
    if (server.hasArg("value")) {
    ambientLight = server.arg("value").toInt();
    server.send(200, "text/plain", "Light value received");
    } else {
    server.send(400, "text/plain", "Bad Request");
    }
  });
  
  server.on("/motor", []() {
    if (server.hasArg("value")) {
    curtainPosition = server.arg("value").toInt();
    server.send(200, "text/plain", "curtainPosition value received");
    } else {
    server.send(400, "text/plain", "Bad Request");
    }
  });

  //debug
  server.on("/debug", []() {
    String datas =  "ambientLight = " + String(ambientLight) + 
                    "\nrequired_ambientLight = " + String(required_ambientLight) + 
                    "\nmotor_command = " + motor_command + 
                    "\nLEDintensity = " + String(LEDintensity) + 
                    "\ncurtainPosition = " + String(curtainPosition) + 
                    "\nrequired_curtainPosition = " + String(required_curtainPosition) + 
                    "\noperatingMode = " + String(operatingMode) + 
                    "\nled_intensity = " + String(led_intensity);
                    
    server.send(200, "text/plain", datas);
  });
  
  server.begin();
  Serial.println("HTTP server started");  
}

void loop() {
  server.handleClient();

  if (operatingMode == "none") {
    // Handle NONE mode
    handle_NONE_mode();
    
  } else if (operatingMode == "manual") {
    // Handle MANUAL mode
    handle_MANUAL_mode();
    
  } else if (operatingMode == "night") {
    // Handle NIGHT mode
    handle_NIGHT_mode();
    
  } else if (operatingMode == "automatic") {
    // Handle AUTOMATIC mode
    handle_AUTOMATIC_mode();
    
  } else {
    Serial.println("Unknown operating mode");
  }
  
}

void handle_NONE_mode(){
  ambientLight = 0;
  required_ambientLight = 0;
  send_motor_command("halt");
  LEDintensity = 0;
  curtainPosition = 0;
  operatingMode = "none";
  led_intensity = 0;
  required_curtainPosition = 0;
}

void handle_MANUAL_mode(){
  // LED handle
  const int new_led_intensity = round(LEDintensity / 6.3125); // // 6.3125 = 101/16
  
  if(led_intensity <=0 && new_led_intensity > 0){
    lc.shutdown(0, false);
  } else if (new_led_intensity == 0){
    lc.shutdown(0, true);
    led_intensity = -1;
  }
  led_intensity = new_led_intensity;
  lc.setIntensity(0, led_intensity);
  
  send_motor_command(required_curtainPosition);
  curtainPosition = -1;   // value not corresponding to reality, not needed
}

void handle_AUTOMATIC_mode(){
  if(is_value_tolerated(ambientLight, required_ambientLight)){
    check_for_halt();
  } else if(ambientLight < required_ambientLight){
    send_motor_command("close");
    up_LED_intensity();
  } else if(ambientLight > required_ambientLight){
    send_motor_command("open");
    down_LED_intensity();
  }
  // og technique
//  ambient_value = get_ambient_light();
//
//    if(ambient_value < desired_value){
//        up_intensity();
//    }else if(ambient_value > desired_value){
//        down_intensity();
//    }
}

void handle_NIGHT_mode(){
    down_LED_intensity();
    send_motor_command(100);
}

bool is_value_tolerated(int A, int B){
  return (abs(A - B) < TOLERANCE);
}

void check_for_halt(){
  String upOrDown = (ambientLight <= required_ambientLight) ? "up" : "down";

  if(upOrDown == "up"){
    if(curtainPosition < 100){
      if (led_intensity >= 0){
        send_motor_command("open");
        down_LED_intensity();
      }else{
        Serial.print("Can't supply the required lighting");
        send_motor_command("halt");
      }
    } else {
      send_motor_command("halt");
    }
  } else {
    send_motor_command("halt");
  } 
}

void up_LED_intensity(){
    if(led_intensity == -1){
        lc.shutdown(0, false);
        led_intensity = 0;
    }
    if(led_intensity <= 15){
        lc.setIntensity(0, ++led_intensity);
    }
}

void down_LED_intensity(){
    if(led_intensity >= 0){
        lc.setIntensity(0, --led_intensity);
    }else{
        lc.shutdown(0, true);
        led_intensity = -1;
    }
}
