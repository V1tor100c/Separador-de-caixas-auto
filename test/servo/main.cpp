#include <Arduino.h>
#include <ESP32Servo.h>

static const int servoPin = 13;

Servo servo1;

void setup() {

  Serial.begin(115200);
  servo1.attach(servoPin);
}

void loop() {
    servo1.write(20);
    Serial.println(30);
    delay(1000);
  

  servo1.write(0);
    Serial.println(0);
    delay(1000);
}