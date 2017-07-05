#include <Wire.h>
#include "SSD1306.h"
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include "images.h"

#define MQTT_VERSION MQTT_VERSION_3_1_1
#define DHTPIN D2
#define DHTTYPE DHT22

const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_SSID_PASSWORD";

const char* MQTT_CLIENT_ID = "nodemcu_temp534543";
const char* MQTT_SERVER_IP = "YOUR_MQTT_SERVER_IP";
const uint16_t MQTT_SERVER_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASSWORD = "";
const char* MQTT_SENSOR_TOPIC = "home/livingroom/temperature";
//const uint16_t SLEEPING_TIME_IN_SECONDS = 300; // 5 minutes x 60 seconds

SSD1306 display(0x3c, D3, D5);
DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

int counter = 1;

float h;
float t;
float hic;

void publishData(float p_temperature, float p_humidity) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["temperature"] = String(p_temperature);
  root["humidity"] = String(p_humidity);
  root.prettyPrintTo(Serial);
  Serial.println("");
  char data[200];
  root.printTo(data, root.measureLength() + 1);
  client.publish(MQTT_SENSOR_TOPIC, data, true);
}

void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
}

void reconnectMqtt() {
  while (!client.connected()) {
    Serial.print("INFO: Attempting MQTT connection...");
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("INFO: connected");
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      delay(5000);
    }
  }
}

void reconnectWifi() {
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void getSensorData() {
  h = dht.readHumidity();
  t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  hic = dht.computeHeatIndex(t, h, false);
}

void drawDisplayTemp() {
  int hicRounded = (int) round(hic);
  int hRounded = (int) round(h);
  int tRounded = (int) round(t);
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "heat: " + String(hicRounded));
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 14, "humi: " + String(hRounded) + " %");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 36, String(tRounded) + " °C");
  
  Serial.print("INFO: Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("INFO: Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("INFO: Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  Serial.println("");
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  reconnectWifi();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.println("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);

  dht.begin();

  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

}

void drawAdditionalData() {
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(10, 128, String(millis()));
  display.display();
}

void drawTempImage() {
  display.drawXbm(70, 0, Thermo_Icon_width, Thermo_Icon_height, Thermo_Icon_bits);
}

void closeConnections() {
  Serial.println("INFO: Closing the MQTT connection");
  client.disconnect();

  //Serial.println("INFO: Closing the Wifi connection");
  //WiFi.disconnect();
  //ESP.deepSleep(SLEEPING_TIME_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
}

void loop() {
  Serial.println("INFO: get sensor data");
  getSensorData();
  Serial.println("INFO: drawing display");
  
  if (!isnan(h) && !isnan(t)) {
    display.clear();
    drawDisplayTemp();
    drawTempImage();
    drawAdditionalData();
    
    if(counter >= 10) {
      Serial.println("INFO: send data to MQTT");
      if (WiFi.status() != WL_CONNECTED) {
        reconnectWifi();
      }
      if (!client.connected()) {
        reconnectMqtt();
      }
      client.loop();
      
      publishData(t, h);
      closeConnections();
      counter = 1;
    }
  
    counter++;
    delay(5000);
  } else {
    Serial.println("WARNING: wrong sensor data, retry");
    delay(500);
  }
  
}

