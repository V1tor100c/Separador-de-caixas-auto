#include <Arduino.h>
#include <ESP32Servo.h>
#include <HCSR04.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

// Substitua pelo nome e senha do seu Wi-Fi se necessário
const char* ssid = "arthu";
const char* password = "12341234";

WebServer server(80);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int stepPin = 16;
const int dirPin = 4;
const int frequenciaMotor = 70;

const int iri = 14;
bool estadoIR = false;

const int pinbraco11 = 32;  // garra
const int pinbraco12 = 8;   // direita
const int pinbraco13 = 2;  // esquerda
const int pinbraco14 =  5;  // baixo 

Servo braco11, braco12, braco13, braco14;

// Definição dos pinos
const int trigPin = 17;
const int echoPin = 12;

const int trigPinFim = 2;
const int echoPinFim = 13;

const int botaoIniciar = 33;
const int botaoEmergencia = 34;

const int buzzer = 23; 
const int LM35 = 36; 

const int pinLedR = 19;
const int pinLedG = 18;
const int pinLedB = 5;
const int transistor = 26;

// --- VARIÁVEIS COMPARTILHADAS (Protegidas pelo Mutex) ---
int quantidadeCaixasP = 0;
int quantidadeCaixasM = 0;
int quantidadeCaixasG = 0;
char tamanhoCaixaMedida = '-';
volatile float temperaturaAtual = 0.0;
volatile bool estadoEmergencia = false;
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
// --------------------------------------------------------

int medidaSemCaixa = 9;
int medidaCaixaP = 6.0;
int medidaCaixaM = 5.1;
int medidaCaixaG = 3.5;
bool viACaixa = false;
char tamanho;
char tamanhoAtual;

volatile bool flagMedirTemperatura = false; 

// Criação do Mutex e do Timer
SemaphoreHandle_t mutexDados; 
hw_timer_t *timerTemperatura = NULL;

// Prototipação das funções
void loop0(void *parameter);
void loop1(void *parameter);
void ligar_esteira();
void voltar_esteira();
void desligar_esteira();
bool leituraFim();
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
void tratarBotaoVirtualIniciar();
String obterNomeEstado(EstadosSistema estado);

