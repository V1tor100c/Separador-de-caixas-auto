#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Inicializa o objeto do PCA9685 com o endereço I2C padrão (0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Dependendo do seu servo (SG90, MG995, etc.), os valores de pulso mínimo e máximo 
// (em "ticks" de 0 a 4095) podem variar. Estes valores são um bom ponto de partida.
#define SERVOMIN  110 // Comprimento de pulso mínimo (equivale a ~0 graus)
#define SERVOMAX  410 // Comprimento de pulso máximo (equivale a ~180 graus)

// Define os canais do PCA9685 onde os servos estão conectados
uint8_t canalServo1 = 0; // Conectado na porta 0
uint8_t canalServo2 = 1; // Conectado na porta 1
uint8_t canalServo3 = 2;
uint8_t canalServo4 = 3;

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando controle de servos com PCA9685 e ESP32!");

  // Inicia a comunicação com o módulo
  pwm.begin();
  
  // Frequência do oscilador interno do PCA9685 (geralmente 27MHz, ajusta a precisão)
  pwm.setOscillatorFrequency(27000000);
  
  // Servos analógicos padrão operam a uma frequência de 50 Hz
  pwm.setPWMFreq(50);

  delay(10);
}

// Função auxiliar para converter o ângulo (0 a 180) para o valor de pulso PWM
int converterAnguloParaPulso(int angulo) {
  // Mapeia o valor do ângulo para o intervalo de pulsos do PCA9685
  return map(angulo, 0, 180, SERVOMIN, SERVOMAX);
}

void loop() {
  Serial.println("Movendo servos de 0 a 180 graus...");
  


  // Varredura de 0 até 180 graus
  for (int angulo = 50; angulo <= 120; angulo += 10) {
    // Envia o sinal PWM para os respectivos canais
    pwm.setPWM(canalServo1, 0, converterAnguloParaPulso(angulo));
    pwm.setPWM(canalServo2, 0, converterAnguloParaPulso(180 - angulo)); // Move invertido
    pwm.setPWM(canalServo3, 0, converterAnguloParaPulso(angulo));
    pwm.setPWM(canalServo4, 0, converterAnguloParaPulso(201));
    delay(50); // Pequena pausa para dar tempo ao servo de se mover
  }

  delay(1000); // Aguarda 1 segundo nas posições finais

  Serial.println("Movendo servos de 180 a 0 graus...");
  
  // Varredura de 180 até 0 graus
  for (int angulo = 120; angulo >= 50; angulo -= 10) {
    pwm.setPWM(canalServo1, 0, converterAnguloParaPulso(angulo));
    pwm.setPWM(canalServo2, 0, converterAnguloParaPulso(180 - angulo));
    pwm.setPWM(canalServo3, 0, converterAnguloParaPulso(angulo));
    pwm.setPWM(canalServo4, 0, converterAnguloParaPulso(180));
    delay(50);
  }

  delay(1000); // Aguarda 1 segundo antes de reiniciar o loop
}


















// #include <Arduino.h>
// #include <ESP32Servo.h>

// const int pinbracoGarra = 13;  //garra
// const int pinbracoDireita = 19;  //direita
// const int pinbracoEsquerda = 12;  //esquerda
// const int pinbracoBase = 5;   // base

// Servo servoGarra, servoDireita, servoEsquerda, servoBase;

// void garraPegaCaixa(){

//   for(int posDegrees = 180; posDegrees >= 130; posDegrees--) {
//     servoGarra.write(posDegrees);
//     Serial.println(posDegrees);
//     delay(20);
//   }

//   for(int posDegrees = 130; posDegrees <= 180; posDegrees++) {
//     servoGarra.write(posDegrees);
//     Serial.println(posDegrees);
//     delay(20);
//   }
// }

// void basePegaCaixa(){
//   for(int posDegrees = 100; posDegrees >= 60; posDegrees--) {
//     servoDireita.write(posDegrees);
//     Serial.println(posDegrees);
//     delay(20);
//   }

//   servoBase.write(65);
//   delay(1000);
// }

// void levantaCaixa(){

//   for(int posDegrees = 60; posDegrees <= 160; posDegrees++) {
//     servoDireita.write(posDegrees);
//     Serial.println(posDegrees);
//     delay(20);
//   }

//   for(int posDegrees = 100; posDegrees <= 170; posDegrees++) {
//     servoEsquerda.write(posDegrees);
//     Serial.println(posDegrees);
//     delay(20);
//   }
// }

// void abaixaCaixa(){
//   servoDireita.write(120);
//   Serial.println("Mexe o DIREIRO");
//   delay(1000);
//   servoEsquerda.write(150);
//   Serial.println("Mexe o ESQUERDO");
//   delay(1000);
// } 

// void baseEsteira(){
//   for(int posDegrees = 65; posDegrees <= 130; posDegrees++) {
//     servoBase.write(posDegrees);
//     Serial.println(posDegrees);
//     delay(20);

//   }
// }

// void garraLargar(){
  
//   servoGarra.write(130);
// }

// void setup() {

//   Serial.begin(115200);
//   servoGarra.attach(pinbracoGarra);
//   servoDireita.attach(pinbracoDireita);
//   servoEsquerda.attach(pinbracoEsquerda);
//   servoBase.attach(pinbracoBase);

//   servoGarra.write(180);
//   servoDireita.write(100);
//   servoEsquerda.write(100);
//   servoBase.write(180);
//   basePegaCaixa();
// }

// void loop() {

//   basePegaCaixa();
//   delay(1000);
//   abaixaCaixa();
//   delay(1000);
//   garraPegaCaixa();
//   delay(1000);
//   levantaCaixa();
//   delay(1000);
//   baseEsteira();
//   delay(1000);
//   garraLargar();
//   delay(5000);

// }