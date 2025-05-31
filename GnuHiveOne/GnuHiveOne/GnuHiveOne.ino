#include <WiFiS3.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include "Arduino_LED_Matrix.h"   // Include the LED_Matrix library

ArduinoLEDMatrix matrix;

// WiFi credentials
const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

// MQTT settings
const char broker[] = "a0da1bf1.ala.eu-central-1.emqxsl.com";
int port = 8883;
const char mqttUsername[] = HIVE_USER;
const char mqttPassword[] = HIVE_PASS;

// Topics
const char mqttPubTopic[] = "home/sensor-data";
const char mqttSubTopic[] = "home/set-module-state";

// Pins

// MQTT clients
WiFiSSLClient wifiClient;
MqttClient mqttClient(wifiClient);

bool fanState = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  matrix.begin();
  matrix.loadFrame(LEDMATRIX_EMOJI_BASIC);

  // WiFi
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
  Serial.print(".");
  delay(1000);
  }

  Serial.println("Connected to WiFi");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  while (WiFi.localIP() == INADDR_NONE) {
    Serial.println("Waiting for IP...");
    delay(500);
  }

  Serial.println(WiFi.localIP());

  IPAddress ip;
  if (WiFi.hostByName(broker, ip)) {
    Serial.print("Resolved IP: ");
    Serial.println(ip);
  } else {
    Serial.println("DNS resolution failed!");
  }

  // MQTT auth
  mqttClient.setUsernamePassword(mqttUsername, mqttPassword);
  
  // Connect MQTT
  Serial.print("Connecting to HiveMQ...");

  int mqttResult = mqttClient.connect(broker, port);
  if (!mqttResult) {
    matrix.loadFrame(LEDMATRIX_EMOJI_SAD);
    Serial.print("MQTT failed: ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.println("connected.");

  mqttClient.subscribe(mqttSubTopic);
  Serial.println("Subscribed to: " + String(mqttSubTopic));

  matrix.loadSequence(LEDMATRIX_ANIMATION_HEARTBEAT_LINE);
  matrix.begin();
  matrix.play(true);

  delay(1000);
}

void loop() {
  matrix.loadFrame(LEDMATRIX_EMOJI_HAPPY);
  mqttClient.poll();

  // Handle incoming JSON messages
  if (mqttClient.available()) {
    String payload = mqttClient.readString();

    Serial.print("Incoming JSON: ");
    Serial.println(payload);

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
      Serial.print("JSON parse error: ");
      Serial.println(err.c_str());
      return;
    }

    const char* module = doc["Module"];
    const char* state = doc["State"];

    if (module && state) {
      Serial.print("Module: ");
      Serial.print(module);
      Serial.print(" - State: ");
      Serial.println(state);

      // Add pin logic here
      // Example:
      // 

      if (strcmp(module, "Fan") == 0) {
        if (strcmp(state, "ON") == 0) {
          matrix.loadFrame(LEDMATRIX_HEART_BIG);
          delay(1000);
          matrix.loadFrame(LEDMATRIX_HEART_SMALL);
          delay(1000);
        } else {
          matrix.loadFrame(LEDMATRIX_BLUETOOTH);
          delay(1000);
        }
      }

      if (strcmp(module, "Light") == 0) { 
        if (strcmp(state, "ON") == 0) {
          matrix.loadFrame(LEDMATRIX_EMOJI_SAD);
          delay(1000);
          matrix.loadFrame(LEDMATRIX_EMOJI_BASIC);
          delay(1000);
        } else {
          matrix.loadFrame(LEDMATRIX_BOOTLOADER_ON);
          delay(1000);
        }
      }

      if (strcmp(module, "Pump") == 0) { 
        if (strcmp(state, "ON") == 0) {
          matrix.loadFrame(LEDMATRIX_LIKE);
          delay(1000);
          matrix.loadFrame(LEDMATRIX_MUSIC_NOTE);
          delay(1000);
        } else {
          matrix.loadFrame(LEDMATRIX_CHIP);
          delay(1000);
        }
      }

    } else {
      Serial.println("Missing 'module' or 'state' fields in JSON.");
    }
}

  // Periodic JSON publish
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 10000) {
    lastMillis = millis();

    // Sensor logic here

    // Create JSON object
    StaticJsonDocument<256> doc;
    doc["device"] = "GNU_ONE";
    doc["temperature"] = "Simulated data";
    doc["fan"] = "dummy fan state";
    doc["light"] = "dummy light state";
    doc["pump"] = "dummy pump state";

    String jsonOut;
    serializeJson(doc, jsonOut);

    mqttClient.beginMessage(mqttPubTopic);
    mqttClient.print(jsonOut);
    mqttClient.endMessage();

    Serial.print("Published JSON: ");
    Serial.println(jsonOut);
  }
}