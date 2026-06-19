#include <Arduino.h>

const int stepPin = 25;
const int dirPin = 26;
const int frequenciaMotor = 200;

void ligar_esteira();
void desligar_esteira();

void setup()
{
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
  // desligar_esteira();
  // delay(1000);
}

void ligar_esteira()
{
  digitalWrite(dirPin, HIGH);
  ledcWrite(0, 128);
}

void desligar_esteira(){
  digitalWrite(dirPin, LOW);
  ledcWrite(0, 0);
}













// // ================== CÓDIGO MÍNIMO PARA TESTE ==================
// // Pinos padrão: STEP = 2, DIR = 3, ENABLE = 4
// // Se não quiser usar o pino ENABLE, conecte-o ao GND e comente as linhas dele.

// #include <Arduino.h>

// const int stepPin = 25;
// const int dirPin  = 26;
// // const int enablePin = 4;   // opcional – pode ser removido

// void setup() {
//   pinMode(stepPin, OUTPUT);
//   pinMode(dirPin, OUTPUT);
//   // pinMode(enablePin, OUTPUT);

//   digitalWrite(stepPin, LOW);
//   digitalWrite(dirPin, LOW);   // LOW = horário (geralmente)
//   // digitalWrite(enablePin, LOW); // LOW = motor habilitado (na maioria dos módulos)
// }

// void loop() {
//   // ---- Gira 400 passos no sentido horário ----
//   digitalWrite(dirPin, LOW);
//   for (int i = 0; i < 400; i++) {
//     digitalWrite(stepPin, HIGH);
//     delayMicroseconds(3000);   // 3 ms ligado
//     digitalWrite(stepPin, LOW);
//     delayMicroseconds(3000);   // 3 ms desligado
//   }
//   delay(1000);  // pausa de 1s

//   // ---- Gira 400 passos no sentido anti-horário ----
//   digitalWrite(dirPin, HIGH);
//   for (int i = 0; i < 400; i++) {
//     digitalWrite(stepPin, HIGH);
//     delayMicroseconds(3000);
//     digitalWrite(stepPin, LOW);
//     delayMicroseconds(3000);
//   }
//   delay(1000);
// }
