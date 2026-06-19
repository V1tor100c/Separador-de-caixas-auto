#include <HCSR04.h> // Inclui a biblioteca do sensor

// Define os pinos de conexão (Trigger, Echo)
int pino_trigger = 5;
int pino_echo = 6;

// Inicializa o sensor nos pinos definidos
UltraSonicDistanceSensor distanceSensor(pino_trigger, pino_echo);

void setup() {
  Serial.begin(9600); // Inicia o Monitor Serial
}

void loop() {
  // Mede a distância em centímetros
  float distancia = distanceSensor.measureDistanceCm();

  // Exibe o resultado no Monitor Serial
  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.println(" cm");

  delay(500); // Aguarda meio segundo para a próxima leitura
}
