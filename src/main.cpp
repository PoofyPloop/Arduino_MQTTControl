// I, Rawad Haddad (000777218), certify that this material is my original work. No other person's work has been used without due acknowledgement and I have not made my work available to anyone else.

#include <Arduino.h> 
#include <OneWire.h> 
#include <DallasTemperature.h> 
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>

#include "mqtt_secrets.h"
#include "wifi_secrets.h"

char ssid[] = SECRET_SSID;                    // Change to your network SSID (name).
char pass[] = SECRET_PASS;                    // Change to your network password.

const char* mqttServer  = "test.mosquitto.org";
const uint16_t mqttPort = 1883;
char mqttUserName[] = SECRET_MQTT_USERNAME;      
char mqttPass[]     = SECRET_MQTT_PASSWORD;     
char clientID[]     = SECRET_MQTT_CLIENT_ID;    

// LED states
byte LEDState = HIGH;
int CurrentState;
int PreviousState;

// WiFi client
WiFiClient client;                                 // Initialize the Wi-Fi client library. Uncomment for nonsecure connection.

// MQTT publish/subscribe client
PubSubClient mqttClient( client );

// Pin that the  DS18B20 is connected to 
const int oneWireBus = D3;      
 
// Setup a oneWire instance to communicate with any OneWire devices 
OneWire oneWire(oneWireBus); 
 
// Pass our oneWire reference to Dallas Temperature sensor  
DallasTemperature DS18B20(&oneWire); 

// Sensor address variable
DeviceAddress sensorAddress;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  String powerTopic = topic;
  if (powerTopic == "MohawkCollege/AC/000777218/power") {
    if (payload[0] == 48) {
      LEDState = LOW;
    } else {
      LEDState = HIGH;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("", "", "")) {
      Serial.println("connected");
      DS18B20.requestTemperatures();
      
      float ftemp;
      ftemp = DS18B20.getTempCByIndex(0);

      char buffer[64];
      sprintf(buffer, "%f", ftemp);

      unsigned long time = millis();
      unsigned long seconds = time / 1000, minutes = seconds / 60, hours = minutes / 60;
      time %= 1000;
      hours %= 24;
      minutes %= 60;
      seconds %= 60;

      char timeBuffer[15];
      sprintf(timeBuffer, "%02d:%02d:%02d", hours, minutes, seconds);

      // Once connected, publish an announcement...
      mqttClient.publish("MohawkCollege/AC/000777218/CurrentTemperature", buffer);
      mqttClient.publish("MohawkCollege/AC/000777218/TimeSinceStarted", timeBuffer);
      // ... and resubscribe
      mqttClient.subscribe("MohawkCollege/AC/000798465/CurrentTemperature");
      mqttClient.subscribe("MohawkCollege/AC/000798465/TimeSinceStarted");
      mqttClient.subscribe("MohawkCollege/AC/000777218/power");

    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.println("\n\nMQTTControl");
  Serial.println("Rawad Haddad");
  Serial.println("000777218");
  
  // configure the USB serial monitor 
  Serial.begin(115200); 
 
  // Start the DS18B20 sensor 
  DS18B20.begin(); 

  pinMode(D4, OUTPUT);

  Serial.println("\nTemperature Application");
  
  // condition that indicated whether the sensor is disconnected or not. if connected, returns sensor address
  if (!DS18B20.getAddress(sensorAddress, 0)) 
  {
    Serial.println("No DS18B20 temperature sensores are installed");
  }
  else {
    Serial.print("Found DS18B20 sensor with address: ");
    for (uint8_t i = 0; i < 8 ; i++) {
      if (sensorAddress[i] < 16) {}
        Serial.print(sensorAddress[i], HEX);
    }
    Serial.println();
  }

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);

  // connect to WiFi
  Serial.printf("\nConnecting to %s ", ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}

void loop() {
  digitalWrite(D4, !LEDState);
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
}