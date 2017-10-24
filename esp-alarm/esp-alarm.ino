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
const String monitoredDeviceNames[] = {"Benchtop%20Supply%201", "Benchtop%20Supply%202", "Benchtop%20Supply%203", "Metcal", "Desolder%20Station", "Hot%20Air%20Gun"};
const int monitoredDevicePins[] = {D1, D2, D3, D5, D6, D7};
const int monitoredDevices = 6;
int deviceRemoved[] = {0,0,0,0,0,0};
const int buzzFor = 5000;
long ringUntil = 0;
long checkRemoved = 0;

// Other config
const int buzzerPin = D4;
const char* deviceName = "Electronics_Bench";
//const String secretKey = "nope"; // Used for jaimyn's slack api proxy
const String secretKey = "<jaimyns api key>"; // Used for jaimyn's slack api proxy

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
    pinMode(monitoredDevicePins[x], INPUT_PULLUP);
  }

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

  // Post in the slack channel that we've (re)connected
  makeRequest("?key=" + secretKey + "&tool=test&message=alarm_online");

  // Handle the webserver requests
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  pinMode(buzzerPin, OUTPUT);
  buzzer(1);
  delay(500);
  buzzer(0);
  
  Serial.println("\n~~~==== Webserver started and setup complete ====~~~");
}

void loop() {
  
  // Handle web server requests if necessary
  server.handleClient();

  if (millis() >= ringUntil) {
    buzzer(0);
  }

  // every 5 minutes? send a message saying that the item has still been removed
  if (millis() >= checkRemoved) {
    for (int x = 0; x < monitoredDevices; x++) {
      if (deviceRemoved[x]) {
        Serial.print(monitoredDeviceNames[x] + " is still removed, please remember to return this to the workbench as soon as possible!\n");
        // TODO make it talk to slack
      }
    }
    checkRemoved = millis() + 300000;
  }

  // If we have WiFi do the alarm stuff
  if (WiFi.status() == WL_CONNECTED) {
    for (int x = 0; x < monitoredDevices; x++) {
      if (digitalRead(monitoredDevicePins[x]) == 1 && deviceRemoved[x] == 0) {
        // device has been removed
        Serial.println(monitoredDeviceNames[x] + " has been removed!\n");
        deviceRemoved[x] = 1;
        makeRequest("?key=" + secretKey + "&tool=" + monitoredDeviceNames[x] + "&message=item_taken");
        delay(300);
        // activate buzzer thingo for x seconds?
        ringUntil = millis() + buzzFor;
        
      } else if (digitalRead(monitoredDevicePins[x]) == 0 && deviceRemoved[x] == 1) {
        // device has been returned
        Serial.println(monitoredDeviceNames[x] + " has been returned!\n");
        deviceRemoved[x] = 0;
        makeRequest("?key=" + secretKey + "&tool=" + monitoredDeviceNames[x] + "&message=item_returned");
        delay(300);
        // activate buzzer thingo for x seconds?
        ringUntil = millis() + buzzFor/2;
      }
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
  http.begin("https://URL/" + url, "E6 28 AC 9C 36 89 29 0B 4C 08 76 F3 5E CD 4C C6 A8 1B F3 85"); //HTTPS
  //http.begin(url); //HTTP

  Serial.print("[HTTP] GETing URL" + url + " ...\n");
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

