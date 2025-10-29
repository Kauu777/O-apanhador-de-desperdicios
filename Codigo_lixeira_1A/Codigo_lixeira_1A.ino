/*
 * Lixeira Inteligente com ESP32 e ThingSpeak
 * * Versão final com 9 LEDs e conexão à rede Eduroam (WPA2-Enterprise).
 * * Envia dados em 3 intervalos de tempo configuráveis.
 * Data: 18/09/2025
 */
#include <WiFi.h>
#include "ThingSpeak.h"
#include "time.h"
#include "esp_wpa2.h" // Biblioteca para conexão WPA2-Enterprise (Eduroam)

// =================================================================
// ★★★ CONFIGURAÇÕES DE REDE EDUROAM - PREENCHA COM SEUS DADOS ★★★
const char* ssid = "eduroam";
const char* eap_identity = "248705@unicamp.br"; // Ex: 123456@ufabc.edu.br
const char* eap_username = "248705@unicamp.br"; // Geralmente o mesmo que a identidade ou apenas o usuário
const char* eap_password = "Qu31r0$17"; // A mesma senha do portal do aluno, etc.
// =================================================================

// --- CONFIGURAÇÕES DO THINGSPEAK ---
unsigned long myChannelNumber = 3051927;
const char * myWriteAPIKey = "AGZXL5IVH8B9WE39";
WiFiClient client;

// --- PINOS DO ESP32 ---
const int PINO_TRIG = 32;
const int PINO_ECHO = 33;
// Pinos para os 9 LEDs. Monte-os na ordem que preferir.
// Sugestão de montagem física (do primeiro ao último pino): 4 Verdes, 3 Amarelos, 2 Vermelhos
const int pinosLeds[] = {25, 26, 27, 14, 12, 13, 15, 2, 4};
const int NUM_LEDS = 9;

// --- PARÂMETROS FÍSICOS DA LIXEIRA ---
const float ALTURA_LIXEIRA_CM = 50.0;
const float DISTANCIA_MINIMA_CM = 4.0;

// --- CONTROLE DE TEMPO E HORÁRIO ---
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3 * 3600; // Fuso de Brasília (UTC-3)
const int   daylightOffset_sec = 0;    // Sem horário de verão

// --- DEFINIÇÃO DOS INTERVALOS DE ENVIO ---
const int HORA_INICIO_1 = 7, HORA_FIM_1 = 9;   // Intervalo 1 (das 7:00 às 8:59)
const int HORA_INICIO_2 = 12, HORA_FIM_2 = 14;  // Intervalo 2 (das 12:00 às 13:59)
const int HORA_INICIO_3 = 16, HORA_FIM_3 = 18;  // Intervalo 3 (das 16:00 às 17:59)

const unsigned long INTERVALO_LEITURA_MS = 5000; // Lê o sensor a cada 5 segundos
const unsigned long INTERVALO_ENVIO_THINGSPEAK_MS = 60000; // Envia dados a cada 60 segundos (se estiver no horário)

const unsigned long PERIODO_24H_MS = 24UL * 60 * 60 * 1000;
unsigned long inicioPeriodo24h = 0, ultimoEnvioThingSpeak = 0, ultimaLeitura = 0;
double somaDistancias = 0.0;
long contadorLeituras = 0;

void conectarWifi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.disconnect(true);

  WiFi.mode(WIFI_STA);
  esp_wifi_sta_wpa2_ent_enable();
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)eap_identity, strlen(eap_identity));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)eap_username, strlen(eap_username));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)eap_password, strlen(eap_password));
  
  WiFi.begin(ssid);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");
    Serial.print("Endereco IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar ao WiFi. Verifique suas credenciais Eduroam. Reiniciando...");
    delay(10000);
    ESP.restart();
  }
}

void configurarTempo() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Configurando a hora...");
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Falha ao obter a hora do servidor NTP.");
    return;
  }
  Serial.print("Hora atual configurada: ");
  Serial.println(&timeinfo, "%A, %d de %B de %Y %H:%M:%S");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Lixeira Inteligente com ESP32...");
  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);
  for (int i = 0; i < NUM_LEDS; i++) pinMode(pinosLeds[i], OUTPUT);
  conectarWifi();
  configurarTempo();
  ThingSpeak.begin(client);
  inicioPeriodo24h = millis();
  testarLeds();
}

