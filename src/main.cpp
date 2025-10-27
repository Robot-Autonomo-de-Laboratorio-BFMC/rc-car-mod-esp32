#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Ticker.h>
#include <ESP32Servo.h>
#include <wifi_data.h>
#include <webpage.cpp>

// Servo steer
const int SERVO_PIN = 25;
Servo steerServo;

// Servo angles
const int SERVO_CENTER = 90;
const int SERVO_LEFT = 45;
const int SERVO_RIGHT = 135;
int currentAngle = SERVO_CENTER;

// Motor drive
const int IN3 = 14;
const int IN4 = 12;
const int ENB = 13;

int SPEED = 255;
const int SPEED_MAX = 255;

// Lights
Ticker autoLightsTicker;
const int LDR = 35;
int LDR_STATUS = 0;
const int HEADLIGHTS = 32;
const int REVERSE_LIGHTS = 33;

WebServer server(80);

void handleRoot()
{
  //html code as char
  server.send(200, "text/html", webpage);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void handleWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  WiFi.begin(SSID, PASSWORD);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(400);
    Serial.print(".");
  }
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("WifiCar"))
  {
    Serial.println("MDNS Responder Started");
  }
}

void checkAutomaticLights()
{
  LDR_STATUS = analogRead(LDR);
  if (LDR_STATUS < 3500)
  {
    Serial.println("HIGH intensity: ");
    Serial.println(LDR_STATUS);
    digitalWrite(HEADLIGHTS, LOW);
  }
  else
  {
    Serial.println("LOW Intensity ");
    Serial.println(LDR_STATUS);
    digitalWrite(HEADLIGHTS, HIGH);
  }
}

void setup(void)
{
  Serial.begin(SERIAL_BAUD);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(REVERSE_LIGHTS, OUTPUT);
  pinMode(HEADLIGHTS, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(HEADLIGHTS, LOW);
  digitalWrite(REVERSE_LIGHTS, LOW);
  
  // Initialize servo
  steerServo.setPeriodHertz(50);
  steerServo.attach(SERVO_PIN, 500, 2500);
  steerServo.write(SERVO_CENTER);
  currentAngle = SERVO_CENTER;
  
  handleWifi();

  server.on("/", handleRoot);
  server.on("/forward", []()
            {
    Serial.println("forward");
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, SPEED);
    server.send(200, "text/plain", "forward"); });

  server.on("/driveStop", []()
            {
    Serial.println("driveStop");
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);
    digitalWrite(REVERSE_LIGHTS, LOW);
    server.send(200, "text/plain", "driveStop"); });

  server.on("/back", []()
            {
    Serial.println("back");
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, SPEED);
    digitalWrite(REVERSE_LIGHTS, HIGH);
    server.send(200, "text/plain", "back"); });

  server.on("/right", []()
            {
    Serial.println("right");
    currentAngle = SERVO_RIGHT;
    steerServo.write(currentAngle);
    server.send(200, "text/plain", "right"); });

  server.on("/left", []()
            {
    Serial.println("left");
    currentAngle = SERVO_LEFT;
    steerServo.write(currentAngle);
    server.send(200, "text/plain", "left"); });

  server.on("/steerStop", []()
            {
    Serial.println("steerStop");
    currentAngle = SERVO_CENTER;
    steerServo.write(currentAngle);
    server.send(200, "text/plain", "steerStop"); });

  server.on("/LightsOn", []()
            {
    Serial.println("headlights On");
    autoLightsTicker.detach();
    digitalWrite(HEADLIGHTS, HIGH);
    server.send(200, "text/plain", "Luces bajas encendidas"); });

  server.on("/LightsOff", []()
            {
    Serial.println("headlights Off");
    autoLightsTicker.detach();
    digitalWrite(HEADLIGHTS, LOW);
    server.send(200, "text/plain", "Luces bajas apagadas"); });

  server.on("/LightsAuto", []()
            {
    Serial.println("headlights automatic");
    autoLightsTicker.attach(1, checkAutomaticLights);
    server.send(200, "text/plain", "Luces bajas automaticas"); });

  server.on("/changeSpeed", []()
            {
    String speedValue = server.arg("speed");
    Serial.println("Speed changed to " + speedValue);
    SPEED = speedValue.toInt();
    server.send(200, "text/plain", "Velocidad limitada a " + speedValue); });

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP Server Started");
}

void loop(void)
{
  server.handleClient();
}