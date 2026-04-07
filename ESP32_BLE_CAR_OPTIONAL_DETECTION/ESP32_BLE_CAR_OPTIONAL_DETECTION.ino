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

/* Ultrasonic Sensor Pins */
#define TRIG_PIN 18
#define ECHO_PIN 19

/* Safety settings */
const float SAFE_DISTANCE_CM = 30.0;
const unsigned long DISTANCE_CHECK_INTERVAL_MS = 10;

char command;

/* Direction states */
bool fState = false;
bool bState = false;
bool lState = false;
bool rState = false;

/* Obstacle detection state */
bool obstacleDetectionEnabled = false;

/* Auto reverse state */
bool autoReverseActive = false;
unsigned long lastDistanceCheck = 0;

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

void clearMotionStates()
{
  fState = false;
  bState = false;
  lState = false;
  rState = false;
}

float readDistanceCM()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.0343 / 2.0;
}

void startAutoReverse()
{
  if (autoReverseActive) return;

  autoReverseActive = true;
  clearMotionStates();
  stopMotor();
  backward();

  Serial.println("Obstacle detected! Auto reverse started.");
}

void stopAutoReverse()
{
  autoReverseActive = false;
  stopMotor();
  updateMotion();
  Serial.println("Auto reverse stopped.");
}

void handleAutoReverse()
{
  if (!autoReverseActive) return;

  if (!obstacleDetectionEnabled) {
    stopAutoReverse();
    return;
  }

  float distance = readDistanceCM();

  if (distance > 0) {
    Serial.print("Auto Reverse Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  if (distance > 0 && distance <= SAFE_DISTANCE_CM) {
    backward();  // keep reversing while obstacle is close
  } else {
    stopAutoReverse();
    Serial.println("Obstacle cleared.");
  }
}

void checkObstacle()
{
  if (!obstacleDetectionEnabled) return;
  if (autoReverseActive) return;

  if (millis() - lastDistanceCheck < DISTANCE_CHECK_INTERVAL_MS) return;
  lastDistanceCheck = millis();

  if (bState) return;  // don't trigger while manually reversing

  float distance = readDistanceCM();

  if (distance > 0) {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  if (distance > 0 && distance <= SAFE_DISTANCE_CM) {
    startAutoReverse();
  }
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

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  stopMotor();
  hornOff();

  SerialBT.begin("Robo");
  Serial.println("Bluetooth Ready");

  setupOTA();
}

void loop()
{
  ArduinoOTA.handle();
  handleAutoReverse();
  checkObstacle();

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

    /* Obstacle detection control */
    if (command == 'D') {
      obstacleDetectionEnabled = true;
      Serial.println("Obstacle detection ENABLED");
    }

    if (command == 'd') {
      obstacleDetectionEnabled = false;
      if (autoReverseActive) {
        stopAutoReverse();
      }
      Serial.println("Obstacle detection DISABLED");
    }

    /* RELEASE commands */
    if (command == 'f') fState = false;
    if (command == 'b') bState = false;
    if (command == 'l') lState = false;
    if (command == 'r') rState = false;
    if (command == 'h') hornOff();

    if (!autoReverseActive) {
      updateMotion();
    }
  }
}
