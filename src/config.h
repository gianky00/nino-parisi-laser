/*
 * ==================================================================================
 * ===                        CONFIGURAZIONE GYVERSABER ESP32                       ===
 * ==================================================================================
 *
 * In questo file puoi personalizzare tutte le impostazioni della tua spada laser.
 * Modifica i pin in base al tuo cablaggio e regola i parametri per ottenere
 * il comportamento desiderato.
 */

#pragma once // Evita inclusioni multiple dello stesso file

// ========================= DEFINIZIONE HARDWARE & PINOUT (ESP32-S3) =========================
// --- Striscia LED ---
#define LED_PIN 18      // Pin a cui è collegata la striscia LED
#define NUM_LEDS 30     // Numero di LED (o microcircuiti WS2811)
#define BRIGHTNESS 255  // Luminosità massima (0-255)

// --- Pulsante di Controllo ---
#define BTN_PIN 4       // Pin del pulsante

// --- Altoparlante / Amplificatore ---
#define SPEAKER_PIN 17  // Pin a cui è collegato l'altoparlante

// --- Comunicazione I2C (Sensore MPU6050) ---
#define MPU_SDA_PIN 8   // Pin SDA per il sensore
#define MPU_SCL_PIN 9   // Pin SCL per il sensore

// --- Comunicazione SPI (Modulo Scheda SD) ---
#define SD_CS_PIN 10    // Pin Chip Select (CS) per la scheda SD
#define SPI_MOSI_PIN 11 // Pin MOSI
#define SPI_MISO_PIN 13 // Pin MISO
#define SPI_SCK_PIN 12  // Pin SCK

// --- Misurazione Tensione Batteria ---
#define VOLT_PIN 1      // Pin per il partitore di tensione
#define R1 100000.0f    // Valore del resistore R1 (in Ohm)
#define R2 33000.0f     // Valore del resistore R2 (in Ohm) - NON SUPERARE 33k per la sicurezza dell'ESP32

// ============================ IMPOSTAZIONI DEL PROGETTO ============================
// --- Comportamento Pulsante ---
#define BTN_TIMEOUT 800     // Tempo (ms) per registrare una pressione prolungata

// --- Sensibilità del Movimento (da ricalibrare in base al tuo sensore) ---
#define SWING_TIMEOUT 500   // Tempo (ms) di attesa tra un suono di swing e l'altro
#define SWING_L_THR 150     // Soglia per un movimento LENTO
#define SWING_THR 300       // Soglia per un movimento VELOCE
#define STRIKE_THR 150      // Soglia per un COLPO
#define STRIKE_S_THR 320    // Soglia per un COLPO FORTE

// --- Effetti Visivi ---
#define FLASH_DELAY 80      // Durata (ms) del flash bianco quando si colpisce qualcosa
#define PULSE_ALLOW 1       // Abilita (1) o disabilita (0) l'effetto di pulsazione della lama
#define PULSE_AMPL 20       // Ampiezza della pulsazione
#define PULSE_DELAY 30      // Intervallo (ms) tra gli impulsi

// --- Sicurezza e Debug ---
#define BATTERY_SAFE 1      // Abilita (1) o disabilita (0) il controllo della batteria
#define DEBUG 1             // Abilita (1) o disabilita (0) i messaggi di debug sulla porta seriale
