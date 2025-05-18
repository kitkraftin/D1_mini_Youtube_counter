#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define DATA_PIN D8
#define CS_PIN D7
#define CLK_PIN D6

MD_Parola display = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

const char* ssid = ""; //SSID
const char* password = ""; //Password
const char* apiKey = ""; //API Key
const char* channelId = ""; //Channel Id
const char* host = "www.googleapis.com";

WiFiClientSecure client;
const int refreshButton = D5;  
long subscriberCount = 0;
long viewCount = 0;
char message[30];

unsigned long lastSwitchTime = 0;  
bool showSubscribers = true;
int displayDuration = 5000;  // Default delay before switching

void setup() {
  pinMode(refreshButton, INPUT_PULLUP);
  Serial.begin(115200);
  display.begin();
  display.setIntensity(5);
  display.displayClear();
  display.setTextAlignment(PA_LEFT);
  display.setSpeed(50);
  display.setPause(1000);
  display.setZoneEffect(0, true, PA_FLIP_UD);
  display.setZoneEffect(0, true, PA_FLIP_LR);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
  client.setInsecure();

  fetchYouTubeStats();
  updateDisplay();
}

void loop() {
  if (display.displayAnimate()) {
    display.displayReset();
  }

  if (digitalRead(refreshButton) == LOW) {
    fetchYouTubeStats();
    updateDisplay();
  }

  // Switch only after the full message scrolls
  if (millis() - lastSwitchTime > displayDuration) {
    Serial.println("here");
    lastSwitchTime = millis();
    showSubscribers = !showSubscribers;
    updateDisplay();
  }
}

void fetchYouTubeStats() {
  Serial.println("Fetching YouTube Stats...");
  if (!client.connect(host, 443)) {
    Serial.println("Connection failed!");
    return;
  }

  String url = "/youtube/v3/channels?part=statistics&id=" + String(channelId) + "&key=" + String(apiKey);
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  String response = "";
  while (client.connected() || client.available()) {
    if (client.available()) {
      response += client.readString();
    }
  }
  client.stop();

  int jsonIndex = response.indexOf('{');
  if (jsonIndex != -1) {
    response = response.substring(jsonIndex);
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (!error) {
      subscriberCount = doc["items"][0]["statistics"]["subscriberCount"].as<long>();
      viewCount = doc["items"][0]["statistics"]["viewCount"].as<long>();
      Serial.print("Subscribers: ");
      Serial.println(subscriberCount);
      Serial.print("Views: ");
      Serial.println(viewCount);
    }
  }
}

void updateDisplay() {
  // Ensure display is fully cleared
  display.displayClear();
  display.displayAnimate();  // Force an empty animation frame
  delay(500);  // Give time to fully clear

  // Set the correct message
  if (showSubscribers) {
    sprintf(message, "Subs: %ld", subscriberCount);
    Serial.println("Displaying Subscribers");
  } else {
    sprintf(message, "Views: %ld", viewCount);
    Serial.println("Displaying Views");
  }

  // Calculate display duration dynamically
  int textLength = strlen(message);
  displayDuration = textLength * 750;  // Adjust timing per character

  // Reset display animation before setting new text
  display.displayReset();

  // Display the new text
  display.displayScroll(message, PA_LEFT, PA_SCROLL_RIGHT, 100);
}