// 1ª INTERRUPÇÃO: Botão Físico (COM DEBOUNCE E ALTERNÂNCIA)
void IRAM_ATTR emergencia() {
  unsigned long tempoAtual = millis();
  if (tempoAtual - tempoUltimoClique > 300) {
    estadoEmergencia = !estadoEmergencia; 
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

  mutexDados = xSemaphoreCreateMutex();

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

  // Configuração das rotas do Servidor Web
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
  digitalWrite(stepPin, LOW);
  digitalWrite(dirPin, LOW);
  ledcSetup(0, frequenciaMotor, 8);
  ledcAttachPin(stepPin, 0);

  // CORREÇÃO: Pinos dos servos corrigidos e separados individualmente
  braco11.attach(pinbraco11, 500, 2400); 
  braco12.attach(pinbraco12, 500, 2400);
  braco13.attach(pinbraco13, 500, 2400);  
  braco14.attach(pinbraco14, 500, 2400);

  xTaskCreatePinnedToCore(loop0, "Task0", 10000, NULL, 1, NULL, 0); 
  xTaskCreatePinnedToCore(loop1, "Task1", 10000, NULL, 1, NULL, 1); 

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(trigPinFim, OUTPUT);
  pinMode(echoPinFim, INPUT);
}

void loop() {}

// CORE 0 - Controle e Sensores em Tempo Real
void loop0(void *parameter) {
  Serial.println("CORE 0 INICIADO!");
  while (true) {
    if (flagMedirTemperatura) {
      flagMedirTemperatura = false;
      int lecturaADC = analogRead(LM35);
      float temp = ((lecturaADC / 4095.0) * 3300.0) / 10.0;
      
      xSemaphoreTake(mutexDados, portMAX_DELAY);
      temperaturaAtual = temp;
      if (temperaturaAtual > 10.0) estadoEmergencia = true; 
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
      if (tempCheck < 9.0) {
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
        voltar_esteira();
        if (leituraFim() == true) {
          desligar_esteira();
          xSemaphoreTake(mutexDados, portMAX_DELAY);
          estadoAtual = MANIPULADOR2_SEPARA_CAIXA;
          xSemaphoreGive(mutexDados);
        }
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

// CORE 1 - Interface, display, RGB e WebServer
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

void ligar_esteira() { 
  digitalWrite(dirPin, HIGH); 
  ledcWrite(0, 128); 
}

void voltar_esteira() { 
  digitalWrite(dirPin, LOW); 
  ledcWrite(0, 128); 
}

void desligar_esteira() { 
  ledcWrite(0, 0); 
}

// CORREÇÃO: Adicionado timeout de 30ms para evitar resets por travamento
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

// CORREÇÃO: Adicionado timeout de 30ms e retorno seguro para ignorar erros
float medirDistancia() { 
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  
  if (duration == 0) {
    return 999.0; 
  }
  
  float distanceCm = duration * 0.034 / 2.0;
  return distanceCm;
}

void sensorDeCaixa() {
  if (viACaixa) return;

  // 1. PRIMEIRA LEITURA: O sensor sentiu alguma coisa? (A quina chegou)
  float distancia = medirDistancia();

  // Ignora erros ou ruídos colados no sensor
  if (distancia == -1 || distancia <= 1.0) {
    return;
  }

  // Se for maior que a medida vazia, não tem caixa
  if (distancia >= (medidaSemCaixa - 0.5)) {
    return;
  }

  // ==========================================
  // --- QUINA DA CAIXA DETECTADA ---
  // ==========================================
  Serial.println("Quina da caixa detectada. Aguardando alinhamento...");
  
  // 2. PAUSA ESTRATÉGICA: Espera a esteira trazer a caixa para o centro do sensor.
  // ATENÇÃO: Ajuste esse valor (ex: 300, 400, 500) conforme a velocidade da sua esteira!
  vTaskDelay(pdMS_TO_TICKS(350)); 

  // 3. SEGUNDA LEITURA: Agora medindo o centro (barriga) da caixa
  distancia = medirDistancia();

  // Refaz a verificação de segurança, vai que a caixa já passou ou foi alarme falso
  if (distancia == -1 || distancia >= (medidaSemCaixa - 0.5)) {
    Serial.println("Erro: A caixa escapou da leitura ou foi alarme falso.");
    return; 
  }

  // ==========================================
  // --- CLASSIFICAÇÃO COM DADO CONFIÁVEL ---
  // ==========================================
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
    return; // Medida inválida (menor que vazia, maior que P)
  }

  Serial.println(distancia); // Mostra a distância final estabilizada

  // Salva no Mutex
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

  // Trava para não ler a mesma caixa duas vezes
  viACaixa = true;

  Serial.print("Caixa detectada e salva: ");
  Serial.println(tipo);
}

// CORREÇÃO: Adicionado filtro de ruído (distancia <= 1.0) para não pular de estado sozinho
// void sensorDeCaixa() {
//   if (viACaixa) return;

//   float distancia = medirDistancia();

//   if (distancia == -1 || distancia <= 1.0) {
//     return;
//   }

//   if (distancia >= (medidaSemCaixa - 0.5)) {
//     return;
//   }

//   char tipo = '-';
//   if (distancia < medidaCaixaG) {        
//     tipo = 'G';
//     Serial.print("CAIXA GRANDE: ");
//     Serial.println(distancia);
//   } else if (distancia < medidaCaixaM) { 
//     tipo = 'M';
//     Serial.print("CAIXA MÉDIA: ");
//     Serial.println(distancia);
//   } else if (distancia < medidaCaixaP) { 
//     tipo = 'P';
//     Serial.print("CAIXA PEQUENA: ");  
//     Serial.println(distancia);
//   } else {
//     return;
//   }

//   xSemaphoreTake(mutexDados, portMAX_DELAY);
//   tamanhoCaixaMedida = tipo;
//   if (tipo == 'P') {
//     quantidadeCaixasP++;
//   } else if (tipo == 'M') {
//     quantidadeCaixasM++;
//   } else if (tipo == 'G') {
//     quantidadeCaixasG++;
//   }
//   xSemaphoreGive(mutexDados);

//   viACaixa = true;

//   Serial.print("Caixa detectada: ");
//   Serial.println(tipo);
// }

void pegarCaixa() {}

void colocarCaixaP() {}

void colocarCaixaM() {}

void colocarCaixaG() {}

void zeraTudo() {
  viACaixa = false;
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  tamanhoCaixaMedida = '-';
  xSemaphoreGive(mutexDados);
  braco11.write(27); braco12.write(25); braco13.write(33); braco14.write(32);
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

// CORREÇÃO: Adicionado botão HTML de "Iniciar Máquina" com estilização integrada
void enviarPaginaWeb() {
  // O comando R"rawliteral(...)rawliteral" permite colar blocos inteiros de texto sem precisar usar aspas e sinais de mais!
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
    
    /* Layout das estatísticas */
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
    // 1. Cria e configura o gráfico estilo "Rosquinha" (Doughnut)
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

    // 2. Fica pedindo os dados para o ESP32 a cada 1 segundo (1000ms)
    setInterval(function() {
      fetch('/dados').then(response => response.json()).then(data => {
        
        // Atualiza Status e Temperatura
        document.getElementById('status').innerText = data.estado;
        document.getElementById('temp').innerText = data.temperatura.toFixed(1);
        
        // Calcula o total
        let total = data.p + data.m + data.g;
        document.getElementById('total').innerText = total;
        
        // Calcula as porcentagens evitando dividir por zero
        let percP = total > 0 ? ((data.p / total) * 100).toFixed(1) : 0;
        let percM = total > 0 ? ((data.m / total) * 100).toFixed(1) : 0;
        let percG = total > 0 ? ((data.g / total) * 100).toFixed(1) : 0;
        
        // Joga as quantidades e porcentagens na tela
        document.getElementById('qtdP').innerText = data.p;
        document.getElementById('percP').innerText = percP + "%";
        document.getElementById('qtdM').innerText = data.m;
        document.getElementById('percM').innerText = percM + "%";
        document.getElementById('qtdG').innerText = data.g;
        document.getElementById('percG').innerText = percG + "%";
        
        // Atualiza a animação do Gráfico
        grafico.data.datasets[0].data = [data.p, data.m, data.g];
        grafico.update();

        // Lógica da tela de emergência piscante
        if(data.temperatura > 45.0 || data.estado.includes('EMERGENCIA')){
          document.getElementById('alerta').style.display = 'block';
        } else { 
          document.getElementById('alerta').style.display = 'none'; 
        }
      });
    }, 1000);
    
    // Funções dos botões virtuais
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

// CORREÇÃO: Função adicionada para tratar com segurança a inicialização via WEB
void tratarBotaoVirtualIniciar() {
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  if (estadoAtual == AGUARDANDO_START && !estadoEmergencia) {
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