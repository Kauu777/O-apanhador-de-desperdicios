/*
 * Teste de LEDs para ESP32
 *
 * Este código não usa o sensor. Ele apenas liga
 * todos os 9 LEDs permanentemente para
 * verificar se a fiação está correta.
 */

// --- PINOS DO ESP32 ---
// Array com os 9 pinos dos LEDs
const int pinosLeds[] = {25, 26, 27, 14, 12, 13, 15, 2, 4};
const int NUM_LEDS = 9;

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Teste de LEDs...");

  // Configura todos os pinos dos LEDs como saída
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(pinosLeds[i], OUTPUT);
  }

  Serial.println("Acendendo todos os LEDs permanentemente...");
  // Acende todos os 9 LEDs de uma vez
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(pinosLeds[i], HIGH);
  }
}

void loop() {
  // Não faz nada.
  // O estado dos pinos (HIGH) definido no setup() será mantido,
  // deixando os LEDs permanentemente acesos.
  delay(1000); // Um pequeno delay para estabilidade, opcional.
}

