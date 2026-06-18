#include <Arduino.h>

const int stepPin = 15;
const int dirPin = 4;

int timermotor = 1000;
int pass = 500;
int sent = 1;

void motorpass(int passos, int sentido);

void setup() {

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(stepPin, LOW);
  digitalWrite(dirPin, LOW);

}

void loop() {
  motorpass(pass, sent);
  delay(3000);
}

void motorpass(int passos, int sentido){
  if(sentido == 1){
    digitalWrite(dirPin, HIGH); // Sentido Horário
  } else {
    digitalWrite(dirPin, LOW);  // Sentido Anti-Horário
  }

  for(int i = 0; i < passos; i++){
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(timermotor); 
    digitalWrite(stepPin, LOW);
    delayMicroseconds(timermotor);
  }
}







// #include <Arduino.h>

// const int stepPin = 15;
// const int dirPin = 4;
// const int frequenciaMotor = 5;

// void ligar_esteira();
// void desligar_esteira();

// void setup() {

//   pinMode(stepPin, OUTPUT);
//   pinMode(dirPin, OUTPUT);
//   digitalWrite(stepPin, LOW);
//   digitalWrite(dirPin, LOW);
//   ledcSetup(0, frequenciaMotor, 8);
//   ledcAttachPin(stepPin, 0);

// }

// void loop() {
//   ligar_esteira();
//   // delay(3000);
//   // desligar_esteira();
//   // delay(1000);
// }

// void ligar_esteira(){
//   digitalWrite(dirPin, HIGH);
//   ledcWrite(0, 128);
// }

// void desligar_esteira(){
//   digitalWrite(dirPin, LOW);
//   ledcWrite(0, 0);
// }