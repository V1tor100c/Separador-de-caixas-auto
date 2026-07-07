#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include <HCSR04.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Adafruit_VL53L0X.h"

const char* ssid = "arthu";
const char* password = "12341234";

// const char* ssid = "POCO F7";
// const char* password = "macrocontrole";

WebServer server(80);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN  110
#define SERVOMAX  410 

uint8_t ServoGarra = 3; 
uint8_t ServoDireito = 0; 
uint8_t ServoEsquerdo = 1;
uint8_t ServoBase = 2;

uint8_t ultAngGarra = 180;
uint8_t ultAngDireito = 180;
uint8_t ultAngEsquerda = 90;
uint8_t ultAngBase = 100;

const int stepPin = 16;
const int dirPin = 4;
const int enablePin = 15;
const int frequenciaMotor = 70;

const int iri = 14;
bool estadoIR = false;

const int trigPinMed = 2;
const int echoPinMed = 13;

const int trigPinFim = 17;   //17
const int echoPinFim = 12;  //12

const int botaoIniciar = 33;
const int botaoEmergencia = 34;

const int buzzer = 23;
const int LM35 = 36;

const int pinLedR = 19;
const int pinLedG = 18;
const int pinLedB = 5;
const int transistor = 26;

int quantidadeCaixasP = 0;
int quantidadeCaixasM = 0;
int quantidadeCaixasG = 0;
char tamanhoCaixaMedida = '-';
volatile float temperaturaAtual = 0.0;

// CORREÇÃO: Separando as variáveis de emergência
volatile bool estadoEmergencia = false;       // Ativada pelos botões
volatile bool emergenciaTemperatura = false;  // Ativada pela temperatura alta

volatile unsigned long tempoUltimoClique = 0;

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

float medidaSemCaixa = 8;
float medidaCaixaP = 5.2;
float medidaCaixaM = 3.9;
float medidaCaixaG = 2.5;
bool viACaixa = false;
char tamanho;
char tamanhoAtual;

unsigned long tempoCaixaAnterior = 0;
unsigned long tempoCaixaAtual = 0;
const long intervaloUmSegundo = 1700; 
int segundosPassados = 0;

volatile bool flagMedirTemperatura = false;

SemaphoreHandle_t mutexDados;
SemaphoreHandle_t mutexI2C; 
hw_timer_t *timerTemperatura = NULL;

void loop0(void *parameter);
void loop1(void *parameter);
void ligar_esteira();
void voltar_esteira();
void desligar_esteira();
bool leituraFim();
float medirDistancia();
void sensorDeCaixa();
void zeraTudo();
void setCorRGB(int r, int g, int b);
void enviarPaginaWeb();
void enviarDadosJSON();
void tratarBotaoVirtualEmergencia();
void tratarBotaoVirtualIniciar();
int converterAnguloParaPulso(int angulo);
void moveServoG(int ang);
void moveServoD(int ang);
void moveServoE(int ang);
void moveServoB(int ang);
void levanteDE();
void pegarCaixa();
// void devolverCaixa();
void levarEsteira();
void servoInit();
void colocarCaixaP();
void colocarCaixaM();
void colocarCaixaG();
String obterNomeEstado(EstadosSistema estado);

void IRAM_ATTR emergencia() {
  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoUltimoClique > 300) {
    estadoEmergencia = !estadoEmergencia;
    tempoUltimoClique = tempoAtual;
  }
}

