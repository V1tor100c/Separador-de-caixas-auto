#include <Arduino.h>

const int stepPin = 25;
const int dirPin = 26;
const int frequenciaMotor = 200;

void ligar_esteira();
void desligar_esteira();

void setup(){
  Serial.begin(115200);

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(stepPin, LOW);
  digitalWrite(dirPin, LOW);
  ledcSetup(0, frequenciaMotor, 8);
  ledcAttachPin(stepPin, 0);
}

void loop(){
  ligar_esteira();
  delay(3000);
}

void ligar_esteira(){
  digitalWrite(dirPin, HIGH);
  ledcWrite(0, 128);
}

void desligar_esteira(){
  digitalWrite(dirPin, LOW);
  ledcWrite(0, 0);
}