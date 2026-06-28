#include <Arduino.h>
#include <ESP32Servo.h>
#include <HCSR04.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

// Substitui pelo nome e senha do Wi-Fi
const char* ssid = "POCO F7";
const char* password = "boracarnavalnorio";

WebServer server(80);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int stepPin = 16;
const int dirPin = 4;
const int frequenciaMotor = 70;

const int pino_trigger = 2;
const int pino_echo = 15;
UltraSonicDistanceSensor distanceSensor(pino_trigger, pino_echo);

const int iri = 14;
bool estadoIR = false;

const int pinbraco11 = 13;  //garra
const int pinbraco12 = 19;  //direita
const int pinbraco13 = 12;  //esquerda
const int pinbraco14 = 5;   // baixo 

Servo braco11, braco12, braco13, braco14;

const int botaoIniciar = 33;
const int botaoEmergencia = 34;

const int buzzer = 23; //pin 23
const int LM35 = 36; // VP Pin no ESP32 (ADC1_CH0)

const int LED_R = 5;
const int LED_G = 18;
const int LED_B = 19;
const int transistor = 26;

// --- VARIÁVEIS COMPARTILHADAS (Protegidas pelo Mutex) ---
int quantidadeCaixasP = 0;
int quantidadeCaixasM = 0;
int quantidadeCaixasG = 0;
char tamanhoCaixaMedida = '-';
volatile float temperaturaAtual = 0.0;
volatile bool estadoEmergencia = false;
volatile unsigned long tempoUltimoClique = 0; // Variável para o Debounce do botão

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
// --------------------------------------------------------

int medidaSemCaixa = 9;
int medidaCaixaP = 6;
int medidaCaixaM = 5;
int medidaCaixaG = 4;
bool viACaixa = false;
char tamanho;
char tamanhoAtual;

volatile bool flagMedirTemperatura = false; // Flag da 2ª Interrupção

// Criação do Mutex e do Timer
SemaphoreHandle_t mutexDados; 
hw_timer_t *timerTemperatura = NULL;

// Prototipação das funções
void loop0(void *parameter);
void loop1(void *parameter);
void ligar_esteira();
void desligar_esteira();
bool leituraIR();
float medirDistancia();
void sensorDeCaixa();
void pegarCaixa();
void colocarCaixaP();
void colocarCaixaM();
void colocarCaixaG();
void zeraTudo();
void setCorRGB(int r, int g, int b);
void enviarPaginaWeb();
void enviarDadosJSON();
void tratarBotaoVirtualEmergencia();
String obterNomeEstado(EstadosSistema estado);

// 1ª INTERRUPÇÃO: Botão Físico (COM DEBOUNCE E ALTERNÂNCIA)
void IRAM_ATTR emergencia() {
  unsigned long tempoAtual = millis();
  
  // Se passou mais de 300ms desde o último clique, considera válido
  if (tempoAtual - tempoUltimoClique > 300) {
    estadoEmergencia = !estadoEmergencia; // Inverte o estado (se for true passa a false)
    tempoUltimoClique = tempoAtual;
  }
}

// 2ª INTERRUPÇÃO OBRIGATÓRIA: Temporização Periódica por Hardware
void IRAM_ATTR onTimerTemperatura() {
  flagMedirTemperatura = true; 
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // Inicializa o Mutex (Memória Compartilhada)
  mutexDados = xSemaphoreCreateMutex();

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(transistor, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(LM35, INPUT);

  digitalWrite(transistor, HIGH);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);
  digitalWrite(buzzer, LOW);
  setCorRGB(0, 0, 255); // Azul: Inicializando

  Serial.print("Conectando ao Wi-Fi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado!");

  // Configuração das rotas do Servidor Web
  server.on("/", enviarPaginaWeb);     
  server.on("/dados", enviarDadosJSON); 
  server.on("/emergencia_virtual", tratarBotaoVirtualEmergencia);
  server.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("ERRO CRITICO: SSD1306 nao encontrado!"));
  } else {
    Serial.println("OLED encontrado!");
    display.clearDisplay();
    display.display();
  }

  // Configuração das Interrupções
  pinMode(botaoIniciar, INPUT);
  pinMode(botaoEmergencia, INPUT);
  attachInterrupt(digitalPinToInterrupt(botaoEmergencia), emergencia, RISING);

  timerTemperatura = timerBegin(0, 80, true); 
  timerAttachInterrupt(timerTemperatura, &onTimerTemperatura, true);
  timerAlarmWrite(timerTemperatura, 1000000, true); // 1s
  timerAlarmEnable(timerTemperatura);

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(stepPin, LOW);
  digitalWrite(dirPin, LOW);
  ledcSetup(0, frequenciaMotor, 8);
  ledcAttachPin(stepPin, 0);

  pinMode(iri, INPUT);

  braco11.attach(pinbraco11, 500, 2400); 
  braco12.attach(pinbraco11, 500, 2400);
  braco13.attach(pinbraco13, 500, 2400);  
  braco14.attach(pinbraco14, 500, 2400);

  // Criação das tarefas
  xTaskCreatePinnedToCore(loop0, "Task0", 10000, NULL, 1, NULL, 0); // Core 0
  xTaskCreatePinnedToCore(loop1, "Task1", 10000, NULL, 1, NULL, 1); // Core 1

  // Configuração de Teste
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
}