void IRAM_ATTR onTimerTemperatura() {
  flagMedirTemperatura = true;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  mutexDados = xSemaphoreCreateMutex();
  mutexI2C = xSemaphoreCreateMutex(); 

  pinMode(pinLedR, OUTPUT);
  pinMode(pinLedG, OUTPUT);
  pinMode(pinLedB, OUTPUT);
  pinMode(transistor, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(LM35, INPUT);

  digitalWrite(transistor, HIGH);
  digitalWrite(buzzer, LOW);
  setCorRGB(0, 0, 255);

  Serial.print("Conectando ao Wi-Fi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado!");

  server.on("/", enviarPaginaWeb);    
  server.on("/dados", enviarDadosJSON);
  server.on("/emergencia_virtual", tratarBotaoVirtualEmergencia);
  server.on("/iniciar_virtual", tratarBotaoVirtualIniciar);
  server.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("ERRO CRITICO: SSD1306 nao encontrado!"));
  } else {
    Serial.println("OLED encontrado!");
    display.clearDisplay();
    display.display();
  }

  pinMode(botaoIniciar, INPUT);
  pinMode(botaoEmergencia, INPUT);
  attachInterrupt(digitalPinToInterrupt(botaoEmergencia), emergencia, RISING);

  timerTemperatura = timerBegin(0, 80, true);
  timerAttachInterrupt(timerTemperatura, &onTimerTemperatura, true);
  timerAlarmWrite(timerTemperatura, 1000000, true);
  timerAlarmEnable(timerTemperatura);

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  digitalWrite(stepPin, LOW);
  digitalWrite(dirPin, LOW);
  digitalWrite(enablePin, LOW);
  ledcSetup(0, frequenciaMotor, 8);
  ledcAttachPin(stepPin, 0);
  
  pinMode(trigPinMed, OUTPUT);
  pinMode(echoPinMed, INPUT);
  pinMode(trigPinFim, OUTPUT);
  pinMode(echoPinFim, INPUT);

  // if (!lox.begin()) {
  //   Serial.println("ERRO: Falha ao inicializar o VL53L0X!");
  //   Serial.println("Verifique as conexões (VIN, GND, SDA, SCL).");
  //   while (1);
  // }

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);

  xTaskCreatePinnedToCore(loop0, "Task0", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(loop1, "Task1", 10000, NULL, 1, NULL, 1);
}

void loop() {}

void loop0(void *parameter) {
  Serial.println("CORE 0 INICIADO!");
  while (true) {
    // pegarCaixa();
    // levarEsteira();
    // colocarCaixaM();
    // while(1){
    //   delay(1000);
    // }

    if (flagMedirTemperatura) {
      flagMedirTemperatura = false;
      int lecturaADC = analogRead(LM35);
      float temp = ((lecturaADC / 4095.0) * 3300.0) / 10.0;
     
      xSemaphoreTake(mutexDados, portMAX_DELAY);
      temperaturaAtual = temp;
      if (temperaturaAtual > 80.0) estadoEmergencia = true;
      xSemaphoreGive(mutexDados);
    }

    xSemaphoreTake(mutexDados, portMAX_DELAY);
    bool emEmergencia = estadoEmergencia;
    float tempCheck = temperaturaAtual;
    xSemaphoreGive(mutexDados);

    if (emEmergencia && estadoAtual != EM_EMERGENCIA) {
      Serial.println("ENTRANDO EM EMERGENCIA!");
      xSemaphoreTake(mutexDados, portMAX_DELAY);
      estadoAtual = EM_EMERGENCIA;
      xSemaphoreGive(mutexDados);
    }
    else if (!emEmergencia && estadoAtual == EM_EMERGENCIA) {
      if (tempCheck < 20.0) {
        Serial.println("SAINDO DA EMERGENCIA!");
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        estadoAtual = AGUARDANDO_START;
        xSemaphoreGive(mutexDados);
        digitalWrite(buzzer, LOW);
      } else {
        Serial.println("MUITO QUENTE! Manter na emergencia.");
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        estadoEmergencia = true;
        xSemaphoreGive(mutexDados);
      }
    }

    switch (estadoAtual) {
      case AGUARDANDO_START:
        digitalWrite(buzzer, LOW);
        desligar_esteira();
        servoInit();
        if (digitalRead(botaoIniciar) == HIGH) {
          xSemaphoreTake(mutexDados, portMAX_DELAY);
          estadoAtual = MANIPULADOR1_PEGA_CAIXA;
          xSemaphoreGive(mutexDados);
          while(digitalRead(botaoIniciar) == HIGH) { vTaskDelay(pdMS_TO_TICKS(10)); }
        }
        break;

      case MANIPULADOR1_PEGA_CAIXA:

        Serial.println("MANIPULADOR 1 PEGA CAIXA!");
        pegarCaixa();
        levarEsteira();
        tempoCaixaAnterior = millis();
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        estadoAtual = ESTEIRA_TRANSPORTANDO;
        xSemaphoreGive(mutexDados);
        break;

        case ESTEIRA_TRANSPORTANDO:
        Serial.println("ESTEIRA TRANSPORTANDO!"); 
        ligar_esteira(); // Mantém a esteira ligada rodando em background
        
        if (millis() - tempoCaixaAnterior < intervaloUmSegundo) {
          break; 
        }

        desligar_esteira();
        sensorDeCaixa();
          for(int i = 0; i < 5; i++){
            medirDistancia();
            delay(100);
          }
          sensorDeCaixa();
       
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        tamanhoAtual = tamanhoCaixaMedida;
        xSemaphoreGive(mutexDados);

        if (tamanhoAtual != '-') {
          xSemaphoreTake(mutexDados, portMAX_DELAY);
          estadoAtual = AGUARDANDO_FIM_ESTEIRA;
          xSemaphoreGive(mutexDados);
        }
        tempoCaixaAnterior = millis();
        break;

      case AGUARDANDO_FIM_ESTEIRA:

        Serial.println("AGUARDANDO FIM ESTEIRA!");
        voltar_esteira();

        if (millis() - tempoCaixaAnterior < intervaloUmSegundo) {
          break; 
        }

          desligar_esteira();
          xSemaphoreTake(mutexDados, portMAX_DELAY);
          estadoAtual = MANIPULADOR2_SEPARA_CAIXA;
          xSemaphoreGive(mutexDados);
        break;

      case MANIPULADOR2_SEPARA_CAIXA:

        Serial.println("MANIPULADOR 2 SEPARA CAIXA!");
        desligar_esteira();
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        tamanho = tamanhoCaixaMedida;
        xSemaphoreGive(mutexDados);

        if (tamanho == 'P') colocarCaixaP();
        else if (tamanho == 'M') colocarCaixaM();
        else if (tamanho == 'G') colocarCaixaG();
       
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        estadoAtual = RESETANDO_MAQUINA;
        xSemaphoreGive(mutexDados);
        break;

      case RESETANDO_MAQUINA:

        Serial.println("RESETANDO MAQUINA!");
        zeraTudo();
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        estadoAtual = AGUARDANDO_START;
        xSemaphoreGive(mutexDados);
        break;

      case EM_EMERGENCIA:

        desligar_esteira();
        digitalWrite(buzzer, HIGH);
        break;
    }
   
    vTaskDelay(pdMS_TO_TICKS(50));

  }
}

