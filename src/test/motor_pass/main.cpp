#include <Arduino.h>

const int stepPin = 16;
const int dirPin = 4;
const int frequenciaMotor = 120;

void ligar_esteira();
void desligar_esteira();

void setup(){
  Serial.begin(115200);

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  // digitalWrite(stepPin, LOW);
  digitalWrite(dirPin, LOW);
  ledcSetup(0, frequenciaMotor, 8);
  ledcAttachPin(stepPin, 0);
}

void loop(){
  // digitalWrite(stepPin, HIGH);
  // Serial.println("");
  // delay(100);
  // digitalWrite(stepPin, LOW);
  // delay(100);
  ligar_esteira();
}

void ligar_esteira(){
  ledcWrite(0, 128);
}

void desligar_esteira(){
  ledcWrite(0, 0);
}