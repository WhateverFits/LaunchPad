#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "config.h"

#ifndef APSSID
#define APSSID "ESPap"
#define APPSK  "thereisnospoon"
#endif

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);

const int led = LED_BUILTIN;
int SafetyEngaged = 0;
bool ReadyToFire = false;
int status[PINCOUNT];

/// @brief Enables each relay in order.
/// @param time Duration each relay is enabled.
/// @param live Live delivers firing current to the relays otherwise tests the connections.
void enableLines(unsigned long time, int live)
{
  Serial.print("Enable lines live: ");
  Serial.println(live);
  digitalWrite(LIVEPIN, live);
  delay(10);
  for (int i = 0; i < PINCOUNT; i++)
  {
    demux(i, 3);
    digitalWrite(MUXENABLE, LOW);
    delay(time);

    if (!live)
      status[i] = analogRead(SENSOR);

    digitalWrite(MUXENABLE, HIGH);
  }

  digitalWrite(LIVEPIN, LOW);
}

void testAll()
{
  Serial.println("Testing all lines");
  enableLines(TESTDURATION, LOW);
}

/// @brief Fires all relays. Will not fire if safety is not engaged.
void fireAll()
{
  if (SafetyEngaged)
  {
    enableLines(FIREDURATION, HIGH);
  }
}
void demux(unsigned int in, int count)
{
  int binaryNum[count];
  Serial.print("Demux pattern: ");
  Serial.print(in);
  Serial.print(" ");
  /* assert: count <= sizeof(int)*CHAR_BIT */
  unsigned int mask = 1U << (count - 1);
  int i;
  for (i = 0; i < count; i++)
  {
    binaryNum[i] = (in & mask) ? 1 : 0;
    in <<= 1;
  }
  Serial.print(binaryNum[0]);
  Serial.print(binaryNum[1]);
  Serial.println(binaryNum[2]);
  digitalWrite(muxPins[0], binaryNum[0]);
  digitalWrite(muxPins[1], binaryNum[1]);
  digitalWrite(muxPins[2], binaryNum[2]);
}

String statusString()
{
    String output = "";
    for (int i = 0; i < PINCOUNT; i++)
    {
        output += String(status[i]) + ";";
    }

}
/// @brief Gives basic information for page load. Still need to specifically run status to test connections.
void handleStartup() {
  if (server.method() != HTTP_GET) {
    digitalWrite(led, LOW);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);

    StaticJsonDocument<200> doc;
    if (SafetyEngaged)
      doc["state"] = "Live";
    else
      doc["state"] = "Test";
    
    doc["ready"] = ReadyToFire;
    doc["launchCount"] = PINCOUNT;

    JsonArray launchers = doc.createNestedArray("launchers");
    for (int i = 0; i < PINCOUNT; i++)
    {
      launchers.add(0);
    }

    String output;
    serializeJson(doc, output);

    server.send(200, "text/json", output);
    digitalWrite(led, HIGH);
  }
}

void handleStatus() {
  if (server.method() != HTTP_GET) {
    digitalWrite(led, LOW);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);
    testAll();
    StaticJsonDocument<200> doc;
    if (ReadyToFire)
      doc["state"] = "Live";
    else
      doc["state"] = "Test";
    doc["safety"] = SafetyEngaged;

    doc["launchCount"] = PINCOUNT;

    JsonArray launchers = doc.createNestedArray("launchers");
    for (int i = 0; i < PINCOUNT; i++)
    {
      launchers.add(status[i] > IGNITORTHRESHOLD);
    }
    String output;
    serializeJson(doc, output);

    server.send(200, "text/json", output);
    digitalWrite(led, HIGH);
  }
}

void handleReady()
{
  if (server.method() != HTTP_GET)
  {
    digitalWrite(led, LOW);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, HIGH);
  }
  else
  {
    if (SafetyEngaged)
    {
      digitalWrite(led, LOW);
      digitalWrite(KLAXON, HIGH);
      Serial.println("Klaxon on, ready to fire");
      ReadyToFire = true;
      server.send(200, "text/json", "{'state':'Live'}");
      digitalWrite(led, HIGH);
    } else 
    {
      Serial.println("Attempt made to make ready without safety engaged");
      server.send(200, "text/json", "{'state':'Test'}");
    }
  }
}

void handleStandby() {
  if (server.method() != HTTP_GET) {
    digitalWrite(led, LOW);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);
    ReadyToFire = false;
    digitalWrite(KLAXON, LOW);
    Serial.println("Klaxon off, safe");
    server.send(200, "text/json", "{'state':'Test'}");
    digitalWrite(led, HIGH);
  }
}


void handleLaunch() {
  if (server.method() != HTTP_GET) {
    digitalWrite(led, HIGH);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, LOW);
  } else {
    digitalWrite(led, LOW);
    if (ReadyToFire && SafetyEngaged){
      Serial.println("Firing now");
      fireAll();
      digitalWrite(KLAXON, LOW);
      ReadyToFire = false;
      Serial.println("Done firing, klaxon off");
      server.send(200, "text/json", "{'launch':true,'state':'Test'}");
    } else {
      Serial.println("Fire attempt made and not ready to fire");
      server.send(200, "text/json", "{'launch':false,'state':'Test'}");
    }
    digitalWrite(led, HIGH);
  }
}

void handleForm() {
  if (server.method() != HTTP_POST) {
    digitalWrite(led, LOW);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(200, "text/plain", message);
    digitalWrite(led, HIGH);
  }
}

/// @brief Give a page back for the index. TODO: Add better interface.
void handleRoot() {
  server.send(200, "text/html", "<html><body><h1>You are connected</h1>\
  <div><a href=\"/status/\">Status</a></div> \
  <div><a href=\"/ready/\">Ready</a></div> \
  <div><a href=\"/standby/\">Standby</a></div> \
  <div><a href=\"/launch/\">Launch</a></div> \
  </body></html>");
}

/// @brief 404 response on not found
void handleNotFound() {
  digitalWrite(led, LOW);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, HIGH);
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/", handleRoot);
  server.on("/startup.js", handleStartup);
  server.on("/status.js", handleStatus);
  server.on("/ready.js", handleReady);
  server.on("/standby.js", handleStandby);
  server.on("/launch.js", handleLaunch);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  pinMode(SENSOR, INPUT);
  pinMode(SAFETYPIN, INPUT_PULLUP);
  pinMode(LIVEPIN, OUTPUT);
  pinMode(muxPins[0], OUTPUT);
  pinMode(muxPins[1], OUTPUT);
  pinMode(muxPins[2], OUTPUT);
  pinMode(MUXENABLE, OUTPUT);
  pinMode(KLAXON, OUTPUT);
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  // Turn off the demultiplexer
  digitalWrite(MUXENABLE, HIGH);
}

void loop() {
  server.handleClient();
  SafetyEngaged = !digitalRead(SAFETYPIN);
}
