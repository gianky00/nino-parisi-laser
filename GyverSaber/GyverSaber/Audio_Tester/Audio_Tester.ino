/*
 * ==================================================================================
 * === TESTER AUDIO I2S DI BASSO LIVELLO PER GYVERSABER                           ===
 * ==================================================================================
 *
 * DESCRIZIONE:
 * Questo sketch testa in modo isolato e diretto l'hardware audio (I2S,
 * amplificatore, altoparlante) generando un'onda sinusoidale pura (un tono
 * costante) senza utilizzare la scheda SD o la decodifica di file WAV.
 *
 * ISTRUZIONI:
 * 1. Carica questo sketch sul tuo ESP32-S3.
 * 2. Dopo il caricamento, dovresti sentire immediatamente un tono pulito e
 *    costante dall'altoparlante.
 * 3. Apri il Monitor Seriale (baud rate 115200) per messaggi di stato.
 *
 * DIAGNOSI:
 * - Se senti il tono: L'hardware audio (I2S, amplificatore, altoparlante)
 *   e i relativi collegamenti sono PERFETTAMENTE FUNZIONANTI. Il problema
 *   audio nel progetto principale è probabilmente legato alla scheda SD, ai
 *   file o alla libreria di decodifica.
 * - Se NON senti il tono: C'è un problema hardware. Controlla attentamente:
 *   - I collegamenti dei 3 pin I2S (definiti sotto).
 *   - L'alimentazione dell'amplificatore.
 *   - Il collegamento dell'altoparlante all'amplificatore.
 *   - Il pinout I2S (BCLK, LRC, DOUT) che potrebbe essere diverso per il
 *     tuo modulo specifico.
 */

#include <Arduino.h>
#include "driver/i2s.h"

// ========================= DEFINIZIONE PINOUT I2S =========================
// Questi pin sono quelli usati dalla libreria ESP8266Audio nel progetto principale.
#define I2S_BCLK_PIN      26 // Bit Clock (PIN CORRETTO PER ESP32-S3)
#define I2S_LRC_PIN       17 // Left/Right Clock (corrisponde a SPEAKER_PIN nel progetto originale)
#define I2S_DOUT_PIN      27 // Data Out

// ========================= PARAMETRI DEL SEGNALE AUDIO =========================
#define SAMPLE_RATE       44100 // Frequenza di campionamento in Hz
#define TONE_FREQUENCY    440   // Frequenza del tono da generare (440 Hz = La)
#define AMPLITUDE         8000  // Ampiezza del segnale (volume)

// Buffer per i campioni audio
int16_t samples[1024];

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n===== Inizio Test Audio I2S di Basso Livello =====");

  // --- Configurazione I2S ---
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false,
      .tx_desc_auto_clear = true
  };

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_BCLK_PIN,
      .ws_io_num = I2S_LRC_PIN,
      .data_out_num = I2S_DOUT_PIN,
      .data_in_num = I2S_PIN_NO_CHANGE
  };

  // --- Inizializzazione I2S ---
  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Errore installazione driver I2S: %d\n", err);
    while (1);
  }

  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Errore impostazione pin I2S: %d\n", err);
    while (1);
  }

  Serial.println("Driver I2S installato e configurato correttamente.");
  Serial.println("Generazione di un tono a 440 Hz in corso...");
  Serial.println("Dovresti sentire un suono costante dall'altoparlante.");

  // --- Generazione dell'onda sinusoidale nel buffer ---
  // Calcoliamo una sinusoide completa e la mettiamo nel buffer.
  // Questo buffer verrà poi ripetuto in continuazione.
  double angle_step = (2 * PI * TONE_FREQUENCY) / SAMPLE_RATE;
  for (int i = 0; i < 1024; i++) {
    samples[i] = (int16_t)(AMPLITUDE * sin(i * angle_step));
  }
}

void loop() {
  size_t bytes_written = 0;
  // Scrive continuamente il buffer di campioni all'output I2S
  i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
  if (bytes_written != sizeof(samples)) {
    Serial.println("Errore di scrittura I2S!");
  }
}