void loop() {}

// CORE 0 - Controle e Sensores em Tempo Real
void loop0(void *parameter) {
  // estadoAtual = ESTEIRA_TRANSPORTANDO;
  Serial.println("CORE 0 INICIADO!");
  while (true) {
    
    // Leitura da Temperatura (Controlada pela Interrupção)
    if (flagMedirTemperatura) {
      flagMedirTemperatura = false;
      int leituraADC = analogRead(LM35);
      float temp = ((leituraADC / 4095.0) * 3300.0) / 10.0;
      
      xSemaphoreTake(mutexDados, portMAX_DELAY);
      temperaturaAtual = temp;
      if (temperaturaAtual > 65.0) estadoEmergencia = true; // Incêndio
      xSemaphoreGive(mutexDados);
    }

    // Leitura segura do estado de emergência e temperatura
    xSemaphoreTake(mutexDados, portMAX_DELAY);
    bool emEmergencia = estadoEmergencia;
    float tempCheck = temperaturaAtual;
    xSemaphoreGive(mutexDados);

    // Gestão Segura de Entrada/Saída de Emergência
    if (emEmergencia && estadoAtual != EM_EMERGENCIA) {
      Serial.println("ENTRANDO EM EMERGENCIA!");
      xSemaphoreTake(mutexDados, portMAX_DELAY);
      estadoAtual = EM_EMERGENCIA;
      xSemaphoreGive(mutexDados);
    } 
    else if (!emEmergencia && estadoAtual == EM_EMERGENCIA) {
      if (tempCheck < 55.0) {
        Serial.println("SAINDO DA EMERGENCIA!");
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        estadoAtual = AGUARDANDO_START;
        xSemaphoreGive(mutexDados);
        digitalWrite(buzzer, LOW); // Desliga o aviso sonoro
      } else {
        Serial.println("MUITO QUENTE! Manter na emergencia.");
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        estadoEmergencia = true; // Força o estado bloqueado por segurança
        xSemaphoreGive(mutexDados);
      }
    }

    switch (estadoAtual) {
      case AGUARDANDO_START:
        digitalWrite(buzzer, LOW);
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
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        estadoAtual = ESTEIRA_TRANSPORTANDO;
        xSemaphoreGive(mutexDados);
        break;

      case ESTEIRA_TRANSPORTANDO:
        // Serial.println("ESTEIRA TRANSPORTANDO!");
        ligar_esteira();
        sensorDeCaixa(); 
        
        xSemaphoreTake(mutexDados, portMAX_DELAY);
        tamanhoAtual = tamanhoCaixaMedida;
        xSemaphoreGive(mutexDados);

        if (tamanhoAtual != '-') {
          xSemaphoreTake(mutexDados, portMAX_DELAY);
          estadoAtual = AGUARDANDO_FIM_ESTEIRA;
          xSemaphoreGive(mutexDados);
        }
        break;

      case AGUARDANDO_FIM_ESTEIRA:
        Serial.println("AGUARDANDO FIM ESTEIRA!");
        if (leituraIR() == true) {
          desligar_esteira();
          xSemaphoreTake(mutexDados, portMAX_DELAY);
          estadoAtual = MANIPULADOR2_SEPARA_CAIXA;
          xSemaphoreGive(mutexDados);
        }
        break;

      case MANIPULADOR2_SEPARA_CAIXA:
        Serial.println("MANIPULADOR 2 SEPARA CAIXA!");
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
        // APENAS desliga e apita. Sem ciclos 'while' infinitos aqui!
        desligar_esteira();
        digitalWrite(buzzer, HIGH); 
        break;
    }
    
    // Essencial para o Watchdog Timer não resetar o tamanhoF = medirDistancia();
    // Serial ESP32
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// CORE 1 - Interface, display, RGB e WebServer
void loop1(void *parameter) {
  Serial.println("CORE 1 INICIADO!");
  while (true) {
    server.handleClient(); // Roda o Servidor Web

    // ==========================================
    // 1. LEITURA SEGURA DE TODAS AS VARIÁVEIS
    // ==========================================
    xSemaphoreTake(mutexDados, portMAX_DELAY);
    EstadosSistema estadoLocal = estadoAtual;
    int pLocal = quantidadeCaixasP;
    int mLocal = quantidadeCaixasM;
    int gLocal = quantidadeCaixasG;
    char ultimaLocal = tamanhoCaixaMedida;
    float tempLocal = temperaturaAtual;
    xSemaphoreGive(mutexDados);

    // ==========================================
    // 2. ATUALIZA O LED RGB
    // ==========================================
    if (estadoLocal == EM_EMERGENCIA) {
      setCorRGB(255, 0, 0);     // Vermelho: Erro/Emergência
    } else if (estadoLocal == AGUARDANDO_START) {
      setCorRGB(0, 0, 255);     // Azul: Aguardando comando
    } else if (estadoLocal == ESTEIRA_TRANSPORTANDO || estadoLocal == MANIPULADOR1_PEGA_CAIXA || estadoLocal == MANIPULADOR2_SEPARA_CAIXA) {
      setCorRGB(255, 255, 0);   // Amarelo: Manipulação
    } else {
      setCorRGB(0, 255, 0);     // Verde: Normal
    }

    // ==========================================
    // 3. ATUALIZA O DISPLAY OLED
    // ==========================================
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
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

// FUNÇÕES DA ESTEIRA E BRAÇOS
void ligar_esteira() { 
  digitalWrite(dirPin, HIGH); 
  ledcWrite(0, 128); 
}

void desligar_esteira() { 
  digitalWrite(dirPin, LOW); 
  ledcWrite(0, 0); 
}

bool leituraIR() { return (digitalRead(iri) == LOW); }

float medirDistancia() { 
  return distanceSensor.measureDistanceCm(); 
}

void sensorDeCaixa() {
  // Se já detectou uma caixa nesta passagem, não faz nada
  if (viACaixa) return;

  // Lê a distância (em cm)
  float distancia = medirDistancia();

  // 1. Ignora leitura inválida (erro do sensor)
  if (distancia == -1) {
    Serial.println("Erro na leitura do ultrassom");
    return;
  }

  // 2. Ignora se for a distância de "sem caixa" (com margem de 0,5 cm)
  //    Ex: se medidaSemCaixa = 9, considera sem caixa se >= 8,5 cm
  if (distancia >= (medidaSemCaixa - 0.5)) {
    // Sem caixa → não faz nada
    return;
  }

  // 3. Classifica o tamanho da caixa
  char tipo = '-';
  if (distancia < medidaCaixaG) {        // menor distância → caixa GRANDE
    tipo = 'G';
    Serial.print("CAIXA GRANDE: ");
    Serial.println(distancia);
  } else if (distancia < medidaCaixaM) { // distância média → caixa MÉDIA
    tipo = 'M';
    Serial.print("CAIXA MÉDIA: ");
    Serial.println(distancia);
  } else if (distancia < medidaCaixaP) { // distância maior → caixa PEQUENA
    tipo = 'P';
    Serial.print("CAIXA PEQUENA: ");  
    Serial.println(distancia);
  } else {
    // Se a distância não se encaixar em nenhuma faixa, provavelmente é ruído
    return;
  }

  // 4. Atualiza os contadores e a variável global (protegido por mutex)
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

  // Marca que a caixa foi detectada (evita repetições)
  viACaixa = true;

  // Exibe no Serial qual caixa foi vista (útil para debug)
  Serial.print("Caixa detectada: ");
  Serial.println(tipo);
}

// void sensorDeCaixa() {
//   if (viACaixa == false) {
//     float tamanhoF = medirDistancia();
//     Serial.print("Caixa de tamanhoooooooooooooooooo: "); Serial.print(tamanhoF); Serial.println(" cm");
    
//     // BLOQUEIA O MUTEX PARA ATUALIZAR OS DADOS
//     xSemaphoreTake(mutexDados, portMAX_DELAY);
//     if (tamanhoF == -1){
//       sensorDeCaixa();
//       Serial.println("CAIXA NAO DETECTADAaaaaaaaaaaaaaaaaaaaaaaaaaa");
//       delay(250);
//     }
//     else if (tamanhoF < medidaCaixaG) {
//       quantidadeCaixasG++;
//       tamanhoCaixaMedida = 'G';
//       viACaixa = true;
//     } else if (tamanhoF < medidaCaixaM) {
//       quantidadeCaixasM++;
//       tamanhoCaixaMedida = 'M';
//       viACaixa = true;
//     } else if (tamanhoF < medidaCaixaP) {
//       quantidadeCaixasP++;
//       tamanhoCaixaMedida = 'P';      
//       viACaixa = true;
//     }
//     xSemaphoreGive(mutexDados);
//   }
// }

void pegarCaixa() {
  // braco11.write(0);
  // delay(500);
  // braco11.write(20);
  // delay(1000);

  // braco11.write(45); 
  // vTaskDelay(pdMS_TO_TICKS(300));
  // braco12.write(45); 
  // braco13.write(45); 
  // vTaskDelay(pdMS_TO_TICKS(300));
  // braco14.write(45);
}

void colocarCaixaP() { 
  // braco21.write(45); 
  // vTaskDelay(pdMS_TO_TICKS(30)); 
  // braco22.write(45); 
  // braco23.write(45); 
  // vTaskDelay(pdMS_TO_TICKS(30)); 
  // braco24.write(15); 
}

void colocarCaixaM() { 
  // braco21.write(45); 
  // vTaskDelay(pdMS_TO_TICKS(30)); 
  // braco22.write(45); 
  // braco23.write(45); 
  // vTaskDelay(pdMS_TO_TICKS(30)); 
  // braco24.write(45); 
}

void colocarCaixaG() { 
  // braco21.write(45); 
  // vTaskDelay(pdMS_TO_TICKS(30)); 
  // braco22.write(45); 
  // braco23.write(45); 
  // vTaskDelay(pdMS_TO_TICKS(30)); 
  // braco24.write(75); 
}

void zeraTudo() {
  viACaixa = false;
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  
  xSemaphoreGive(mutexDados);
  braco11.write(27); braco12.write(25); braco13.write(33); braco14.write(32);
}

void setCorRGB(int r, int g, int b) { 
  analogWrite(LED_R, r); 
  analogWrite(LED_G, g); 
  analogWrite(LED_B, b); 
}

// INTERFACE WEB
void enviarPaginaWeb() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Painel Esteira UTFPR</title>";
  html += "<style>body{font-family:Arial; text-align:center; background:#f4f4f4; margin:0; padding:20px;} ";
  html += ".card{background:white; padding:20px; margin:10px auto; max-width:450px; border-radius:8px; box-shadow:0 4px 8px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;} .status{font-weight:bold; color:blue;}";
  html += ".btn{padding:15px 25px; font-size:16px; margin:10px; border:none; border-radius:5px; cursor:pointer; font-weight:bold;}";
  html += ".btn-danger{background-color:#d9534f; color:white;} .btn-danger:hover{background-color:#c9302c;}";
  html += ".danger-alert{background-color:#f2dede; color:#a94442; border:1px solid #ebccd1; padding:15px; border-radius:4px; margin-bottom:15px; font-weight:bold;}";
  html += "</style>";
  
  html += "<script>setInterval(function() {";
  html += "  fetch('/dados').then(response => response.json()).then(data => {";
  html += "    document.getElementById('status').innerText = data.estado;";
  html += "    document.getElementById('temp').innerText = data.temperatura.toFixed(1);";
  html += "    document.getElementById('qtdP').innerText = data.p;";
  html += "    document.getElementById('qtdM').innerText = data.m;";
  html += "    document.getElementById('qtdG').innerText = data.g;";
  html += "    document.getElementById('total').innerText = data.p + data.m + data.g;";
  html += "    if(data.temperatura > 45.0 || data.estado.includes('EMERGENCIA')){";
  html += "      document.getElementById('alerta').style.display = 'block';";
  html += "    } else { document.getElementById('alerta').style.display = 'none'; }";
  html += "  });";
  html += "}, 1000);";
  html += "function dispararEmergenciaVirtual(){ fetch('/emergencia_virtual'); }</script>";
  
  html += "</head><body>";
  html += "<h1>UTFPR - Sistema de Separacao</h1>";
  html += "<div id='alerta' class='card danger-alert' style='display:none;'>!!! ALERTA DE INCENDIO / EMERGENCIA !!!</div>";
  html += "<div class='card'><h2>Status: <span id='status'>Carregando...</span></h2><h3>Temperatura: <span id='temp'>0.0</span> C</h3></div>";
  html += "<div class='card'><h3>Contador de Caixas:</h3><p>Total: <b id='total'>0</b></p><p>Pequenas: <span id='qtdP'>0</span></p><p>Medias: <span id='qtdM'>0</span></p><p>Grandes: <span id='qtdG'>0</span></p></div>";
  html += "<div class='card'><button class='btn btn-danger' onclick='dispararEmergenciaVirtual()'>EMERGENCIA VIRTUAL</button></div>";
  html += "</body></html>";
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
  estadoEmergencia = !estadoEmergencia; // Para que o botão virtual também alterne (entre e saia)
  xSemaphoreGive(mutexDados);
  server.send(200, "text/plain", "Emergencia Alternada");
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





















// #include <Arduino.h>
// #include <ESP32Servo.h>
// #include <HCSR04.h>
// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
// #include <WiFi.h>
// #include <WebServer.h>

//   // Substitua pelo nome e senha do Wi-Fi da sua casa ou laboratório
//   const char* ssid = "POCO F7";
//   const char* password = "boracarnavalnorio";

//   WebServer server(80);

//   #define SCREEN_WIDTH 128
//   #define SCREEN_HEIGHT 64
//   #define OLED_ADDR 0x3C
//   Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//   const int stepPin = 15;
//   const int dirPin = 4;
//   const int frequenciaMotor = 70;

//   const int pino_trigger = 5;
//   const int pino_echo = 2;
//   UltraSonicDistanceSensor distanceSensor(pino_trigger, pino_echo);

//   const int iri = 14;
//   bool estadoIR = false;

//   Servo braco11, braco12, braco13, braco14;
//   Servo braco21, braco22, braco23, braco24;

//   const int botaoIniciar = 25;
//   const int botaoEmergencia = 34;

//   const int buzzer = 2; //pin 23
//   const int LM35 = 36; // VP Pin no ESP32 (ADC1_CH0)

//   const int LED_R = 25;
//   const int LED_G = 26;
//   const int LED_B = 27;

//   // --- VARIÁVEIS COMPARTILHADAS (Protegidas pelo Mutex) ---
//   int quantidadeCaixasP = 0;
//   int quantidadeCaixasM = 0;
//   int quantidadeCaixasG = 0;
//   char tamanhoCaixaMedida = '-';
//   volatile float temperaturaAtual = 0.0;
//   volatile bool estadoEmergencia = false;

//   enum EstadosSistema {
//     AGUARDANDO_START,
//     MANIPULADOR1_PEGA_CAIXA,
//     ESTEIRA_TRANSPORTANDO,
//     AGUARDANDO_FIM_ESTEIRA,
//     MANIPULADOR2_SEPARA_CAIXA,
//     RESETANDO_MAQUINA,
//     EM_EMERGENCIA
//   };
//   EstadosSistema estadoAtual = AGUARDANDO_START;
//   // --------------------------------------------------------

//   int medidaSemCaixa = 15;
//   int medidaCaixaP = 12;
//   int medidaCaixaM = 10;
//   int medidaCaixaG = 8;
//   bool viACaixa = false;
//   char tamanho;
//   char tamanhoAtual;

//   volatile bool flagMedirTemperatura = false; // Flag da 2ª Interrupção

//   // Criação do Mutex e do Timer
//   SemaphoreHandle_t mutexDados; 
//   hw_timer_t *timerTemperatura = NULL;

//   // Prototipação das funções
//   void loop0(void *parameter);
//   void loop1(void *parameter);
//   void ligar_esteira();
//   void desligar_esteira();
//   bool leituraIR();
//   float medirDistancia();
//   void sensorDeCaixa();
//   void pegarCaixa();
//   void colocarCaixaP();
//   void colocarCaixaM();
//   void colocarCaixaG();
//   void zeraTudo();
//   void setCorRGB(int r, int g, int b);
//   void enviarPaginaWeb();
//   void enviarDadosJSON();
//   void tratarBotaoVirtualEmergencia();
//   String obterNomeEstado(EstadosSistema estado);

//   // 1ª INTERRUPÇÃO: Botão Físico
//   void IRAM_ATTR emergencia() {
//     estadoEmergencia = true;
//   }

//   // 2ª INTERRUPÇÃO OBRIGATÓRIA: Temporização Periódica por Hardware
//   void IRAM_ATTR onTimerTemperatura() {
//     flagMedirTemperatura = true; 
//   }

//   void setup() {
//     Serial.begin(115200);
//     Wire.begin(21, 22);

//     // Inicializa o Mutex (Memória Compartilhada)
//     mutexDados = xSemaphoreCreateMutex();

//     pinMode(LED_R, OUTPUT);
//     pinMode(LED_G, OUTPUT);
//     pinMode(LED_B, OUTPUT);
//     pinMode(buzzer, OUTPUT);
//     pinMode(LM35, INPUT);

//     digitalWrite(buzzer, LOW);
//     setCorRGB(0, 0, 255); // Azul: Inicializando

//     Serial.print("Conectando ao Wi-Fi ");
//     WiFi.begin(ssid, password);
//     while (WiFi.status() != WL_CONNECTED) {
//       delay(500);
//       Serial.print(".");
//     }
//     Serial.println("\nWi-Fi Conectado!");

//     // Configuração das rotas do Servidor Web
//     server.on("/", enviarPaginaWeb);     
//     server.on("/dados", enviarDadosJSON); 
//     server.on("/emergencia_virtual", tratarBotaoVirtualEmergencia);
//     server.begin();

//     if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
//       Serial.println(F("ERRO CRITICO: SSD1306 nao encontrado!"));
//     } else {
//       Serial.println("OLED encontrado!");
//       display.clearDisplay();
//       display.display();
//     }

//     // Configuração das Interrupções
//     pinMode(botaoIniciar, INPUT);
//     pinMode(botaoEmergencia, INPUT);
//     attachInterrupt(digitalPinToInterrupt(botaoEmergencia), emergencia, RISING);

//     timerTemperatura = timerBegin(0, 80, true); 
//     timerAttachInterrupt(timerTemperatura, &onTimerTemperatura, true);
//     timerAlarmWrite(timerTemperatura, 1000000, true); // 1s
//     timerAlarmEnable(timerTemperatura);

//     pinMode(stepPin, OUTPUT);
//     pinMode(dirPin, OUTPUT);
//     digitalWrite(stepPin, LOW);
//     digitalWrite(dirPin, LOW);
//     ledcSetup(0, frequenciaMotor, 8);
//     ledcAttachPin(stepPin, 0);

//     pinMode(iri, INPUT);

//     braco11.attach(30, 500, 2400); braco12.attach(31, 500, 2400);
//     braco13.attach(32, 500, 2400); braco14.attach(33, 500, 2400);

//     braco21.attach(34, 500, 2400); braco22.attach(35, 500, 2400);
//     braco23.attach(36, 500, 2400); braco24.attach(37, 500, 2400);

//     // Criação das tarefas
//     xTaskCreatePinnedToCore(loop0, "Task0", 10000, NULL, 1, NULL, 0); // Core 0
//     xTaskCreatePinnedToCore(loop1, "Task1", 10000, NULL, 1, NULL, 1); // Core 1
//   }

//   void loop() {}

//   // CORE 0 - Controle e Sensores em Tempo Real
//   void loop0(void *parameter) {
//     Serial.println("CORE 0 INICIADO!");
//     while (true) {
      
//       // Leitura da Temperatura (Controlada pela Interrupção)
//       if (flagMedirTemperatura) {
//         flagMedirTemperatura = false;
//         int leituraADC = analogRead(LM35);
//         float temp = ((leituraADC / 4095.0) * 3300.0) / 10.0;
        
//         xSemaphoreTake(mutexDados, portMAX_DELAY);
//         temperaturaAtual = temp;
//         if (temperaturaAtual > 65.0) estadoEmergencia = true; // Incêndio
//         xSemaphoreGive(mutexDados);
//       }

//       // Verifica Emergência (Pode vir do botão físico, do sensor ou da Web)
//       xSemaphoreTake(mutexDados, portMAX_DELAY);
//       bool emEmergencia = estadoEmergencia;
//       xSemaphoreGive(mutexDados);

//       if (emEmergencia) {
//         xSemaphoreTake(mutexDados, portMAX_DELAY);
//         estadoAtual = EM_EMERGENCIA;
//         xSemaphoreGive(mutexDados);
//       }

//       switch (estadoAtual) {
//         case AGUARDANDO_START:
//           digitalWrite(buzzer, LOW);
//           if (digitalRead(botaoIniciar) == HIGH) {
//             xSemaphoreTake(mutexDados, portMAX_DELAY);
//             estadoAtual = MANIPULADOR1_PEGA_CAIXA;
//             xSemaphoreGive(mutexDados);
//             while(digitalRead(botaoIniciar) == HIGH) { vTaskDelay(pdMS_TO_TICKS(10)); }
//           }
//           break;

//         case MANIPULADOR1_PEGA_CAIXA:
//           pegarCaixa();
//           xSemaphoreTake(mutexDados, portMAX_DELAY);
//           estadoAtual = ESTEIRA_TRANSPORTANDO;
//           xSemaphoreGive(mutexDados);
//           break;

//         case ESTEIRA_TRANSPORTANDO:
//           ligar_esteira();
//           sensorDeCaixa(); 
          
//           xSemaphoreTake(mutexDados, portMAX_DELAY);
//           tamanhoAtual = tamanhoCaixaMedida;
//           xSemaphoreGive(mutexDados);

//           if (tamanhoAtual != '-') {
//             xSemaphoreTake(mutexDados, portMAX_DELAY);
//             estadoAtual = AGUARDANDO_FIM_ESTEIRA;
//             xSemaphoreGive(mutexDados);
//           }
//           break;

//         case AGUARDANDO_FIM_ESTEIRA:
//           if (leituraIR() == true) {
//             desligar_esteira();
//             xSemaphoreTake(mutexDados, portMAX_DELAY);
//             estadoAtual = MANIPULADOR2_SEPARA_CAIXA;
//             xSemaphoreGive(mutexDados);
//           }
//           break;

//         case MANIPULADOR2_SEPARA_CAIXA:
//           xSemaphoreTake(mutexDados, portMAX_DELAY);
//           tamanho = tamanhoCaixaMedida;
//           xSemaphoreGive(mutexDados);

//           if (tamanho == 'P') colocarCaixaP();
//           else if (tamanho == 'M') colocarCaixaM();
//           else if (tamanho == 'G') colocarCaixaG();
          
//           xSemaphoreTake(mutexDados, portMAX_DELAY);
//           estadoAtual = RESETANDO_MAQUINA;
//           xSemaphoreGive(mutexDados);
//           break;

//         case RESETANDO_MAQUINA:
//           zeraTudo();
//           xSemaphoreTake(mutexDados, portMAX_DELAY);
//           estadoAtual = AGUARDANDO_START;
//           xSemaphoreGive(mutexDados);
//           break;

//         case EM_EMERGENCIA:
//           Serial.println("EMERGENCIA ATIVADA!");
//           desligar_esteira();
//           digitalWrite(buzzer, HIGH); 
          
//           while(estadoAtual == EM_EMERGENCIA) {
//             // Serial.println("EMERGENCIA WHILE!!!");
//             xSemaphoreTake(mutexDados, portMAX_DELAY);
//             float tempCheck = temperaturaAtual;
//             xSemaphoreGive(mutexDados);

//             Serial.println(digitalRead(botaoEmergencia) == HIGH && tempCheck < 55.0);
//             if (digitalRead(botaoEmergencia) == HIGH && tempCheck < 55.0) { 
//               while(digitalRead(botaoEmergencia) == HIGH){}
//               Serial.println("Saindo da emergencia!");
//               Serial.println("EMERGENCIA DESATIVADA!");
//               xSemaphoreTake(mutexDados, portMAX_DELAY);
//               estadoEmergencia = false;
//               estadoAtual = AGUARDANDO_START;
//               xSemaphoreGive(mutexDados);
//             }
//           }
//           break;
//       }
//       vTaskDelay(pdMS_TO_TICKS(50));
//     }
//   }

//   // CORE 1 - Interface, display, RGB e WebServer
//   void loop1(void *parameter) {
//     Serial.println("CORE 1 INICIADO!");
//     while (true) {
//       server.handleClient(); // Roda o Servidor Web

//       // ==========================================
//       // 1. LEITURA SEGURA DE TODAS AS VARIÁVEIS
//       // ==========================================
//       xSemaphoreTake(mutexDados, portMAX_DELAY);
//       EstadosSistema estadoLocal = estadoAtual;
//       int pLocal = quantidadeCaixasP;
//       int mLocal = quantidadeCaixasM;
//       int gLocal = quantidadeCaixasG;
//       char ultimaLocal = tamanhoCaixaMedida;
//       float tempLocal = temperaturaAtual;
//       xSemaphoreGive(mutexDados);

//       // ==========================================
//       // 2. ATUALIZA O LED RGB
//       // ==========================================
//       if (estadoLocal == EM_EMERGENCIA) {
//         setCorRGB(255, 0, 0);     // Vermelho: Erro/Emergência
//       } else if (estadoLocal == AGUARDANDO_START) {
//         setCorRGB(0, 0, 255);     // Azul: Aguardando comando
//       } else if (estadoLocal == ESTEIRA_TRANSPORTANDO || estadoLocal == MANIPULADOR1_PEGA_CAIXA || estadoLocal == MANIPULADOR2_SEPARA_CAIXA) {
//         setCorRGB(255, 255, 0);   // Amarelo: Manipulação
//       } else {
//         setCorRGB(0, 255, 0);     // Verde: Normal
//       }

//       // ==========================================
//       // 3. ATUALIZA O DISPLAY OLED
//       // ==========================================
//       display.clearDisplay();
//       display.setTextSize(1);
//       display.setTextColor(SSD1306_WHITE);

//       display.setCursor(0, 0); display.println("UTFPR - MECHATRONICS"); 
//       display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
//       display.setCursor(0, 13); display.print("IP: "); display.println(WiFi.localIP().toString()); 
//       display.setCursor(0, 24); display.print("Stat: "); display.println(obterNomeEstado(estadoLocal)); 
//       display.setCursor(0, 35); display.print("Temp: "); display.print(tempLocal, 1); display.println(" C"); 
//       display.setCursor(0, 45); display.print("Ultima: "); display.println(ultimaLocal); 
//       display.setCursor(0, 55);
//       display.print("T:"); display.print(pLocal + mLocal + gLocal); 
//       display.print("        P:"); display.print(pLocal);
//       display.print(" M:"); display.print(mLocal);
//       display.print(" G:"); display.print(gLocal);

//       display.display();
//       vTaskDelay(pdMS_TO_TICKS(100)); 
//     }
//   }

//   // FUNÇÕES DA ESTEIRA E BRAÇOS
//   void ligar_esteira() { 
//     digitalWrite(dirPin, HIGH); 
//     ledcWrite(0, 128); 
//     }

//   void desligar_esteira() { 
//     digitalWrite(dirPin, LOW); 
//     ledcWrite(0, 0); 
//     }

//   bool leituraIR() { return (
//     digitalRead(iri) == LOW); 
//     }

//   float medirDistancia() { 
//     return distanceSensor.measureDistanceCm(); 
//     }

//   void sensorDeCaixa() {
//     if (viACaixa == false) {
//       float tamanho = medirDistancia();
      
//       // BLOQUEIA O MUTEX PARA ATUALIZAR OS DADOS
//       xSemaphoreTake(mutexDados, portMAX_DELAY);
//       if (tamanho < medidaCaixaG) {
//         quantidadeCaixasG++;
//         tamanhoCaixaMedida = 'G';
//         viACaixa = true;
//       } else if (tamanho < medidaCaixaM) {
//         quantidadeCaixasM++;
//         tamanhoCaixaMedida = 'M';
//         viACaixa = true;
//       } else if (tamanho < medidaCaixaP) {
//         quantidadeCaixasP++;
//         tamanhoCaixaMedida = 'P';      
//         viACaixa = true;
//       }
//       xSemaphoreGive(mutexDados);
//     }
//   }

//   void pegarCaixa() {
//     braco11.write(45); 
//     vTaskDelay(pdMS_TO_TICKS(30));
//     braco12.write(45); 
//     braco13.write(45); 
//     vTaskDelay(pdMS_TO_TICKS(30));
//     braco14.write(45);
//   }

//   void colocarCaixaP() { 
//     braco21.write(45); 
//     vTaskDelay(pdMS_TO_TICKS(30)); 
//     braco22.write(45); 
//     braco23.write(45); 
//     vTaskDelay(pdMS_TO_TICKS(30)); 
//     braco24.write(15); 
//   }

//   void colocarCaixaM() { 
//     braco21.write(45); 
//     vTaskDelay(pdMS_TO_TICKS(30)); 
//     braco22.write(45); 
//     braco23.write(45); 
//     vTaskDelay(pdMS_TO_TICKS(30)); 
//     braco24.write(45); 
//     }

//   void colocarCaixaG() { 
//     braco21.write(45); 
//     vTaskDelay(pdMS_TO_TICKS(30)); 
//     braco22.write(45); 
//     braco23.write(45); 
//     vTaskDelay(pdMS_TO_TICKS(30)); 
//     braco24.write(75); 
//   }

//   void zeraTudo() {
//     viACaixa = false;
//     xSemaphoreTake(mutexDados, portMAX_DELAY);
//     tamanhoCaixaMedida = '-';
//     xSemaphoreGive(mutexDados);
//     braco11.write(0); braco12.write(0); braco13.write(0); braco14.write(0);
//     braco21.write(0); braco22.write(0); braco23.write(0); braco24.write(0);
//   }

//   void setCorRGB(int r, int g, int b) { 
//     analogWrite(LED_R, r); 
//     analogWrite(LED_G, g); 
//     analogWrite(LED_B, b); 
//   }

// // INTERFACE WEB
// void enviarPaginaWeb() {
//   String html = "<!DOCTYPE html><html>";
//   html += "<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
//   html += "<title>Painel Esteira UTFPR</title>";
//   html += "<style>body{font-family:Arial; text-align:center; background:#f4f4f4; margin:0; padding:20px;} ";
//   html += ".card{background:white; padding:20px; margin:10px auto; max-width:450px; border-radius:8px; box-shadow:0 4px 8px rgba(0,0,0,0.1);}";
//   html += "h1{color:#333;} .status{font-weight:bold; color:blue;}";
//   html += ".btn{padding:15px 25px; font-size:16px; margin:10px; border:none; border-radius:5px; cursor:pointer; font-weight:bold;}";
//   html += ".btn-danger{background-color:#d9534f; color:white;} .btn-danger:hover{background-color:#c9302c;}";
//   html += ".danger-alert{background-color:#f2dede; color:#a94442; border:1px solid #ebccd1; padding:15px; border-radius:4px; margin-bottom:15px; font-weight:bold;}";
//   html += "</style>";
  
//   html += "<script>setInterval(function() {";
//   html += "  fetch('/dados').then(response => response.json()).then(data => {";
//   html += "    document.getElementById('status').innerText = data.estado;";
//   html += "    document.getElementById('temp').innerText = data.temperatura.toFixed(1);";
//   html += "    document.getElementById('qtdP').innerText = data.p;";
//   html += "    document.getElementById('qtdM').innerText = data.m;";
//   html += "    document.getElementById('qtdG').innerText = data.g;";
//   html += "    document.getElementById('total').innerText = data.p + data.m + data.g;";
//   html += "    if(data.temperatura > 45.0 || data.estado.includes('EMERGENCIA')){";
//   html += "      document.getElementById('alerta').style.display = 'block';";
//   html += "    } else { document.getElementById('alerta').style.display = 'none'; }";
//   html += "  });";
//   html += "}, 1000);";
//   html += "function dispararEmergenciaVirtual(){ fetch('/emergencia_virtual'); }</script>";
  
//   html += "</head><body>";
//   html += "<h1>UTFPR - Sistema de Separacao</h1>";
//   html += "<div id='alerta' class='card danger-alert' style='display:none;'>!!! ALERTA DE INCENDIO / EMERGENCIA !!!</div>";
//   html += "<div class='card'><h2>Status: <span id='status'>Carregando...</span></h2><h3>Temperatura: <span id='temp'>0.0</span> C</h3></div>";
//   html += "<div class='card'><h3>Contador de Caixas:</h3><p>Total: <b id='total'>0</b></p><p>Pequenas: <span id='qtdP'>0</span></p><p>Medias: <span id='qtdM'>0</span></p><p>Grandes: <span id='qtdG'>0</span></p></div>";
//   html += "<div class='card'><button class='btn btn-danger' onclick='dispararEmergenciaVirtual()'>EMERGENCIA VIRTUAL</button></div>";
//   html += "</body></html>";
//   server.send(200, "text/html", html);
// }

// void enviarDadosJSON() {
//   // Pega a chave do Mutex rapidinho, monta o JSON e devolve a chave
//   xSemaphoreTake(mutexDados, portMAX_DELAY);
//   String json = "{";
//   json += "\"estado\":\"" + obterNomeEstado(estadoAtual) + "\","; 
//   json += "\"temperatura\":" + String(temperaturaAtual) + ",";
//   json += "\"p\":" + String(quantidadeCaixasP) + ",";
//   json += "\"m\":" + String(quantidadeCaixasM) + ",";
//   json += "\"g\":" + String(quantidadeCaixasG);
//   json += "}";
//   xSemaphoreGive(mutexDados);
  
//   server.send(200, "application/json", json);
// }

// void tratarBotaoVirtualEmergencia() {
//   xSemaphoreTake(mutexDados, portMAX_DELAY);
//   estadoEmergencia = true;
//   xSemaphoreGive(mutexDados);
//   server.send(200, "text/plain", "Emergencia Ativada");
// }

// String obterNomeEstado(EstadosSistema estado) {
//   switch(estado) {
//     case AGUARDANDO_START: return "Aguardando Start";
//     case MANIPULADOR1_PEGA_CAIXA: return "Pegando Caixa";
//     case ESTEIRA_TRANSPORTANDO: return "Esteira Ligada";
//     case AGUARDANDO_FIM_ESTEIRA: return "Caixa em Curso";
//     case MANIPULADOR2_SEPARA_CAIXA: return "Separando Caixa";
//     case RESETANDO_MAQUINA: return "Resetando";
//     case EM_EMERGENCIA: return "!!! EMERGENCIA !!!";
//     default: return "Desconhecido";
//   }
// }




