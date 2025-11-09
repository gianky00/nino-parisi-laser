/*
 * ==================================================================================
 * === HARDWARE DEBUGGER PER GYVERSABER (V4 - BARE-METAL AUDIO)                   ===
 * ==================================================================================
 *
 * DESCRIZIONE:
 * Questa è la versione definitiva del debugger. Utilizza una logica audio
 * "bare-metal" che interagisce direttamente con l'hardware del DAC per la
 * massima compatibilità, bypassando i problemi dell'ambiente di compilazione.
 *
 * ISTRUZIONI:
 * 1. Carica questo sketch sul tuo ESP32-S3.
 * 2. Apri il Monitor Seriale con un baud rate di 115200.
 * 3. Segui le istruzioni e premi Invio per avanzare tra i test.
 */

// ============================ LIBRERIE ============================
#include <Arduino.h>
#include "Wire.h"
#include <SPI.h>
#include <SD.h>
#include "FastLED.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ========================= REGISTRI HARDWARE DAC =========================
#define SENS_SAR_DAC_CTRL1_REG      0x6000E018
#define SENS_SAR_DAC_CTRL2_REG      0x6000E01C
#define SENS_SAR_DAC_DATA1_REG      0x6000E024
#define SENS_DAC_CLK_EN             (BIT(18))
#define SENS_DAC_CW_EN1_M           (BIT(20))
#define SENS_DAC_DIG_FORCE_M        (BIT(19))

// ========================= DEFINIZIONI HARDWARE =========================
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

CRGB leds[NUM_LEDS];
Adafruit_MPU6050 mpu;
SPIClass spi = SPIClass(HSPI);

// ========================= FUNZIONI BARE-METAL =========================
void bareMetalDacWrite(uint8_t value) { WRITE_PERI_REG(SENS_SAR_DAC_DATA1_REG, value); }
void bareMetalDacEnable() {
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_DAC_CLK_EN);
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DIG_FORCE_M);
    pinMode(SPEAKER_PIN, ANALOG);
}

// ========================= FUNZIONI DI TEST =========================
void waitForUserInput() {
  Serial.println("\n-------------------------------------------------");
  Serial.println(">>> Premi Invio per continuare...");
  while (Serial.available() == 0) { delay(100); }
  while (Serial.available() > 0) { Serial.read(); }
}

void playTestSound() {
    const char* filename = "/ON.wav";
    File audioFile = SD.open(filename);
    if (!audioFile) {
        Serial.println("RISULTATO: ERRORE! Impossibile trovare 'ON.wav'.");
        return;
    }

    uint32_t sampleRate = 0;
    audioFile.seek(24);
    audioFile.read((byte*)&sampleRate, 4);
    uint16_t bitsPerSample = 0;
    audioFile.seek(34);
    audioFile.read((byte*)&bitsPerSample, 2);
    audioFile.seek(44);

    if ((bitsPerSample != 8 && bitsPerSample != 16) || sampleRate == 0) {
        Serial.println("RISULTATO: ERRORE! Formato WAV non supportato.");
        audioFile.close();
        return;
    }

    Serial.println("RISULTATO: SUCCESSO! Riproduzione in corso...");
    uint32_t delay_us = 1000000 / sampleRate;
    uint8_t buffer[512];
    int bytesRead;

    while ((bytesRead = audioFile.read(buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytesRead; i++) {
            uint8_t sample = (bitsPerSample == 8) ? buffer[i] : ((int16_t)(buffer[i+1] << 8 | buffer[i]) >> 8) + 128;
            bareMetalDacWrite(sample);
            delayMicroseconds(delay_us);
            if (bitsPerSample == 16) i++;
        }
    }
    bareMetalDacWrite(128);
    audioFile.close();
    Serial.println("  - Riproduzione completata.");
}

// =================================== SETUP ===================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n===== INIZIO DIAGNOSTICA HARDWARE (Bare-Metal Audio) =====");

  // --- TEST MPU6050 ---
  Serial.println("\n[TEST 1] --- Sensore MPU6050 ---");
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  if (!mpu.begin()) Serial.println("RISULTATO: ERRORE! MPU6050 non trovato.");
  else Serial.println("RISULTATO: SUCCESSO! MPU6050 trovato.");
  waitForUserInput();

  // --- TEST SD Card ---
  Serial.println("\n[TEST 2] --- Scheda SD ---");
  spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, spi)) Serial.println("RISULTATO: ERRORE! Montaggio SD fallito.");
  else Serial.println("RISULTATO: SUCCESSO! Scheda SD montata.");
  waitForUserInput();

  // --- TEST LED ---
  Serial.println("\n[TEST 3] --- Striscia LED ---");
  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS);
  fill_solid(leds, NUM_LEDS, CRGB::Red); FastLED.show(); delay(500);
  fill_solid(leds, NUM_LEDS, CRGB::Green); FastLED.show(); delay(500);
  fill_solid(leds, NUM_LEDS, CRGB::Blue); FastLED.show(); delay(500);
  fill_solid(leds, NUM_LEDS, CRGB::Black); FastLED.show();
  Serial.println("RISULTATO: Test LED completato.");
  waitForUserInput();

  // --- TEST AUDIO ---
  Serial.println("\n[TEST 4] --- Sistema Audio (Bare-Metal) ---");
  bareMetalDacEnable();
  playTestSound();

  Serial.println("\n===== DIAGNOSTICA INIZIALE COMPLETATA =====");
  pinMode(BTN_PIN, INPUT_PULLUP);
}

// =================================== LOOP ===================================
void loop() {
  // Lasciato vuoto, tutti i test sono nel setup.
  delay(1000);
}
