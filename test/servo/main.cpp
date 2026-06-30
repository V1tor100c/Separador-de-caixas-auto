#include <Arduino.h>
#include <ESP32Servo.h>

const int pinbracoGarra = 13;  //garra
const int pinbracoDireita = 19;  //direita
const int pinbracoEsquerda = 12;  //esquerda
const int pinbracoBase = 5;   // base

Servo servoGarra, servoDireita, servoEsquerda, servoBase;

void garraPegaCaixa(){

  for(int posDegrees = 180; posDegrees >= 130; posDegrees--) {
    servoGarra.write(posDegrees);
    Serial.println(posDegrees);
    delay(20);
  }

  for(int posDegrees = 130; posDegrees <= 180; posDegrees++) {
    servoGarra.write(posDegrees);
    Serial.println(posDegrees);
    delay(20);
  }
}

void basePegaCaixa(){
  for(int posDegrees = 100; posDegrees >= 60; posDegrees--) {
    servoDireita.write(posDegrees);
    Serial.println(posDegrees);
    delay(20);
  }

  servoBase.write(65);
  delay(1000);
}

void levantaCaixa(){

  for(int posDegrees = 60; posDegrees <= 160; posDegrees++) {
    servoDireita.write(posDegrees);
    Serial.println(posDegrees);
    delay(20);
  }

  for(int posDegrees = 100; posDegrees <= 170; posDegrees++) {
    servoEsquerda.write(posDegrees);
    Serial.println(posDegrees);
    delay(20);
  }
}

void abaixaCaixa(){
  servoDireita.write(120);
  Serial.println("Mexe o DIREIRO");
  delay(1000);
  servoEsquerda.write(150);
  Serial.println("Mexe o ESQUERDO");
  delay(1000);
} 

void baseEsteira(){
  for(int posDegrees = 65; posDegrees <= 130; posDegrees++) {
    servoBase.write(posDegrees);
    Serial.println(posDegrees);
    delay(20);

  }
}

void garraLargar(){
  
  servoGarra.write(130);
}

void setup() {

  Serial.begin(115200);
  servoGarra.attach(pinbracoGarra);
  servoDireita.attach(pinbracoDireita);
  servoEsquerda.attach(pinbracoEsquerda);
  servoBase.attach(pinbracoBase);

  servoGarra.write(180);
  servoDireita.write(100);
  servoEsquerda.write(100);
  servoBase.write(180);
  basePegaCaixa();
}

void loop() {

  basePegaCaixa();
  delay(1000);
  abaixaCaixa();
  delay(1000);
  garraPegaCaixa();
  delay(1000);
  levantaCaixa();
  delay(1000);
  baseEsteira();
  delay(1000);
  garraLargar();
  delay(5000);




  // servoBase.write(65);
  // delay(1000);

  // // =-=-=-=-=-=-=-=ABAIXA CAIXA
  // servoDireita.write(120);
  // Serial.println("Mexe o DIREIRO");
  // delay(1000);
  // servoEsquerda.write(100);
  // Serial.println("Mexe o ESQUERDO");
  // delay(1000);
  // //=-=-=-=-=-=-=-=-=-=-=-=-/

  // servoGarra.write(130);
  // delay(1000); 
  // servoGarra.write(180);

  //  // =-=-=-=-=-=-=-=LEVANTA CAIXA
  // servoDireita.write(120);
  // Serial.println("Mexe o DIREIRO");
  // delay(1000);
  // servoEsquerda.write(170);
  // Serial.println("Mexe o ESQUERDO");
  // delay(1000);
  // //=-=-=-=-=-=-=-=-=-=-=-=-/

  // servoBase.write(130);

  // delay(5000);



  // servoGarra.write(180);
  // delay(100);
  // servoDireita.write(100);
  // delay(100);
  // servoEsquerda.write(50);
  // delay(100);
  // // servoBase.write(150);
  // Serial.println("Volta tudo");
  // delay(1000); 
}

// void garraPegaCaixa(){
//   servoGarra.write(130);
//   delay(1000); 
//   servoGarra.write(180);
//   delay(1000);
// }

// void basePegacaixa(){
//   servoBase.write(65);
//   delay(1000);
// }

// void levantaCaixa(){
//   servoDireita.write(120);
//   Serial.println("Mexe o DIREIRO");
//   delay(1000);
//   servoEsquerda.write(170);
//   Serial.println("Mexe o ESQUERDO");
//   delay(1000);
// }

// void abaixaCaixa(){
//   servoDireita.write(120);
//   Serial.println("Mexe o DIREIRO");
//   delay(1000);
//   servoEsquerda.write(100);
//   Serial.println("Mexe o ESQUERDO");
//   delay(1000);
// } 