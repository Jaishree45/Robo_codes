#include "BluetoothSerial.h"
#include <WiFi.h>
#include <ArduinoOTA.h>

BluetoothSerial SerialBT;

/* WiFi credentials for OTA */
const char* ssid = "Veda";
const char* password = "Jaishree1451";

/* Motor Pins */
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25

/* Horn Pin */
#define HORN_PIN 4

char command;

/* Direction states */
bool fState = false;
bool bState = false;
bool lState = false;
bool rState = false;

/* ---- Your required motor functions (unchanged) ---- */

void stopMotor()
{
  digitalWrite(IN1, 1);
  digitalWrite(IN2, 1);
  digitalWrite(IN3, 1);
  digitalWrite(IN4, 1);
}

void forward()
{
  digitalWrite(IN3, 0);
  digitalWrite(IN4, 1);
}

void backward()
{
  digitalWrite(IN3, 1);
  digitalWrite(IN4, 0);
}

void left()
{
  digitalWrite(IN1, 0);
  digitalWrite(IN2, 1);
}

void right()
{
  digitalWrite(IN1, 1);
  digitalWrite(IN2, 0);
}

/* Horn */
void hornOn()
{
  digitalWrite(HORN_PIN, 1);
}

void hornOff()
{
  digitalWrite(HORN_PIN, 0);
}

/* Update motor outputs according to states */
void updateMotion()
{
  stopMotor();

  if (fState) forward();
  if (bState) backward();
  if (lState) left();
  if (rState) right();
}

void setupOTA()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  unsigned long startAttempt = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    ArduinoOTA.setHostname("Robo");

    ArduinoOTA.onStart([]() {
      Serial.println("OTA Start");
    });

    ArduinoOTA.onEnd([]() {
      Serial.println("\nOTA End");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
    });

    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("OTA Error[%u]\n", error);
    });

    ArduinoOTA.begin();
    Serial.println("OTA Ready");
  } else {
    Serial.println("WiFi connection failed. OTA not available.");
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(HORN_PIN, OUTPUT);

  stopMotor();
  hornOff();

  SerialBT.begin("Robo");
  Serial.println("Bluetooth Ready");

  setupOTA();
}

void loop()
{
  ArduinoOTA.handle();

  if (SerialBT.available()) {
    command = SerialBT.read();

    Serial.print("Received: ");
    Serial.println(command);

    /* PRESS commands */
    if (command == 'F') fState = true;
    if (command == 'B') bState = true;
    if (command == 'L') lState = true;
    if (command == 'R') rState = true;
    if (command == 'H') hornOn();

    /* RELEASE commands */
    if (command == 'f') fState = false;
    if (command == 'b') bState = false;
    if (command == 'l') lState = false;
    if (command == 'r') rState = false;
    if (command == 'h') hornOff();

    updateMotion();
  }
}
