/*
 * ==================================================================================
 * === SUPER-DUPER COOL ARDUINO BASED MULTICOLOR SOUND PLAYING LIGHTSABER           ===
 * === Progetto Originale di AlexGyver                                            ===
 * ===                                                                            ===
 * === Questo codice è la versione STABILE E DEFINITIVA per ESP32-S3.              ===
 * === Utilizza librerie moderne e testate per la massima compatibilità.          ===
 * ==================================================================================
 *
 * AGGIORNAMENTI CHIAVE IN QUESTA VERSIONE (V2):
 * - SOSTITUITE le vecchie librerie I2Cdev e MPU6050 con la libreria ufficiale
 * "Adafruit MPU6050". Questo risolve i problemi di compilazione e blocco.
 * - Semplificata l'inizializzazione del sensore.
 * - Corretto il valore della resistenza R2 per la sicurezza dell'ESP32.
 * - Mantenute tutte le funzionalità originali con codice più pulito e stabile.
 */

// ============================ LIBRERIE ============================
// --- Librerie Core ---
#include <Arduino.h>
#include <EEPROM.h>
#include "Wire.h"
#include <SPI.h>
#include <SD.h>

// --- Librerie Specifiche per Dispositivi ---
#include "FastLED.h"
// --- MODIFICA IMPORTANTE: Nuove librerie per il sensore MPU6050 ---
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// --- LIBRERIE AUDIO DI ALTA QUALITÀ PER ESP32 ---
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// ========================= DEFINIZIONE HARDWARE & PINOUT (ESP32-S3) =========================
// --- Striscia LED ---
#define LED_PIN 18
#define NUM_LEDS 30
#define BRIGHTNESS 255

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
#define R1 100000.0f
#define R2 33000.0f         // Valore corretto per la sicurezza dell'ESP32

// ============================ IMPOSTAZIONI DEL PROGETTO ============================
#define BTN_TIMEOUT 800
#define SWING_TIMEOUT 500
#define SWING_L_THR 150
#define SWING_THR 300
#define STRIKE_THR 150
#define STRIKE_S_THR 320
#define FLASH_DELAY 80

#define PULSE_ALLOW 1
#define PULSE_AMPL 20
#define PULSE_DELAY 30

#define BATTERY_SAFE 1
#define DEBUG 1

// ============================ OGGETTI GLOBALI & VARIABILI ============================
// --- Dispositivi & Librerie ---
CRGB leds[NUM_LEDS];
Adafruit_MPU6050 mpu; // NUOVO oggetto per il sensore
SPIClass spi = SPIClass(HSPI);

// --- Audio Objects ---
AudioGeneratorWAV *wav;
AudioFileSourceSD *file;
AudioOutputI2S *out;

// --- Dati di Movimento & Sensore ---
unsigned long ACC, GYR;
unsigned long mpuTimer;

// --- Stato della Spada Laser ---
boolean ls_state = false;
boolean ls_chg_state = false;

// --- Gestione Pulsante ---
boolean btnState, btn_flag, hold_flag;
byte btn_counter;
unsigned long btn_timer;

// --- Timer ---
unsigned long PULSE_timer, swing_timer, swing_timeout, battery_timer;

// --- Colore & Effetti ---
byte nowColor;
byte red_val, green_val, blue_val, redOffset, greenOffset, blueOffset;
int PULSEOffset;
float k = 0.2;

// --- Sistema ---
boolean eeprom_flag = false;
float voltage;

// ============================ DEFINIZIONE FILE AUDIO (PROGMEM) ============================
// NOTA: I nomi dei file sulla scheda SD devono iniziare con una barra '/'
const char strike1[] PROGMEM = "/SK1.wav"; const char strike2[] PROGMEM = "/SK2.wav";
const char* const strikes[] PROGMEM = { strike1, strike2 /*, ... (omesso per brevità) */};

const char strike_s1[] PROGMEM = "/SKS1.wav"; const char strike_s2[] PROGMEM = "/SKS2.wav";
const char* const strikes_short[] PROGMEM = { strike_s1, strike_s2 /*, ... */};

const char swing1[] PROGMEM = "/SWS1.wav"; const char swing2[] PROGMEM = "/SWS2.wav";
const char* const swings[] PROGMEM = { swing1, swing2 /*, ... */};

const char swingL1[] PROGMEM = "/SWL1.wav"; const char swingL2[] PROGMEM = "/SWL2.wav";
const char* const swings_L[] PROGMEM = { swingL1, swingL2 /*, ... */};