void loop() {
  static float ultimoNivelPercent = 0;
  if (WiFi.status() != WL_CONNECTED) conectarWifi();

  if (millis() - ultimaLeitura >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = millis();
    float distanciaCm = medirDistancia();
    ultimoNivelPercent = map(distanciaCm, DISTANCIA_MINIMA_CM, ALTURA_LIXEIRA_CM, 100, 0);
    ultimoNivelPercent = constrain(ultimoNivelPercent, 0, 100);
    atualizarLeds(ultimoNivelPercent);
    if (distanciaCm > 0 && distanciaCm <= ALTURA_LIXEIRA_CM) {
      somaDistancias += distanciaCm;
      contadorLeituras++;
    }
  }

  if (millis() - ultimoEnvioThingSpeak >= INTERVALO_ENVIO_THINGSPEAK_MS) {
    ultimoEnvioThingSpeak = millis();
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)){
      bool dentroDoHorario = (timeinfo.tm_hour >= HORA_INICIO_1 && timeinfo.tm_hour < HORA_FIM_1) ||
                             (timeinfo.tm_hour >= HORA_INICIO_2 && timeinfo.tm_hour < HORA_FIM_2) ||
                             (timeinfo.tm_hour >= HORA_INICIO_3 && timeinfo.tm_hour < HORA_FIM_3);
      if(dentroDoHorario) {
        enviarDadosThingSpeak(ultimoNivelPercent, -1);
      }
    }
  }

  if (millis() - inicioPeriodo24h >= PERIODO_24H_MS) {
    calcularEEnviarMediaFinal();
  }
}

float medirDistancia() {
  digitalWrite(PINO_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PINO_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PINO_TRIG, LOW);
  long duracao = pulseIn(PINO_ECHO, HIGH, 50000); 
  float distanciaCm = (duracao * 0.0343) / 2.0;
  if (distanciaCm > ALTURA_LIXEIRA_CM || distanciaCm <= 0) return ALTURA_LIXEIRA_CM;
  return distanciaCm;
}

void atualizarLeds(float nivel) {
  int ledsParaAcender = map(nivel, 0, 100, 0, NUM_LEDS);
  Serial.print("Nivel: "); Serial.print(nivel); Serial.print("% | LEDs: "); Serial.println(ledsParaAcender);
  for (int i = 0; i < NUM_LEDS; i++) digitalWrite(pinosLeds[i], (i < ledsParaAcender) ? HIGH : LOW);
}

void enviarDadosThingSpeak(float nivelAtual, float mediaFinal) {
  Serial.println("Enviando dados para o ThingSpeak...");
  ThingSpeak.setField(1, nivelAtual);
  if (mediaFinal >= 0) ThingSpeak.setField(2, mediaFinal);
  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (httpCode == 200) Serial.println("Dados enviados com sucesso.");
  else Serial.println("Erro ao enviar dados. Codigo: " + String(httpCode));
}

void calcularEEnviarMediaFinal() {
  Serial.println("--- FIM DO PERIODO DE 24 HORAS ---");
  float mediaDiaria = -1;
  if (contadorLeituras > 0) mediaDiaria = somaDistancias / contadorLeituras;
  float nivelAtual = map(medirDistancia(), DISTANCIA_MINIMA_CM, ALTURA_LIXEIRA_CM, 100, 0);
  enviarDadosThingSpeak(constrain(nivelAtual, 0, 100), mediaDiaria);
  somaDistancias = 0.0;
  contadorLeituras = 0;
  inicioPeriodo24h = millis();
}

void testarLeds() {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < NUM_LEDS; j++) digitalWrite(pinosLeds[j], HIGH);
    delay(250);
    for (int j = g = 0; j < NUM_LEDS; j++) digitalWrite(pinosLeds[j], LOW);
    delay(250);
  }
}
