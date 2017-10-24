#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>

// WiFi/Webserver Details
const char* ssid = "HSBNEWiFi";
const char* password = "HSBNEPortHack";
const int server_port = 80;

// Monitored devices - please make sure they match
const String monitoredDeviceNames[] = {"Soldering Iron 1", "Soldering Iron 2", "Benchtop Supply 1", "Benchtop Supply 1", "Benchtop Supply 1", "Benchtop Supply 1", "Benchtop Supply 1", "Benchtop Supply 1"};
const int monitoredDevicePins[] = {D0, D1, D2, D3, D4, D5, D6, D7};
const int monitoredDevices = 8;

// Other config
const int buzzerPin = D4;
const char* deviceName = "Electronics_Bench";
const String secretKey = "3717ea73-b793-4dd0-8d9a-3603d44ded1"; // Used for jaimyn's slack api proxy

// Program variables
int buzzerState = 0;

ESP8266WebServer server(server_port);

void setup() {
  delay(500);
  Serial.begin(115200); // Start serial
  Serial.println("\n\nStarting Setup\nSetting up pins:");

  // Set up all our pins
  for (int x = 0; x < monitoredDevices; x++) {
    Serial.print(monitoredDeviceNames[x] + ", ");
    pinMode(monitoredDevicePins[x], INPUT);
  }
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, 1);

  Serial.println("\nConnecting to: " + (String)ssid);
  WiFi.begin(ssid, password); // Attempt a WiFi connection in station mode

  // Let's wait until WiFi is connected before anything else
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // We're connected to WiFi so let's print some info
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start the MDNS service (device will be visible by http://deviceName.local)
  if (MDNS.begin(deviceName)) {
    Serial.println("MDNS responder started");
  }

  // Most in the slack channel that we've reconnected
  makeRequest("?key=" + secretKey + "&tool=test&message=alarm_online");

  // Handle the webserver requests
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("\n~~~==== Webserver started and setup complete ====~~~");
}

void loop() {
  // Handle web server requests if necessary
  server.handleClient();

  if (millis() % 1000) toggleBuzzer();

  // If we have WiFi do the alarm stuff
  if (WiFi.status() == WL_CONNECTED) {
    for (int x = 0; x < monitoredDevices; x++) {
      //Serial.print("Alarm Triggered! " + monitoredDeviceNames[x]);
      digitalRead(monitoredDevicePins[x]);
    }
  }
}

void toggleBuzzer() {
  digitalWrite(buzzerPin, !buzzerState);
  buzzerState = !buzzerState;
}

void buzzer(int state) {
  digitalWrite(buzzerPin, state);
  buzzerState = state;
}

void makeRequest(String url) {
  HTTPClient http;

  Serial.print("[HTTP] begin...\n");
  http.begin("https://jabelone.lib.id/security-bot@dev/send_message/" + url, "E6 28 AC 9C 36 89 29 0B 4C 08 76 F3 5E CD 4C C6 A8 1B F3 85"); //HTTPS
  //http.begin(url); //HTTP

  Serial.print("[HTTP] GETing https://jabelone.lib.id/security-bot@dev/send_message/" + url + " ...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] got code: %d", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.print("\nResponse: ");
      Serial.println(payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

