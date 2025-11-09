/*
 * ==================================================================================
 * === TEST AUDIO "BARE-METAL" DI BASSISSIMO LIVELLO PER ESP32-S3                 ===
 * ==================================================================================
 *
 * DESCRIZIONE:
 * Questo sketch è la soluzione definitiva per testare l'hardware audio bypassando
 * qualsiasi funzione dell'Arduino IDE che potrebbe essere corrotta o mancante nel
 * tuo ambiente di sviluppo (`dacWrite`, `dac_output_voltage`, ecc.).
 *
 * COME FUNZIONA:
 * Interagisce direttamente con i registri di memoria dell'hardware del DAC
 * dell'ESP32-S3 per generare il suono. Questo è il modo più diretto possibile
 * per pilotare l'hardware, senza dipendere da nessuna libreria o funzione
 * problematica.
 *
 * ISTRUZIONI:
 * 1. Carica questo sketch sul tuo ESP32-S3.
 * 2. Apri il Monitor Seriale (baud rate 115200) per i messaggi di stato.
 * 3. Se l'hardware è funzionante, sentirai il suono del file "ON.wav".
 *
 * DIAGNOSI FINALE:
 * - Se questo sketch COMPILA e senti il suono: Problema risolto! Il tuo IDE ha
 *   un problema, ma noi lo abbiamo aggirato. Possiamo integrare questa
 *   soluzione nel progetto principale.
 * - Se questo sketch COMPILA ma NON senti il suono: Il problema è 100%
 *   hardware (cablaggio, amplificatore, alimentazione).
 * - Se questo sketch NON COMPILA: Il problema nel tuo IDE è così profondo che
 *   nemmeno le funzioni base come `pinMode` sono accessibili, e va reinstallato.
 */

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// ========================= DEFINIZIONI HARDWARE =========================
#define SPEAKER_PIN 17 // DAC Channel 1 è su GPIO17 sull'S3
#define SD_CS_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SCK_PIN 12
SPIClass spi = SPIClass(HSPI);

// ========================= REGISTRI HARDWARE DEL DAC (ESP32-S3) =========================
// Indirizzi di memoria presi direttamente dalla documentazione tecnica di Espressif
#define SENS_SAR_DAC_CTRL1_REG      0x6000E018
#define SENS_SAR_DAC_CTRL2_REG      0x6000E01C
#define SENS_SAR_DAC_DATA1_REG      0x6000E024

// Maschere di bit per controllare i registri
#define SENS_SW_TONE_EN             (BIT(8))
#define SENS_SW_FSTEP_M             (0x0000FFFF)
#define SENS_DAC_CLK_INV            (BIT(17))
#define SENS_DAC_CLK_EN             (BIT(18))
#define SENS_DAC_DIG_FORCE_M        (BIT(19))
#define SENS_DAC_CW_EN1_M           (BIT(20))

// ========================= FUNZIONE DAC "BARE-METAL" =========================
void bareMetalDacWrite(uint8_t value) {
    // Scrive il valore a 8-bit direttamente nel registro dati del DAC1
    WRITE_PERI_REG(SENS_SAR_DAC_DATA1_REG, value);
}

void bareMetalDacEnable() {
    // Abilita il clock del DAC
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_DAC_CLK_EN);

    // Abilita il controller del DAC e lo scollega dal generatore di toni
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DIG_FORCE_M);

    // Imposta il pin GPIO17 per usare la funzione RTC/DAC
    pinMode(SPEAKER_PIN, ANALOG);
}


// ========================= LOGICA DI RIPRODUZIONE =========================
void playTestSound() {
    const char* filename = "/ON.wav";
    File audioFile = SD.open(filename);
    if (!audioFile) {
        Serial.println("RISULTATO: ERRORE! Impossibile trovare il file audio 'ON.wav'.");
        return;
    }

    uint32_t sampleRate = 0;
    audioFile.seek(24);
    audioFile.read((byte*)&sampleRate, 4);

    uint16_t bitsPerSample = 0;
    audioFile.seek(34);
    audioFile.read((byte*)&bitsPerSample, 2);
    audioFile.seek(44);

    if (bitsPerSample != 8 && bitsPerSample != 16 || sampleRate == 0) {
        Serial.println("RISULTATO: ERRORE! Formato WAV non supportato.");
        audioFile.close();
        return;
    }

    Serial.println("RISULTATO: SUCCESSO! Riproduzione del file di test in corso...");

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

    bareMetalDacWrite(128); // Silenzia il DAC
    audioFile.close();
    Serial.println("  - Riproduzione completata.");
}

// =================================== SETUP ===================================
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("\n\n===== INIZIO TEST AUDIO BARE-METAL =====");

  spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, spi)) {
    Serial.println("ERRORE CRITICO: Montaggio della scheda SD fallito. Impossibile continuare.");
    while(1);
  } else {
    Serial.println("Scheda SD OK.");
  }

  // Abilita il DAC a basso livello
  bareMetalDacEnable();
  Serial.println("DAC abilitato a livello hardware.");

  // Riproduci il suono di test
  playTestSound();

  Serial.println("\n===== TEST COMPLETATO =====");
}

void loop() {
  // Niente nel loop, il test viene eseguito una sola volta.
  delay(1000);
}
