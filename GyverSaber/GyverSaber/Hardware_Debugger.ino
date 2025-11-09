/*
 * ==================================================================================
 * === HARDWARE DEBUGGER PER GYVERSABER ESP32-S3                                  ===
 * ==================================================================================
 *
 * DESCRIZIONE:
 * Questo sketch esegue una serie di test su tutti i componenti hardware della
 * spada laser per verificare che siano collegati e funzionanti correttamente.
 *
 * ISTRUZIONI:
 * 1. Carica questo sketch sul tuo ESP32-S3.
 * 2. Apri il Monitor Seriale con un baud rate di 115200.
 * 3. Segui i messaggi stampati per diagnosticare eventuali problemi.
 *
 * OBIETTIVO:
 * Isolare e risolvere problemi legati a:
 * - Connessioni errate o saldature difettose.
 * - Componenti hardware non funzionanti.
 * - Incompatibilità delle librerie o configurazioni errate.
 *
 */

// ============================ LIBRERIE NECESSARIE ============================
#include <Arduino.h>
#include "Wire.h"
#include <SPI.h>
#include <SD.h>
#include "FastLED.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// ========================= DEFINIZIONE HARDWARE & PINOUT (ESP32-S3) =========================
// --- Striscia LED ---
#define LED_PIN 18
#define NUM_LEDS 30
#define BRIGHTNESS 150

// --- Pulsante di Controllo ---
#define BTN_PIN 4

// --- Altoparlante / Amplificatore ---
#define SPEAKER_PIN 17

// --- Comunicazione I2C (Sensore MPU6050) ---
#define MPU_SDA_PIN 8
#define MPU_SCL_PIN 9

// --- Comunicazione SPI (Modulo Scheda SD) ---
#define SD_CS_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SCK_PIN 12

// --- Misurazione Tensione Batteria ---
#define VOLT_PIN 1

// ============================ OGGETTI GLOBALI ============================
CRGB leds[NUM_LEDS];
Adafruit_MPU6050 mpu;
SPIClass spi = SPIClass(HSPI);
AudioGeneratorWAV *wav;
AudioFileSourceSD *file;
AudioOutputI2S *out;

