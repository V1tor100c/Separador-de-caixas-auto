#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Inicializa o objeto do PCA9685 com o endereço I2C padrão (0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Dependendo do seu servo (SG90, MG995, etc.), os valores de pulso mínimo e máximo 
// (em "ticks" de 0 a 4095) podem variar. Estes valores são um bom ponto de partida.
#define SERVOMIN  110 // Comprimento de pulso mínimo (equivale a ~0 graus)
#define SERVOMAX  410 // Comprimento de pulso máximo (equivale a ~180 graus)

// Define os canais do PCA9685 onde os servos estão conectados
uint8_t ServoGarra = 3; // Conectado na porta 0
uint8_t ServoDireito = 0; // Conectado na porta 1
uint8_t ServoEsquerdo = 1;
uint8_t ServoBase = 2;

uint8_t ultAngGarra = 180;
uint8_t ultAngDireito = 180;
uint8_t ultAngEsquerda = 90;
uint8_t ultAngBase = 100;


// Função auxiliar para converter o ângulo (0 a 180) para o valor de pulso PWM
int converterAnguloParaPulso(int angulo) {
  // Mapeia o valor do ângulo para o intervalo de pulsos do PCA9685
  return map(angulo, 0, 180, SERVOMIN, SERVOMAX);
}

void moveServoG(int ang){
  if(ang > ultAngGarra){
    for(int angulo = ultAngGarra; angulo <= ang; angulo++) {
      pwm.setPWM(ServoGarra, 0, converterAnguloParaPulso(angulo));
      Serial.println(angulo);
      delay(20);
      ultAngGarra = angulo;
    }
  }
  else if(ang < ultAngGarra){
    for(int angulo = ultAngGarra; angulo >= ang; angulo--) {
      pwm.setPWM(ServoGarra, 0, converterAnguloParaPulso(angulo));
      Serial.println(angulo);
      delay(20);
      ultAngGarra = angulo;
    }
  }
  else{
    return;
  }
}

void moveServoD(int ang){
  if(ang > ultAngDireito){
    for(int angulo = ultAngDireito; angulo <= ang; angulo++) {
      pwm.setPWM(ServoDireito, 0, converterAnguloParaPulso(angulo));
      Serial.println(angulo);
      delay(20);
      ultAngDireito = angulo;
    }
  }
  else if(ang < ultAngDireito){
    for(int angulo = ultAngDireito; angulo >= ang; angulo--) {
      pwm.setPWM(ServoDireito, 0, converterAnguloParaPulso(angulo));
      Serial.println(angulo);
      delay(20);
      ultAngDireito = angulo;
    }
  }
  else{
    return;
  }
}

void moveServoE(int ang){
  // Serial.println("ServoEsquerdo");
  // Serial.print("ANG: ");
  // Serial.print(ang);
  // Serial.print(", ULT ANG: ");
  // Serial.println(ultAngEsquerda);
  if(ang > ultAngEsquerda){
    for(int angulo = ultAngEsquerda; angulo <= ang; angulo++) {
      pwm.setPWM(ServoEsquerdo, 0, converterAnguloParaPulso(angulo));
      Serial.println(angulo);
      delay(20);
      ultAngEsquerda = angulo;
    }
  }
  else if(ang < ultAngEsquerda){
    for(int angulo = ultAngEsquerda; angulo >= ang; angulo--) {
      pwm.setPWM(ServoEsquerdo, 0, converterAnguloParaPulso(angulo));
      Serial.println(angulo);
      delay(20);
      ultAngEsquerda = angulo;
    }
  }
  else{
    return;
  }
}

void moveServoB(int ang){
  if(ang > ultAngBase){
    for(int angulo = ultAngBase; angulo <= ang; angulo++) {
      pwm.setPWM(ServoBase, 0, converterAnguloParaPulso(angulo));
      Serial.println(angulo);
      delay(20);
      ultAngBase = angulo;
    }
  }
  else if(ang < ultAngBase){
    for(int angulo = ultAngBase; angulo >= ang; angulo--) {
      pwm.setPWM(ServoBase, 0, converterAnguloParaPulso(angulo));
      Serial.println(angulo);
      delay(20);
      ultAngBase = angulo;
    }
  }
  else{
    return;
  }
}

void levanteDE(){
  int angulod = 195;
  for(float angulo = ultAngEsquerda; angulo <= 170; angulo += 3) {
      pwm.setPWM(ServoEsquerdo, 0, converterAnguloParaPulso(angulo));
      pwm.setPWM(ServoDireito, 0, converterAnguloParaPulso(angulod));
      Serial.print("levata: ");
      Serial.println(angulo);
      angulod -= 3;
      delay(20);
      ultAngEsquerda = angulo;
      ultAngDireito = angulod;
    }
}

void pegarCaixa(){

  moveServoB(35);
  moveServoE(95);
  moveServoD(185);
  moveServoE(60);
  moveServoD(195);
  moveServoB(75);
  moveServoD(205);
  moveServoG(155);
  delay(500);
  moveServoG(205);
  
}

void levarEsteira(){

  moveServoE(70);
  levanteDE();
  moveServoB(155);
  moveServoD(140);
  moveServoE(150);
  moveServoG(180);
}

void servoInit(){
  moveServoB(30);
  moveServoG(160);
  moveServoE(60);
  moveServoD(150);
}


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
void loop() {  
  // moveServoB(75);
  servoInit();
  pegarCaixa();
  delay(500);
  // levarEsteira();
  // delay(500);

  while(1){
    delay(500);
  }
}