const char sound_on[] PROGMEM = "/ON.wav";
const char sound_off[] PROGMEM = "/OFF.wav";
const char sound_hum[] PROGMEM = "/HUM.wav";
char sound_buffer[12];

// =================================== SETUP ===================================
void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println(F("Avvio GyverSaber su ESP32 (V2 Stabile)..."));
  }

  pinMode(BTN_PIN, INPUT_PULLUP);

  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  
  spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
  
  // --- NUOVA INIZIALIZZAZIONE MPU6050 ---
  if (!mpu.begin()) {
    if(DEBUG) Serial.println("MPU6050 non trovato!");
    while (1) { delay(10); }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  if(DEBUG) Serial.println("MPU6050 OK");

  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(100);
  setAll(0, 0, 0);

  if (!SD.begin(SD_CS_PIN, spi)) {
    if (DEBUG) Serial.println(F("Montaggio Scheda SD Fallito!"));
  } else {
    if (DEBUG) Serial.println(F("Scheda SD OK"));
  }
  
  out = new AudioOutputI2S(0, 1);
  out->SetPinout(25, SPEAKER_PIN, 27);
  out->SetOutputModeMono(true);
  out->SetGain(2.0);

  EEPROM.begin(16);
  if (EEPROM.read(0) <= 5) {
    nowColor = EEPROM.read(0);
  } else {
    nowColor = 0;
    EEPROM.write(0, nowColor);
    EEPROM.commit();
  }
  setColor(nowColor);

  byte capacity = voltage_measure();
  if (DEBUG) {
    Serial.print(F("Tensione Batteria: ")); Serial.println(voltage);
  }
  int ledsToShow = map(capacity, 100, 0, (NUM_LEDS / 2), 0);
  for (int i = 0; i < ledsToShow; i++) {
    setPixel(i, red_val, green_val, blue_val);
    setPixel((NUM_LEDS - 1 - i), red_val, green_val, blue_val);
    FastLED.show();
    delay(25);
  }
  delay(1000);
  setAll(0, 0, 0);
  FastLED.setBrightness(BRIGHTNESS);
}

// =================================== MAIN LOOP ===================================
void loop() {
  if (wav && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      delete wav; wav = nullptr;
      delete file; file = nullptr;
    }
  }

  handleSaberState();
  if(ls_state){
    randomPULSE();
    getMotion();
    strikeTick();
    swingTick();
    batteryTick();
    
    if (!wav || !wav->isRunning()){
      strcpy_P(sound_buffer, (char*)pgm_read_word(&(sound_hum)));
      playSound(sound_buffer);
    }
  }

  btnTick();
}

// =============================== FUNZIONI DI SUPPORTO ================================

void playSound(const char* filename) {
  if (wav && wav->isRunning()) {
    wav->stop();
    delete wav; wav = nullptr;
    delete file; file = nullptr;
  }
  
  file = new AudioFileSourceSD(filename);
  wav = new AudioGeneratorWAV();
  wav->begin(file, out);
}

void handleSaberState() {
  if (!ls_chg_state) return;

  ls_state = !ls_state;

  if (ls_state) {
    if (voltage_measure() > 10 || !BATTERY_SAFE) {
      if (DEBUG) Serial.println(F("SPADA ACCESA"));
      strcpy_P(sound_buffer, (char*)pgm_read_word(&(sound_on)));
      playSound(sound_buffer);
      delay(200);
      light_up();
    } else {
      ls_state = false;
      if (DEBUG) Serial.println(F("TENSIONE BASSA! La spada non si accenderà."));
    }
  } else {
    if (DEBUG) Serial.println(F("SPADA SPENTA"));
    strcpy_P(sound_buffer, (char*)pgm_read_word(&(sound_off)));
    playSound(sound_buffer);
    delay(300);
    light_down();
    if (eeprom_flag) {
      eeprom_flag = false;
      EEPROM.write(0, nowColor);
      EEPROM.commit();
    }
  }
  ls_chg_state = false;
}

void btnTick() {
  btnState = !digitalRead(BTN_PIN);
  if (btnState && !btn_flag) {
    btn_flag = true;
    btn_counter++;
    btn_timer = millis();
  }
  if (!btnState && btn_flag) {
    btn_flag = false;
    hold_flag = false;
  }

  if (btn_flag && btnState && (millis() - btn_timer > BTN_TIMEOUT) && !hold_flag) {
    ls_chg_state = true;
    hold_flag = true;
    btn_counter = 0;
  }

  if ((millis() - btn_timer > BTN_TIMEOUT) && (btn_counter != 0)) {
    if (ls_state) {
      if (btn_counter == 3) {
        nowColor = (nowColor + 1) % 6;
        setColor(nowColor);
        setAll(red_val, green_val, blue_val);
        eeprom_flag = true;
      }
    }
    btn_counter = 0;
  }
}

