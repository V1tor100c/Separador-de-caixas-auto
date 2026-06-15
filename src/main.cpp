#include <Arduino.h>
#include <ESP32Servo.h>

const int stepPin = 15;
const int dirPin = 4;
const int frequenciaMotor = 20;

void loop0(void * parament);
void loop1(void * parament);

void ligar_esteira();
void desligar_esteira();

void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    loop0,       /* Task function. */
    "Task0",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    NULL,        /* Task handle. */
    0            /* core where the task should run */
  );

  xTaskCreatePinnedToCore(
    loop1,       /* Task function. */
    "Task1",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    NULL,        /* Task handle. */
    1            /* core where the task should run */
  );

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(stepPin, LOW);
  digitalWrite(dirPin, LOW);
  ledcSetup(0, frequenciaMotor, 8);
  ledcAttachPin(stepPin, 0);

}

void loop() {
}

/*
● Leitura dos sensores;
● Detecção das caixas;
● Classificação das dimensões;
● Controle dos manipuladores robóticos;
● Controle dos motores da esteira;
● Tratamento das interrupções;
● Sincronização temporal do sistema.*/

void loop0(void * parament) {
  Serial.println("Task0");
  while(true) {
    ligar_esteira();
    delay(3000);
    desligar_esteira();
    delay(3000);
  }
}

/*
● Servidor Web;
● Aplicativo móvel (quando utilizado);
● Comunicação Wi-Fi;
● Atualização do display OLED;
● Controle do LED RGB;
● Registro de eventos;
● Armazenamento de dados;
● Supervisão geral do sistema.*/

void loop1(void * parament) {
  Serial.println("Task1");
  while(true) {

  } 
}

void ligar_esteira(){
  digitalWrite(dirPin, HIGH);
  ledcWrite(0, 128);
}

void desligar_esteira(){
  digitalWrite(dirPin, LOW);
  ledcWrite(0, 0);
}