#include <Arduino.h>
#include <ESP32Servo.h>
#include <HCSR04.h>
#include <ESP32Servo.h>

const int stepPin = 15;
const int dirPin = 4;
const int frequenciaMotor = 20;

const int pino_trigger = 5;
const int pino_echo = 2;
UltraSonicDistanceSensor distanceSensor(pino_trigger, pino_echo);

const int iri = 14;

const int bracoI[] = {30, 31, 32, 33};

const int botaoEmergencia = 12;

int medidaSemCaixa = 15;
int medidaCaixaG = 12;
int medidaCaixaM = 10;
int medidaCaixaP = 8;

int quantidadeCaixasP = 0;
int quantidadeCaixasM = 0;
int quantidadeCaixasG = 0;
bool viACaixa = false;

bool bracoOperando = false;
bool caixaNaEsteira = false;

bool estadoEmergencia = false;

void loop0(void *parament);
void loop1(void *parament);

void ligar_esteira();
void desligar_esteira();

bool leituraIR();

float medirDistancia();
void sensorDeCaixa();

void pegarCaixa();

void emergencia();

void zeraTudo();

void setup()
{
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
      loop0,   /* Task function. */
      "Task0", /* name of task. */
      10000,   /* Stack size of task */
      NULL,    /* parameter of the task */
      1,       /* priority of the task */
      NULL,    /* Task handle. */
      0        /* core where the task should run */
  );

  xTaskCreatePinnedToCore(
      loop1,   /* Task function. */
      "Task1", /* name of task. */
      10000,   /* Stack size of task */
      NULL,    /* parameter of the task */
      1,       /* priority of the task */
      NULL,    /* Task handle. */
      1        /* core where the task should run */
  );

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(stepPin, LOW);
  digitalWrite(dirPin, LOW);
  ledcSetup(0, frequenciaMotor, 8);
  ledcAttachPin(stepPin, 0);

  pinMode(iri, INPUT);

  for(int i = 0; i < 4; i++){
    pinMode(bracoI[i], OUTPUT);
  }

  pinMode(botaoEmergencia, INPUT);
}

void loop()
{
}

/*
● Leitura dos sensores;
● Detecção das caixas;
● Classificação das dimensões;
● Controle dos manipuladores robóticos;
● Controle dos motores da esteira;
● Tratamento das interrupções;
● Sincronização temporal do sistema.*/

void loop0(void *parameter){
  Serial.println("Task0");
  while (true){
    if (digitalRead(estadoEmergencia) == HIGH){
      emergencia();
      estadoEmergencia = true;
      while (botaoEmergencia == true){
        if (digitalRead(estadoEmergencia) == HIGH){
          estadoEmergencia = false;
        }
      }
    } 
    if (leituraIR() == true)
    {
      desligar_esteira();
    }
    else
    {
      ligar_esteira();
    }
    sensorDeCaixa();
    zeraTudo();
    vTaskDelay(pdMS_TO_TICKS(10));
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

void loop1(void *parameter)
{
  Serial.println("Task1");
  while (true)
  {

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void ligar_esteira()
{
  digitalWrite(dirPin, HIGH);
  ledcWrite(0, 128);
}

void desligar_esteira()
{
  digitalWrite(dirPin, LOW);
  ledcWrite(0, 0);
}

bool leituraIR()
{
  int estadoIR = digitalRead(iri);
  if (estadoIR == LOW)
  {
    return true;
  }
  else
  {
    return false;
  }
}

float medirDistancia()
{
  return distanceSensor.measureDistanceCm();
}

void sensorDeCaixa()
{
  if (viACaixa == false)
  {
    viACaixa = true;
    float tamanho = medirDistancia();
    if (tamanho < medidaCaixaP)
    {
      quantidadeCaixasP++;
    }
    else if (tamanho < medidaCaixaM)
    {
      quantidadeCaixasM++;
    }
    else if (tamanho < medidaCaixaG)
    {
      quantidadeCaixasG++;
    }
    else
    {
    }
  }
}

void pegarCaixa(){

}

void emergencia()
{
  desligar_esteira();
}

void zeraTudo()
{
  bracoOperando = false;
  caixaNaEsteira = false;
  viACaixa = false;
}