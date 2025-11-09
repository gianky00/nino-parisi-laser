/*
 * ==================================================================================
 * === HARDWARE DEBUGGER PER GYVERSABER ESP32-S3 (V3 - DAC Audio)                 ===
 * ==================================================================================
 *
 * DESCRIZIONE:
 * Questo sketch esegue una serie di test su tutti i componenti hardware della
 * spada laser. Utilizza una logica audio custom basata sul DAC interno per
 * la massima compatibilità e robustezza.
 *
 * ISTRUZIONI (MODALITÀ INTERATTIVA):
 * 1. Carica questo sketch sul tuo ESP32-S3.
 * 2. Apri il Monitor Seriale con un baud rate di 115200.
 * 3. Invia qualsiasi carattere o premi "Invio" per procedere tra i test.
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
#include "driver/dac.h" // Libreria per il DAC

// ========================= DEFINIZIONE HARDWARE & PINOUT (ESP32-S3) =========================
#define LED_PIN 18
#define NUM_LEDS 30
#define BRIGHTNESS 150
#define BTN_PIN 4
#define SPEAKER_PIN 17
#define MPU_SDA_PIN 8
#define MPU_SCL_PIN 9
#define SD_CS_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SCK_PIN 12
#define VOLT_PIN 1

// ============================ OGGETTI GLOBALI ============================
CRGB leds[NUM_LEDS];
Adafruit_MPU6050 mpu;
SPIClass spi = SPIClass(HSPI);

// =========================== FUNZIONI DI SUPPORTO ===========================
void waitForUserInput() {
  Serial.println("\n-------------------------------------------------");
  Serial.println(">>> Premi Invio o invia un carattere per continuare...");
  Serial.println("-------------------------------------------------");
  while (Serial.available() == 0) { delay(100); }
  while (Serial.available() > 0) { Serial.read(); }
}

void playTestSound() {
    const char* filename = "/ON.wav";
    File audioFile = SD.open(filename);
    if (!audioFile) {
        Serial.println("RISULTATO: ERRORE! Impossibile trovare il file audio 'ON.wav' sulla scheda SD.");
        return;
    }

    uint32_t sampleRate = 0;
    audioFile.seek(24);
    audioFile.read((byte*)&sampleRate, 4);

    uint16_t bitsPerSample = 0;
    audioFile.seek(34);
    audioFile.read((byte*)&bitsPerSample, 2);

    audioFile.seek(44);

    if (bitsPerSample != 8 && bitsPerSample != 16) {
        Serial.println("RISULTATO: ERRORE! Profondità di bit non supportata (solo 8 o 16 bit).");
        audioFile.close();
        return;
    }

    Serial.println("RISULTATO: SUCCESSO! Riproduzione del file di test in corso...");
    Serial.println("  - Se non senti l'audio, controlla i collegamenti dell'amplificatore e dell'altoparlante.");

    uint32_t delay_us = 1000000 / sampleRate;
    uint8_t buffer[512];
    int bytesRead;

    while ((bytesRead = audioFile.read(buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytesRead; i++) {
            uint8_t sample = (bitsPerSample == 8) ? buffer[i] : ((int16_t)(buffer[i+1] << 8 | buffer[i]) >> 8) + 128;
            dac_output_voltage(DAC_CHANNEL_1, sample);
            delayMicroseconds(delay_us);
            if (bitsPerSample == 16) i++;
        }
    }

    dac_output_voltage(DAC_CHANNEL_1, 128); // Silenzia il DAC
    audioFile.close();
    Serial.println("  - Riproduzione completata.");
}


// =================================== SETUP: TEST SEQUENZIALI ===================================
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("\n\n===== INIZIO DIAGNOSTICA HARDWARE GYVERSABER (V3 - DAC Audio) =====");

  // --- TEST 1: Sensore MPU6050 ---
  Serial.println("\n[TEST 1] --- Sensore MPU6050 ---");
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  if (!mpu.begin()) {
    Serial.println("RISULTATO: ERRORE! Sensore MPU6050 non trovato.");
    Serial.println("  - Controlla i collegamenti I2C (SDA, SCL) e l'alimentazione del sensore.");
  } else {
    Serial.println("RISULTATO: SUCCESSO! Sensore MPU6050 inizializzato correttamente.");
  }
  waitForUserInput();

  // --- TEST 2: Scheda SD ---
  Serial.println("\n[TEST 2] --- Scheda SD ---");
  spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, spi)) {
    Serial.println("RISULTATO: ERRORE! Montaggio della scheda SD fallito.");
  } else {
    Serial.println("RISULTATO: SUCCESSO! Scheda SD montata correttamente.");
  }
  waitForUserInput();

  // --- TEST 3: Striscia LED ---
  Serial.println("\n[TEST 3] --- Striscia LED ---");
  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  Serial.println("  - Test Colore ROSSO..."); fill_solid(leds, NUM_LEDS, CRGB::Red); FastLED.show(); delay(1000);
  Serial.println("  - Test Colore VERDE..."); fill_solid(leds, NUM_LEDS, CRGB::Green); FastLED.show(); delay(1000);
  Serial.println("  - Test Colore BLU..."); fill_solid(leds, NUM_LEDS, CRGB::Blue); FastLED.show(); delay(1000);
  fill_solid(leds, NUM_LEDS, CRGB::Black); FastLED.show();
  Serial.println("RISULTATO: SUCCESSO! Test LED completato.");
  waitForUserInput();

  // --- TEST 4: Sistema Audio (DAC) ---
  Serial.println("\n[TEST 4] --- Sistema Audio (DAC) ---");
  dac_output_enable(DAC_CHANNEL_1);
  playTestSound();

  Serial.println("\n===== DIAGNOSTICA INIZIALE COMPLETATA =====");
  Serial.println("===== INIZIO TEST IN TEMPO REALE =====");
  pinMode(BTN_PIN, INPUT_PULLUP);
}

// =================================== LOOP: TEST CONTINUI ===================================
void loop() {
  boolean btnState = !digitalRead(BTN_PIN);
  Serial.print("Stato Pulsante: ");
  Serial.print(btnState ? "PREMUTO" : "RILASCIATO");

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  Serial.print("  |  Accel: X="); Serial.print(a.acceleration.x, 1);
  Serial.print(" Y="); Serial.print(a.acceleration.y, 1);
  Serial.print(" Z="); Serial.print(a.acceleration.z, 1);

  int raw_adc = analogRead(VOLT_PIN);
  Serial.print("  |  ADC Batteria (raw): ");
  Serial.println(raw_adc);

  delay(100);
}
