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
bool estadoIR = false;

const int bracoI[] = {30, 31, 32, 33};
Servo braco11, braco12, braco13, braco14;
Servo braco21, braco22, braco23, braco24;

const int botaoIniciar = 13;
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

volatile bool estadoEmergencia = false;

char tamanhoCaixaMedida = 'X';

// Definição dos Estados do Sistema (Máquina de Estados)
enum EstadosSistema {
  AGUARDANDO_START,
  MANIPULADOR1_PEGA_CAIXA,
  ESTEIRA_TRANSPORTANDO,
  AGUARDANDO_FIM_ESTEIRA,
  MANIPULADOR2_SEPARA_CAIXA,
  RESETANDO_MAQUINA,
  EM_EMERGENCIA
};
EstadosSistema estadoAtual = AGUARDANDO_START;

// Prototipação das funções
void loop0(void *parament);
void loop1(void *parament);

void ligar_esteira();
void desligar_esteira();

bool leituraIR();

float medirDistancia();
void sensorDeCaixa();

void pegarCaixa();

void colocarCaixaP();
void colocarCaixaM();
void colocarCaixaG();

void IRAM_ATTR emergencia();

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

  braco11.attach(30, 500, 2400);
  braco12.attach(31, 500, 2400);
  braco13.attach(32, 500, 2400);
  braco14.attach(33, 500, 2400);

  braco21.attach(34, 500, 2400);
  braco22.attach(35, 500, 2400);
  braco23.attach(36, 500, 2400);
  braco24.attach(37, 500, 2400);

  pinMode(botaoIniciar, INPUT);
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
  while (true) {
    
    // Se a interrupção detectou emergência, força o estado de erro
    if (estadoEmergencia) {
      estadoAtual = EM_EMERGENCIA;
    }

    switch (estadoAtual) {
      
      case AGUARDANDO_START:
        if (digitalRead(botaoIniciar) == HIGH){
          estadoAtual = MANIPULADOR1_PEGA_CAIXA;
          while(digitalRead(botaoIniciar) == HIGH) { delay(10); }
        }

      case MANIPULADOR1_PEGA_CAIXA:
        Serial.println("[Status] Manipulador 1 Movendo...");
        pegarCaixa();
        estadoAtual = ESTEIRA_TRANSPORTANDO;
        break;

      case ESTEIRA_TRANSPORTANDO:
        Serial.println("[Status] Ligando Esteira e Medindo Caixa...");
        ligar_esteira();
        // Faz a leitura física do ultrassônico e classifica
        sensorDeCaixa(); 
        if(tamanhoCaixaMedida != 'X'){
          estadoAtual = AGUARDANDO_FIM_ESTEIRA;
        }
        break;

      case AGUARDANDO_FIM_ESTEIRA:
        // Continua com a esteira ligada até a caixa bater no segundo sensor
        estadoIR = leituraIR();
        if (estadoIR == LOW) {
          desligar_esteira();
          estadoAtual = MANIPULADOR2_SEPARA_CAIXA;
        }
        break;

      case MANIPULADOR2_SEPARA_CAIXA:
        Serial.print("[Status] Separando caixa do tipo: ");
        Serial.println(tamanhoCaixaMedida);
        
        if(leituraIR() == true){
          if(tamanhoCaixaMedida = 'P'){
            colocarCaixaP();
          }
          else if(tamanhoCaixaMedida = 'M'){
            colocarCaixaM();
          }
          else{
            colocarCaixaG();
          }
          estadoAtual = RESETANDO_MAQUINA;
        }
        break;

      case RESETANDO_MAQUINA:
        zeraTudo();
        estadoAtual = AGUARDANDO_START;
        break;

      case EM_EMERGENCIA:
        desligar_esteira();
        Serial.println("!!! SISTEMA EM PARADA DE EMERGENCIA !!!");
        // Só sai do loop de emergência se o botão for destravado/solto
        if (digitalRead(botaoEmergencia) == HIGH) { 
          estadoEmergencia = false;
          estadoAtual = AGUARDANDO_START;
          Serial.println("-> Sistema Reiniciado com Sucesso.");
        }
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Delay do FreeRTOS para não travar o processador
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
      tamanhoCaixaMedida = 'P';
    }
    else if (tamanho < medidaCaixaM)
    {
      quantidadeCaixasM++;
      tamanhoCaixaMedida = 'M';
    }
    else if (tamanho < medidaCaixaG)
    {
      quantidadeCaixasG++;
      tamanhoCaixaMedida = 'G';
    }
    else
    {
    }
  }
}

void pegarCaixa(){
  braco11.write(45);
  vTaskDelay(pdMS_TO_TICKS(30));
  braco12.write(45);
  braco13.write(45);
  vTaskDelay(pdMS_TO_TICKS(30));
  braco14.write(45);
}

void colocarCaixaP(){
  braco21.write(45);
  vTaskDelay(pdMS_TO_TICKS(30));
  braco22.write(45);
  braco23.write(45);
  vTaskDelay(pdMS_TO_TICKS(30));
  braco24.write(15);
}
void colocarCaixaM(){
  braco21.write(45);
  vTaskDelay(pdMS_TO_TICKS(30));
  braco22.write(45);
  braco23.write(45);
  vTaskDelay(pdMS_TO_TICKS(30));
  braco24.write(45);
}
void colocarCaixaG(){
  braco21.write(45);
  vTaskDelay(pdMS_TO_TICKS(30));
  braco22.write(45);
  braco23.write(45);
  vTaskDelay(pdMS_TO_TICKS(30));
  braco24.write(75);
}

void emergencia(){
  desligar_esteira();
}

void zeraTudo(){
  bracoOperando = false;
  caixaNaEsteira = false;
  viACaixa = false;

  braco11.write(0);
  braco12.write(0);
  braco13.write(0);
  braco14.write(0);

  braco21.write(0);
  braco22.write(0);
  braco23.write(0);
  braco24.write(0);

  tamanhoCaixaMedida = 'X';
}