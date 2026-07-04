#include <Arduino.h>

const int pinLedR = 19;
const int pinLedG = 18;
const int pinLedB = 5;
const int transistor = 26;



void setup(){
  pinMode(pinLedR, OUTPUT);
  pinMode(pinLedG, OUTPUT);
  pinMode(pinLedB, OUTPUT);
  pinMode(transistor, OUTPUT);

  digitalWrite(pinLedR, LOW);
  digitalWrite(pinLedG, LOW);
  digitalWrite(pinLedB, LOW);
  digitalWrite(transistor, HIGH);
}

void ledR(){
  digitalWrite(pinLedR, HIGH);
  digitalWrite(pinLedG, LOW);
  digitalWrite(pinLedB, LOW);
}

void ledG(){
  digitalWrite(pinLedR, LOW);
  digitalWrite(pinLedG, HIGH);
  digitalWrite(pinLedB, LOW);
}

void ledB(){
  digitalWrite(pinLedR, LOW);
  digitalWrite(pinLedG, LOW);
  digitalWrite(pinLedB, HIGH);
}

void ledY(){
  digitalWrite(pinLedR, HIGH);
  digitalWrite(pinLedG, HIGH);
  digitalWrite(pinLedB, LOW);
}

void ledD(){
  digitalWrite(pinLedR, LOW);
  digitalWrite(pinLedG, LOW);
  digitalWrite(pinLedB, LOW);
}

void loop(){
  ledR();
  delay(1000);

  ledG();
  delay(1000);

  ledB();
  delay(1000);

  ledY();  
  delay(1000);

  ledD();
  delay(1000);
}