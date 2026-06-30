#include <Arduino.h>

// Definição dos pinos que você informou
const int trigPin = 19;
const int echoPin = 13;

// Variáveis para armazenar a duração do pulso e a distância calculada
long duration;
float distanceCm;

void setup() {
  // Inicia a comunicação serial a 115200 baud (padrão do ESP)
  Serial.begin(115200); 
  
  // Configura a direção dos pinos
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Um pequeno delay para dar tempo do Monitor Serial abrir
  delay(1000); 
  Serial.println("Iniciando teste do sensor ultrassônico...");
}

void loop() {
  // 1. Garante que o pino trigger está em nível baixo
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // 2. Aciona o trigger por 10 microssegundos para emitir o som
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // 3. Lê o tempo (em microssegundos) que o sinal levou para ir e voltar
  duration = pulseIn(echoPin, HIGH);
  
  // 4. Calcula a distância em centímetros
  // A velocidade do som é ~340 m/s ou 0.034 cm/µs.
  // Dividimos por 2 porque o tempo lido é de ida e volta.
  distanceCm = duration * 0.034 / 2.0;
  
  // 5. Exibe os valores no Monitor Serial
  Serial.print("Distância: ");
  Serial.print(distanceCm);
  Serial.println(" cm");
  
  // Aguarda 1 segundo antes da próxima leitura
  delay(1000);
}





// #include <Arduino.h>
// #include <HCSR04.h>

// const int pino_trigger = 25;
// const int pino_echo = 26;

// UltraSonicDistanceSensor distanceSensor(pino_trigger, pino_echo);

// float distancia = 0;

// float medirDistancia() { 
//   return distanceSensor.measureDistanceCm(); 
// }

// void setup() {
//   Serial.begin(115200);
// }

// void loop() {
//   distancia = medirDistancia();
//   Serial.print("Distancia: ");
//   Serial.print(distancia);
//   Serial.println(" cm");
  
//   delay(500);
// }