void loop1(void *parameter) {
  Serial.println("CORE 1 INICIADO!");
  while (true) {
    server.handleClient();

    xSemaphoreTake(mutexDados, portMAX_DELAY);
    EstadosSistema estadoLocal = estadoAtual;
    int pLocal = quantidadeCaixasP;
    int mLocal = quantidadeCaixasM;
    int gLocal = quantidadeCaixasG;
    char ultimaLocal = tamanhoCaixaMedida;
    float tempLocal = temperaturaAtual;
    xSemaphoreGive(mutexDados);

    if (estadoLocal == EM_EMERGENCIA) {
      setCorRGB(255, 0, 0);    
    } else if (estadoLocal == AGUARDANDO_START) {
      setCorRGB(0, 0, 255);    
    } else if (estadoLocal == ESTEIRA_TRANSPORTANDO || estadoLocal == MANIPULADOR1_PEGA_CAIXA || estadoLocal == MANIPULADOR2_SEPARA_CAIXA) {
      setCorRGB(255, 255, 0);  
    } else {
      setCorRGB(0, 255, 0);    
    }

    // CORREÇÃO: Protegendo a atualização do Display I2C contra concorrência
    xSemaphoreTake(mutexI2C, portMAX_DELAY);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0); display.println("UTFPR - MECHATRONICS");
    display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
    display.setCursor(0, 13); display.print("IP: "); display.println(WiFi.localIP().toString());
    display.setCursor(0, 24); display.print("Stt: "); display.println(obterNomeEstado(estadoLocal));
    display.setCursor(0, 35); display.print("Temp: "); display.print(tempLocal, 1); display.println(" C");
    display.setCursor(0, 45); display.print("Ultima: "); display.println(ultimaLocal);
    display.setCursor(0, 55);
    display.print("T:"); display.print(pLocal + mLocal + gLocal);
    display.print("    P:"); display.print(pLocal);
    display.print(" M:"); display.print(mLocal);
    display.print(" G:"); display.print(gLocal);

    display.display();
    xSemaphoreGive(mutexI2C);
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void ligar_esteira() {
  digitalWrite(dirPin, HIGH);
  digitalWrite(enablePin, LOW);
  ledcWrite(0, 128);
}

void voltar_esteira() {
  digitalWrite(dirPin, LOW);
  digitalWrite(enablePin, LOW);
  ledcWrite(0, 128);
}

void desligar_esteira() {
  digitalWrite(dirPin, HIGH);
  digitalWrite(enablePin, HIGH);
  ledcWrite(0, 0);
}

bool lecturaFim() {
  digitalWrite(trigPinFim, LOW);
  delayMicroseconds(2);
 
  digitalWrite(trigPinFim, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinFim, LOW);
 
  float duration = pulseIn(echoPinFim, HIGH, 30000);
  if (duration == 0) return false;

  float distanceCm = duration * 0.034 / 2.0;

  if (distanceCm > 0 && distanceCm < 5){
    return true;
  }
  else{
    return false;
  }
}


float medirDistancia() {
  digitalWrite(trigPinMed, LOW);
  delayMicroseconds(2);
 
  digitalWrite(trigPinMed, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinMed, LOW);
 
  long duration = pulseIn(echoPinMed, HIGH, 30000);
 
  float distanceCm = duration * 0.034 / 2.0;
  return distanceCm;
}

// float medirDistancia() {
//   digitalWrite(trigPinMed, LOW);
//   delayMicroseconds(2);
 
//   digitalWrite(trigPinMed, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(trigPinMed, LOW);
 
//   long duration = pulseIn(echoPinMed, HIGH, 30000);
 
//   if (duration == 0) {
//     return 999.0;
//   }
 
//   float distanceCm = duration * 0.034 / 2.0;
//   return distanceCm;
// }

void sensorDeCaixa() {
  if (viACaixa) return;

  float distancia = medirDistancia();

  if (distancia == -1 || distancia <= 1.0) {
    return;
  }

  if (distancia >= (medidaSemCaixa - 0.5)) {
    return;
  }

  Serial.println("Quina da caixa detectada. Aguardando alinhamento...");
 
  vTaskDelay(pdMS_TO_TICKS(350));

  distancia = medirDistancia();

  if (distancia == -1 || distancia >= (medidaSemCaixa - 0.5)) {
    Serial.println("Erro: A caixa escapou da leitura ou foi alarme falso.");
    return;
  }

  char tipo = '-';
  if (distancia < medidaCaixaG) {        
    tipo = 'G';
    Serial.print("CAIXA GRANDE: ");
  } else if (distancia < medidaCaixaM) {
    tipo = 'M';
    Serial.print("CAIXA MÉDIA: ");
  } else if (distancia < medidaCaixaP) {
    tipo = 'P';
    Serial.print("CAIXA PEQUENA: ");  
  } else {
    return;
  }

  Serial.println(distancia);

  xSemaphoreTake(mutexDados, portMAX_DELAY);
  tamanhoCaixaMedida = tipo;
  if (tipo == 'P') {
    quantidadeCaixasP++;
  } else if (tipo == 'M') {
    quantidadeCaixasM++;
  } else if (tipo == 'G') {
    quantidadeCaixasG++;
  }
  xSemaphoreGive(mutexDados);

  viACaixa = true;

  Serial.print("Caixa detectada e salva: ");
  Serial.println(tipo);
}

int converterAnguloParaPulso(int angulo) {
  return map(angulo, 0, 180, SERVOMIN, SERVOMAX);
}

void moveServoG(int ang){
  if(ang > ultAngGarra){
    for(int angulo = ultAngGarra; angulo <= ang; angulo++) {
      // CORREÇÃO: Protegendo a escrita I2C no driver de servo
      xSemaphoreTake(mutexI2C, portMAX_DELAY);
      pwm.setPWM(ServoGarra, 0, converterAnguloParaPulso(angulo));
      xSemaphoreGive(mutexI2C);
      Serial.println(angulo);
      delay(20);
      ultAngGarra = angulo;
    }
  }
  else if(ang < ultAngGarra){
    for(int angulo = ultAngGarra; angulo >= ang; angulo--) {
      // CORREÇÃO: Protegendo a escrita I2C no driver de servo
      xSemaphoreTake(mutexI2C, portMAX_DELAY);
      pwm.setPWM(ServoGarra, 0, converterAnguloParaPulso(angulo));
      xSemaphoreGive(mutexI2C);
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
      // CORREÇÃO: Protegendo a escrita I2C no driver de servo
      xSemaphoreTake(mutexI2C, portMAX_DELAY);
      pwm.setPWM(ServoDireito, 0, converterAnguloParaPulso(angulo));
      xSemaphoreGive(mutexI2C);
      Serial.println(angulo);
      delay(20);
      ultAngDireito = angulo;
    }
  }
  else if(ang < ultAngDireito){
    for(int angulo = ultAngDireito; angulo >= ang; angulo--) {
      // CORREÇÃO: Protegendo a escrita I2C no driver de servo
      xSemaphoreTake(mutexI2C, portMAX_DELAY);
      pwm.setPWM(ServoDireito, 0, converterAnguloParaPulso(angulo));
      xSemaphoreGive(mutexI2C);
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
  if(ang > ultAngEsquerda){
    for(int angulo = ultAngEsquerda; angulo <= ang; angulo++) {
      // CORREÇÃO: Protegendo a escrita I2C no driver de servo
      xSemaphoreTake(mutexI2C, portMAX_DELAY);
      pwm.setPWM(ServoEsquerdo, 0, converterAnguloParaPulso(angulo));
      xSemaphoreGive(mutexI2C);
      Serial.println(angulo);
      delay(20);
      ultAngEsquerda = angulo;
    }
  }
  else if(ang < ultAngEsquerda){
    for(int angulo = ultAngEsquerda; angulo >= ang; angulo--) {
      // CORREÇÃO: Protegendo a escrita I2C no driver de servo
      xSemaphoreTake(mutexI2C, portMAX_DELAY);
      pwm.setPWM(ServoEsquerdo, 0, converterAnguloParaPulso(angulo));
      xSemaphoreGive(mutexI2C);
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
      // CORREÇÃO: Protegendo a escrita I2C no driver de servo
      xSemaphoreTake(mutexI2C, portMAX_DELAY);
      pwm.setPWM(ServoBase, 0, converterAnguloParaPulso(angulo));
      xSemaphoreGive(mutexI2C);
      Serial.println(angulo);
      delay(20);
      ultAngBase = angulo;
    }
  }
  else if(ang < ultAngBase){
    for(int angulo = ultAngBase; angulo >= ang; angulo--) {
      // CORREÇÃO: Protegendo a escrita I2C no driver de servo
      xSemaphoreTake(mutexI2C, portMAX_DELAY);
      pwm.setPWM(ServoBase, 0, converterAnguloParaPulso(angulo));
      xSemaphoreGive(mutexI2C);
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
      // CORREÇÃO: Protegendo a escrita I2C no driver de servo
      xSemaphoreTake(mutexI2C, portMAX_DELAY);
      pwm.setPWM(ServoEsquerdo, 0, converterAnguloParaPulso(angulo));
      pwm.setPWM(ServoDireito, 0, converterAnguloParaPulso(angulod));
      xSemaphoreGive(mutexI2C);
      Serial.print("levata: ");
      Serial.println(angulo);
      angulod -= 3;
      delay(20);
      ultAngEsquerda = angulo;
      ultAngDireito = angulod;
    }
}

void servoInit(){
  moveServoB(30);
  moveServoG(160);
  moveServoE(60);
  moveServoD(150);
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
  moveServoB(150);
  moveServoD(140);
  moveServoE(150);
  moveServoG(180);
  // moveServoD(100);
}

void colocarCaixaP() {
  moveServoE(147);
  moveServoG(205);
  moveServoD(70);
  moveServoB(30);
  moveServoD(150);
  // moveServoE(95);
  moveServoG(160);
}

void colocarCaixaM() {
  moveServoE(147);
  moveServoG(205);
  moveServoD(70);
  moveServoB(100);
  moveServoD(150);
  // moveServoE(95);
  moveServoG(160);
}

void colocarCaixaG() {
  moveServoE(147);
  moveServoG(205);
  moveServoD(70);
  moveServoB(220);
  moveServoD(150);
  // moveServoE(95);
  moveServoG(160);
}

bool leituraFim() {
  digitalWrite(trigPinFim, LOW);
  delayMicroseconds(2);
 
  digitalWrite(trigPinFim, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPinFim, LOW);
 
  float duration = pulseIn(echoPinFim, HIGH, 30000);
  if (duration == 0) return false;

  float distanceCm = duration * 0.034 / 2.0;

  if (distanceCm > 0 && distanceCm < 5){
    return true;
  }
  else{
    return false;
  }
}

void zeraTudo() {
  tempoCaixaAtual = 0;
  tempoCaixaAnterior = 0;
  viACaixa = false;
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  tamanhoCaixaMedida = '-';
  xSemaphoreGive(mutexDados);
  servoInit();
}

void setCorRGB(int r, int g, int b) {
  digitalWrite(transistor, HIGH);
  if(r == 0){
    digitalWrite(pinLedR, LOW);
  } else {
    digitalWrite(pinLedR, HIGH);
  }
  if(g == 0){
    digitalWrite(pinLedG, LOW);
  } else {
    digitalWrite(pinLedG, HIGH);
  }
  if(b == 0){
    digitalWrite(pinLedB, LOW);
  } else {
    digitalWrite(pinLedB, HIGH);
  }
}

void enviarPaginaWeb() {
  String html = R"rawliteral(
<!DOCTYPE html><html>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>Painel Esteira UTFPR</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body{font-family:Arial, sans-serif; text-align:center; background:#eef2f5; margin:0; padding:20px;}
    .card{background:white; padding:20px; margin:10px auto; max-width:480px; border-radius:10px; box-shadow:0 5px 15px rgba(0,0,0,0.1);}
    h1{color:#2c3e50;} .status{font-weight:bold; color:blue;}
    .btn{padding:15px 25px; font-size:16px; margin:10px; border:none; border-radius:5px; cursor:pointer; font-weight:bold; width: 45%; transition: 0.2s;}
    .btn-danger{background-color:#e74c3c; color:white;} .btn-danger:hover{background-color:#c0392b;}
    .btn-success{background-color:#2ecc71; color:white;} .btn-success:hover{background-color:#27ae60;}
    .danger-alert{background-color:#ffcccc; color:#cc0000; border:2px solid #cc0000; padding:15px; border-radius:5px; margin-bottom:15px; font-weight:bold; text-transform: uppercase; animation: blink 1s infinite;}
    .stats-container { display: flex; justify-content: space-between; margin: 15px 0;}
    .stat-box { background: #f8f9fa; border-left: 4px solid #3498db; padding: 10px; width: 30%; border-radius: 5px; box-sizing: border-box;}
    .stat-box.m { border-left-color: #f1c40f; }
    .stat-box.g { border-left-color: #e67e22; }
    .stat-box span { display: block; font-size: 24px; font-weight: bold; color: #333;}
    .stat-box small { color: #7f8c8d; font-size: 14px;}
    @keyframes blink { 50% { opacity: 0.5; } }
  </style>
</head>
<body>
  <h1>UTFPR - Sistema de Separação</h1>
  <div id='alerta' class='card danger-alert' style='display:none;'>⚠ ALERTA DE EMERGÊNCIA ⚠</div>
  <div class='card'>
    <h2>Status: <span id='status' style='color:#3498db;'>Carregando...</span></h2>
    <h3>Temperatura do Motor: <span id='temp'>0.0</span> °C</h3>
  </div>
  <div class='card'>
    <h3 style="margin-top: 0;">Estatísticas de Produção</h3>
    <p style="font-size: 18px;">Total Processado: <b id='total'>0</b> caixas</p>
    <div class="stats-container">
      <div class="stat-box">Pequena<span id='qtdP'>0</span><small id='percP'>0%</small></div>
      <div class="stat-box m">Média<span id='qtdM'>0</span><small id='percM'>0%</small></div>
      <div class="stat-box g">Grande<span id='qtdG'>0</span><small id='percG'>0%</small></div>
    </div>
    <div style="position: relative; height:200px; width:100%;">
      <canvas id="graficoProducao"></canvas>
    </div>
  </div>
  <div class='card'>
    <button class='btn btn-success' onclick='dispararIniciarVirtual()'>INICIAR</button>
    <button class='btn btn-danger' onclick='dispararEmergenciaVirtual()'>EMERGENCIA</button>
  </div>
  <script>
    const ctx = document.getElementById('graficoProducao').getContext('2d');
    const grafico = new Chart(ctx, {
        type: 'doughnut',
        data: {
            labels: ['Pequena', 'Média', 'Grande'],
            datasets: [{
                data: [0, 0, 0],
                backgroundColor: ['#3498db', '#f1c40f', '#e67e22'],
                borderWidth: 1
            }]
        },
        options: { responsive: true, maintainAspectRatio: false }
    });
    setInterval(function() {
      fetch('/dados').then(response => response.json()).then(data => {
        document.getElementById('status').innerText = data.estado;
        document.getElementById('temp').innerText = data.temperatura.toFixed(1);
        let total = data.p + data.m + data.g;
        document.getElementById('total').innerText = total;
        let percP = total > 0 ? ((data.p / total) * 100).toFixed(1) : 0;
        let percM = total > 0 ? ((data.m / total) * 100).toFixed(1) : 0;
        let percG = total > 0 ? ((data.g / total) * 100).toFixed(1) : 0;
        document.getElementById('qtdP').innerText = data.p;
        document.getElementById('percP').innerText = percP + "%";
        document.getElementById('qtdM').innerText = data.m;
        document.getElementById('percM').innerText = percM + "%";
        document.getElementById('qtdG').innerText = data.g;
        document.getElementById('percG').innerText = percG + "%";
        grafico.data.datasets[0].data = [data.p, data.m, data.g];
        grafico.update();
        if(data.temperatura > 10.0 || data.estado.includes('EMERGENCIA')){
          document.getElementById('alerta').style.display = 'block';
        } else {
          document.getElementById('alerta').style.display = 'none';
        }
      });
    }, 1000);
    function dispararEmergenciaVirtual(){ fetch('/emergencia_virtual'); }
    function dispararIniciarVirtual(){ fetch('/iniciar_virtual'); }
  </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void enviarDadosJSON() {
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  String json = "{";
  json += "\"estado\":\"" + obterNomeEstado(estadoAtual) + "\",";
  json += "\"temperatura\":" + String(temperaturaAtual) + ",";
  json += "\"p\":" + String(quantidadeCaixasP) + ",";
  json += "\"m\":" + String(quantidadeCaixasM) + ",";
  json += "\"g\":" + String(quantidadeCaixasG);
  json += "}";
  xSemaphoreGive(mutexDados);
  server.send(200, "application/json", json);
}

void tratarBotaoVirtualEmergencia() {
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  estadoEmergencia = !estadoEmergencia;
  xSemaphoreGive(mutexDados);
  server.send(200, "text/plain", "Emergencia Alternada");
}

void tratarBotaoVirtualIniciar() {
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  if (estadoAtual == AGUARDANDO_START && !estadoEmergencia && !emergenciaTemperatura) {
    estadoAtual = MANIPULADOR1_PEGA_CAIXA;
    Serial.println("Comando INICIAR recebido via WEB!");
  }
  xSemaphoreGive(mutexDados);
  server.send(200, "text/plain", "Comando de Iniciar Processado");
}

String obterNomeEstado(EstadosSistema estado) {
  switch(estado) {
    case AGUARDANDO_START: return "Aguardando Start";
    case MANIPULADOR1_PEGA_CAIXA: return "Pegando Caixa";
    case ESTEIRA_TRANSPORTANDO: return "Esteira Ligada";
    case AGUARDANDO_FIM_ESTEIRA: return "Caixa em Curso";
    case MANIPULADOR2_SEPARA_CAIXA: return "Separando Caixa";
    case RESETANDO_MAQUINA: return "Resetando";
    case EM_EMERGENCIA: return "!!!EMERGENCIA!!!";
    default: return "Desconhecido";
  }
}
