#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "DIGI-MA4c";
const char* password = "2N9UCsjDFk";
const char* serverIP = "192.168.1.140";  // Replace with the IP address of server NodeMCU 
const int lightSensorPin = A0;

// for ambientLighting
const int readingsTolerance = 10;
const int required_numberOfReadings = 10;
int numberOfReadings = 0;
float ambientLightValue[10];

WiFiClient wifiClient;

void connect_to_wifi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void setup() {
  Serial.begin(115200);
  connect_to_wifi();
}

void loop() {
  float lightValue = (analogRead(lightSensorPin))/1024.0 * 100;
  ambientLightValue[numberOfReadings++] = lightValue;

  if (numberOfReadings == required_numberOfReadings){
    // calculate the average value of lightPercentage
    float average_ambientLightValue = 0;
    for(int i = 0; i < numberOfReadings; ++i){
        average_ambientLightValue += ambientLightValue[i];
     }
    average_ambientLightValue /= required_numberOfReadings;
    average_ambientLightValue = round(average_ambientLightValue);
    
//    Serial.println(average_ambientLightValue);
    

    // remove odd values and calculate again
    for(int i = 0; i < numberOfReadings; ++i){
        if(abs(average_ambientLightValue - ambientLightValue[i] >= readingsTolerance)){
              ambientLightValue[i] = 0;
          }
     }
    average_ambientLightValue = 0;
    for(int i = 0; i < numberOfReadings; ++i){
        average_ambientLightValue += ambientLightValue[i];
    }

    average_ambientLightValue /= required_numberOfReadings;
    average_ambientLightValue = round(average_ambientLightValue);

//    Serial.println(average_ambientLightValue);
    
    // send the lightPercentage value
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = "http://" + String(serverIP) + "/ambientLighting?value=" + String(average_ambientLightValue);
        http.begin(wifiClient, url);

        Serial.println(url);
    
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

    numberOfReadings = 0;
  }

}