// =================================== SETUP: TEST SEQUENZIALI ===================================
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("\n\n===== INIZIO DIAGNOSTICA HARDWARE GYVERSABER ESP32-S3 =====");

  // --- TEST 1: Sensore MPU6050 (Accelerometro/Giroscopio) ---
  Serial.println("\n[TEST 1] --- Sensore MPU6050 ---");
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  if (!mpu.begin()) {
    Serial.println("RISULTATO: ERRORE! Sensore MPU6050 non trovato.");
    Serial.println("  - Controlla i collegamenti I2C (SDA, SCL) e l'alimentazione del sensore.");
    while (1); // Blocca l'esecuzione per evidenziare il problema
  } else {
    Serial.println("RISULTATO: SUCCESSO! Sensore MPU6050 inizializzato correttamente.");
  }

  // --- TEST 2: Scheda SD ---
  Serial.println("\n[TEST 2] --- Scheda SD ---");
  spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, spi)) {
    Serial.println("RISULTATO: ERRORE! Montaggio della scheda SD fallito.");
    Serial.println("  - Controlla i collegamenti SPI (CS, MOSI, MISO, SCK).");
    Serial.println("  - Assicurati che la scheda SD sia inserita e formattata in FAT32.");
  } else {
    Serial.println("RISULTATO: SUCCESSO! Scheda SD montata correttamente.");
    Serial.print("  - Tipo di Scheda: ");
    switch (SD.cardType()) {
      case CARD_NONE: Serial.println("Nessuna"); break;
      case CARD_MMC: Serial.println("MMC"); break;
      case CARD_SD: Serial.println("SDSC"); break;
      case CARD_SDHC: Serial.println("SDHC"); break;
      default: Serial.println("Sconosciuto");
    }
    Serial.print("  - Dimensione: ");
    Serial.print(SD.cardSize() / (1024 * 1024));
    Serial.println(" MB");

    Serial.println("  - Elenco File nella Root:");
    File root = SD.open("/");
    while (true) {
      File entry = root.openNextFile();
      if (!entry) break;
      Serial.print("    ");
      Serial.println(entry.name());
      entry.close();
    }
    root.close();
  }

  // --- TEST 3: Striscia LED ---
  Serial.println("\n[TEST 3] --- Striscia LED ---");
  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  Serial.println("  - Test Colore ROSSO...");
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay(1000);

  Serial.println("  - Test Colore VERDE...");
  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  delay(1000);

  Serial.println("  - Test Colore BLU...");
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
  delay(1000);

  Serial.println("  - Test Arcobaleno...");
  fill_rainbow(leds, NUM_LEDS, 0, 7);
  FastLED.show();
  delay(1000);

  fill_solid(leds, NUM_LEDS, CRGB::Black); // Spegni i LED
  FastLED.show();
  Serial.println("RISULTATO: SUCCESSO! Test LED completato.");
  Serial.println("  - Se non hai visto i colori, controlla il collegamento del pin dati e l'alimentazione della striscia.");

  // --- TEST 4: Sistema Audio (I2S) ---
  Serial.println("\n[TEST 4] --- Sistema Audio ---");
  out = new AudioOutputI2S(0, 1);
  out->SetPinout(25, SPEAKER_PIN, 27); // BCLK, LRC, DOUT
  out->SetOutputModeMono(true);
  out->SetGain(2.0);

  const char* test_sound = "/ON.wav"; // Assicurati che questo file esista sulla SD
  file = new AudioFileSourceSD(test_sound);
  wav = new AudioGeneratorWAV();

  if (SD.exists(test_sound) && wav->begin(file, out)) {
    Serial.println("RISULTATO: SUCCESSO! Riproduzione del file di test in corso...");
    Serial.println("  - Se non senti l'audio, controlla i collegamenti dell'amplificatore e dell'altoparlante.");
    while (wav->isRunning()) {
      if (!wav->loop()) wav->stop();
    }
    Serial.println("  - Riproduzione completata.");
  } else {
    Serial.println("RISULTATO: ERRORE! Impossibile riprodurre il file audio.");
    Serial.println("  - Assicurati che il file 'ON.wav' esista sulla scheda SD.");
  }
  delete wav;
  delete file;

  Serial.println("\n===== DIAGNOSTICA INIZIALE COMPLETATA =====");
  Serial.println("===== INIZIO TEST IN TEMPO REALE =====");

  // --- Setup PIN per test continui ---
  pinMode(BTN_PIN, INPUT_PULLUP);
}

// =================================== LOOP: TEST CONTINUI ===================================
void loop() {
  // --- Test Continuo 1: Pulsante ---
  boolean btnState = !digitalRead(BTN_PIN);
  Serial.print("Stato Pulsante: ");
  Serial.print(btnState ? "PREMUTO" : "RILASCIATO");

  // --- Test Continuo 2: Dati MPU6050 ---
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  Serial.print("  |  Accel: X=");
  Serial.print(a.acceleration.x, 1);
  Serial.print(" Y=");
  Serial.print(a.acceleration.y, 1);
  Serial.print(" Z=");
  Serial.print(a.acceleration.z, 1);
  Serial.print(" m/s^2");
  Serial.print("  |  Gyro: X=");
  Serial.print(g.gyro.x, 1);
  Serial.print(" Y=");
  Serial.print(g.gyro.y, 1);
  Serial.print(" Z=");
  Serial.print(g.gyro.z, 1);
  Serial.print(" rad/s");

  // --- Test Continuo 3: Tensione Batteria ---
  int raw_adc = analogRead(VOLT_PIN);
  Serial.print("  |  ADC Batteria (raw): ");
  Serial.println(raw_adc);

  delay(100); // Rallenta l'output per renderlo leggibile
}
