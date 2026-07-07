#include <Arduino.h>

const int stepPin = 16;
const int dirPin = 4;
const int enablePin = 15;
const int frequenciaMotor = 100;

void ligar_esteira();
void desligar_esteira();

void setup(){
  Serial.begin(115200);

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  digitalWrite(dirPin, HIGH);
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
  delay(3000);
  desligar_esteira();
  delay(3000);
}

void ligar_esteira(){
  digitalWrite(enablePin, LOW);
  ledcWrite(0, 128);
}

void desligar_esteira(){
  digitalWrite(enablePin, HIGH);
  ledcWrite(0, 0);
}