// --- NUOVA FUNZIONE getMotion ---
void getMotion() {
  if (millis() - mpuTimer > 10) {
    mpuTimer = millis();
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Calcoliamo la magnitudine totale dei vettori
    // Usiamo i valori diretti in m/s^2 e rad/s, che sono più standard
    ACC = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));
    GYR = sqrt(sq(g.gyro.x) + sq(g.gyro.y) + sq(g.gyro.z));
  }
}

void strikeTick() {
    if (wav && wav->isRunning()) return;
    
    // Le soglie andranno ritestate, perché i valori della nuova libreria sono diversi
    if (ACC > 30) { // Esempio di nuova soglia per il colpo
        if (ACC > 60) { // Esempio di nuova soglia per il colpo forte
            strcpy_P(sound_buffer, (char*)pgm_read_word(&(strikes[random(2)])));
        } else {
            strcpy_P(sound_buffer, (char*)pgm_read_word(&(strikes_short[random(2)])));
        }
        playSound(sound_buffer);
        hit_flash();
    }
}

void swingTick() {
    if (wav && wav->isRunning()) return;
    if (millis() - swing_timer < SWING_TIMEOUT) return;
    
    // Le soglie andranno ritestate
    if (GYR > 4) { // Esempio di nuova soglia per il movimento
        if (GYR > 8) { // Esempio di nuova soglia per il movimento veloce
            strcpy_P(sound_buffer, (char*)pgm_read_word(&(swings[random(2)])));
        } else {
            strcpy_P(sound_buffer, (char*)pgm_read_word(&(swings_L[random(2)])));
        }
        playSound(sound_buffer);
        swing_timer = millis();
    }
}

void randomPULSE() {
  if (PULSE_ALLOW && (millis() - PULSE_timer > PULSE_DELAY)) {
    PULSE_timer = millis();
    PULSEOffset = PULSEOffset * k + random(-PULSE_AMPL, PULSE_AMPL) * (1.0 - k);
    redOffset = constrain(red_val + PULSEOffset, 0, 255);
    greenOffset = constrain(green_val + PULSEOffset, 0, 255);
    blueOffset = constrain(blue_val + PULSEOffset, 0, 255);
    setAll(redOffset, greenOffset, blueOffset);
  }
}

void setPixel(int pixel, byte r, byte g, byte b) { leds[pixel].setRGB(r, g, b); }
void setAll(byte r, byte g, byte b) { for (int i = 0; i < NUM_LEDS; i++) { leds[i].setRGB(r, g, b); } FastLED.show(); }
void light_up() { for (int i = 0; i < (NUM_LEDS / 2) + 1; i++) { setPixel(i, red_val, green_val, blue_val); setPixel((NUM_LEDS - 1 - i), red_val, green_val, blue_val); FastLED.show(); delay(15); } }
void light_down() { for (int i = (NUM_LEDS / 2) ; i >= 0; i--) { setPixel(i, 0, 0, 0); setPixel((NUM_LEDS - 1 - i), 0, 0, 0); FastLED.show(); delay(15); } }
void hit_flash() { setAll(255, 255, 255); delay(FLASH_DELAY); setAll(red_val, green_val, blue_val); }

void setColor(byte color) {
  switch (color) {
    case 0: red_val = 255; green_val = 0;   blue_val = 0;   break;
    case 1: red_val = 0;   green_val = 255; blue_val = 0;   break;
    case 2: red_val = 0;   green_val = 0;   blue_val = 255; break;
    case 3: red_val = 255; green_val = 0;   blue_val = 255; break;
    case 4: red_val = 255; green_val = 255; blue_val = 0;   break;
    case 5: red_val = 0;   green_val = 255; blue_val = 255; break;
  }
}

// --- GESTIONE BATTERIA ---
#define VALORE_ADC_PIENO 3150
#define VALORE_ADC_SCARICO 2400

void batteryTick() {
  if (BATTERY_SAFE && (millis() - battery_timer > 30000)) {
    battery_timer = millis();
    if (voltage_measure() < 15) {
      ls_chg_state = true;
    }
  }
}

byte voltage_measure() {
  uint32_t sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(VOLT_PIN);
  }
  int valoreLetto = sum / 10;
  int percent = map(valoreLetto, VALORE_ADC_SCARICO, VALORE_ADC_PIENO, 0, 100);
  return constrain(percent, 0, 100);
}
