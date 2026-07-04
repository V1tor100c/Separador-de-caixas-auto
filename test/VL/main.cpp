#include "Adafruit_VL53L0X.h"

// Cria o objeto do sensor
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void setup() {
  Serial.begin(115200);
  
  Serial.println("=== Teste Sensor VL53L0X ===");
  
  // Inicializa o sensor
  if (!lox.begin()) {
    Serial.println("ERRO: Falha ao inicializar o VL53L0X!");
    Serial.println("Verifique as conexões (VIN, GND, SDA, SCL).");
    while (1);  // Para o programa aqui se falhar
  }
  
  Serial.println("Sensor VL53L0X inicializado com sucesso!");
  Serial.println("Aguardando leituras...\n");
}

void loop() {
  // Estrutura para armazenar os dados de medição
  VL53L0X_RangingMeasurementData_t measure;
  
  // Faz a leitura da distância
  // O segundo parâmetro 'false' desativa mensagens de debug
  lox.rangingTest(&measure, false);
  
  // Verifica se a leitura é válida (RangeStatus != 4 indica erro)
  if (measure.RangeStatus != 4) {
    // Exibe a distância em milímetros
    Serial.print("Distância: ");
    Serial.print(measure.RangeMilliMeter);
    Serial.println(" mm");
    
    // Opcional: exibe também em centímetros
    Serial.print("          ");
    Serial.print(measure.RangeMilliMeter / 10.0);
    Serial.println(" cm");
  } else {
    // Se o status for 4, significa que o objeto está fora do alcance
    Serial.println("Sem objeto detectado (fora do alcance)");
  }
  
  // Pequeno delay entre as leituras
  delay(500);
}