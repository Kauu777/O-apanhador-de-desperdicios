/*
 * Lixeira Inteligente com ESP32 - Versão Offline
 * * Este código mede o nível de lixo e exibe em 9 LEDs, sem depender de Wi-Fi.
 * Data: 18/09/2025
 */

// --- PINOS DO ESP32 ---
const int PINO_TRIG = 32;
const int PINO_ECHO = 33;
// Pinos para os 9 LEDs. Monte-os na ordem que preferir.
const int pinosLeds[] = {25, 26, 27, 14, 12, 13, 15, 2, 4};
const int NUM_LEDS = 9;

// --- PARÂMETROS FÍSICOS DA LIXEIRA ---
const float ALTURA_LIXEIRA_CM = 50.0;
const float DISTANCIA_MINIMA_CM = 1;

// --- CONTROLE DE TEMPO ---
// Intervalo para LER O SENSOR E ATUALIZAR OS LEDS (em milissegundos)
const unsigned long INTERVALO_LEITURA_MS = 5000; // Lê o sensor a cada 5 segundos
unsigned long ultimaLeitura = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Lixeira Inteligente com ESP32 (Modo Offline)...");

  // Configura pinos
  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(pinosLeds[i], OUTPUT);
  }

  testarLeds();
}

void loop() {
  // Bloco de Leitura do Sensor (controlado por INTERVALO_LEITURA_MS)
  if (millis() - ultimaLeitura >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = millis(); // Atualiza o tempo da última leitura

    // Medir a distância
    float distanciaCm = medirDistancia();

    // Calcular o nível de preenchimento em porcentagem
    float nivelPercent = map(distanciaCm, DISTANCIA_MINIMA_CM, ALTURA_LIXEIRA_CM, 100, 0);
    nivelPercent = constrain(nivelPercent, 0, 100);

    // Atualizar os LEDs
    atualizarLeds(nivelPercent);
  }
}

float medirDistancia() {
  digitalWrite(PINO_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PINO_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PINO_TRIG, LOW);
  long duracao = pulseIn(PINO_ECHO, HIGH, 50000); // Timeout de 50ms para evitar travamentos
  float distanciaCm = (duracao * 0.0343) / 2.0;
  if (distanciaCm > ALTURA_LIXEIRA_CM || distanciaCm <= 0) {
    return ALTURA_LIXEIRA_CM;
  }
  return distanciaCm;
}

void atualizarLeds(float nivel) {
  int ledsParaAcender = map(nivel, 0, 100, 0, NUM_LEDS);
  Serial.print("Nivel: "); 
  Serial.print(nivel); 
  Serial.print("% | LEDs: "); 
  Serial.println(ledsParaAcender);
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(pinosLeds[i], (i < ledsParaAcender) ? HIGH : LOW);
  }
}

void testarLeds() {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < NUM_LEDS; j++) digitalWrite(pinosLeds[j], HIGH);
    delay(250);
    for (int j = 0; j < NUM_LEDS; j++) digitalWrite(pinosLeds[j], LOW);
    delay(250);
  }
}

