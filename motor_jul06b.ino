#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Stepper.h>

WiFiClient wifiClient;

const char* ssid = "DIGI-MA4c";
const char* password = "2N9UCsjDFk";
const char* serverIP = "192.168.1.140";

// Stepper initialization and variables
const int NUMBER_OF_STEPS = 1;
const int StepsPerRevolution = 2048;
Stepper myStepper(StepsPerRevolution, D1, D6, D2, D7);

String motorCommand = "halt";   // Initial command
int requestedPercentage = -1;   // -1 means automatic mode, we halt the motor when the server requests.
                // [0-100] targeted percentage, for manual mode.

int currentPosition = 0;    // Current position in steps
const int maxSteps = 5500;    // Maximum steps
int oldPercentage = 0;      // We send updates on curtain position in automatic mode,
int currentPercentage = 0;    // but only when we have at least 1% difference

ESP8266WebServer server(80);

void connect_to_wifi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

// Function to move the motor based on the current command from server
void moveMotorAUTOMATIC() {
  if (motorCommand == "open") {
    if (currentPosition + NUMBER_OF_STEPS <= maxSteps) {
      myStepper.step(NUMBER_OF_STEPS);
      currentPosition += NUMBER_OF_STEPS;
  }
  } else if (motorCommand == "close") {
    if (currentPosition - NUMBER_OF_STEPS >= 0) {
      myStepper.step(-NUMBER_OF_STEPS);
      currentPosition -= NUMBER_OF_STEPS;
    }
  }
  currentPercentage= (currentPosition * 100) / maxSteps;
  if (oldPercentage != currentPercentage){
    send_curtain_position();
    oldPercentage = currentPercentage;
  }
}

// Function to move the motor based on requested percentage from server
void moveMotorMANUAL() {
  if (currentPercentage < requestedPercentage){
    if (currentPosition + NUMBER_OF_STEPS <= maxSteps) {
      myStepper.step(NUMBER_OF_STEPS);
      currentPosition += NUMBER_OF_STEPS;
    }
  } else if (currentPercentage > requestedPercentage){
    if (currentPosition - NUMBER_OF_STEPS >= 0) {
      myStepper.step(-NUMBER_OF_STEPS);
      currentPosition -= NUMBER_OF_STEPS;
    }
  } 
  currentPercentage= (currentPosition * 100) / maxSteps;
}

void setup() {
  Serial.begin(115200);
  connect_to_wifi();
  Serial.print("NodeMCU IP Address: ");
  Serial.println(WiFi.localIP());

  // Set the motor speed (RPM)
  myStepper.setSpeed(15);
  
  // Data receive server NodeMCU
  server.on("/motor", HTTP_GET, []() {
    if (server.hasArg("command") && server.hasArg("percentage")) {
      requestedPercentage = (server.arg("percentage")).toInt();
    motorCommand = server.arg("command");
      Serial.println("Received command " + motorCommand + "\nRequested percentage " + requestedPercentage);
      server.send(200, "text/plain", "Mode received");
    } else {
      server.send(400, "text/plain", "Bad Request");
    }
  });
  
  //debug
  server.on("/debug", []() {
    String datas =  "motorCommand = " + String(motorCommand) + 
                    "\requestedPercentage = " + String(requestedPercentage) + 
                    "\currentPosition = " + currentPosition + 
                    "\maxSteps = " + String(maxSteps) + 
                    "\oldPercentage = " + String(oldPercentage) + 
                    "\currentPercentage = " + String(currentPercentage);
                    
    server.send(200, "text/plain", datas);
  });
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  if (requestedPercentage == -1){
    moveMotorAUTOMATIC();
  }else {
    moveMotorMANUAL();
  }
}

void send_curtain_position() {
  // Send the lightPercentage value
    if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://" + String(serverIP) + "/motor?value=" + String(currentPercentage);
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
