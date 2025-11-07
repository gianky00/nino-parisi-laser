/*
 * ==================================================================================
 * ===           GYVERSABER V2 - PORTING PER ESP32-S3 CON PLATFORMIO              ===
 * === Progetto Originale di AlexGyver, riadattato e migliorato                   ===
 * ==================================================================================
 *
 * DESCRIZIONE:
 * Questo progetto trasforma un ESP32-S3 in una spada laser multifunzione con:
 * - Lama multicolore basata su LED indirizzabili (WS2811/WS2812).
 * - Effetti sonori realistici basati su file .wav da una scheda SD.
 * - Rilevamento del movimento (swing e colpi) tramite un sensore MPU6050.
 * - Gestione avanzata dell'alimentazione con monitoraggio della batteria.
 *
 * MIGLIORAMENTI IN QUESTA VERSIONE:
 * - Struttura del progetto basata su PlatformIO per una gestione pulita
 *   delle dipendenze e una facile compilazione.
 * - Codice modularizzato: le impostazioni sono in `config.h`.
 * - Utilizzo di librerie moderne e stabili per ESP32.
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
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// --- Librerie Audio ---
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// --- IMPOSTAZIONI E CONFIGURAZIONE ---
#include "config.h"         // Contiene tutte le impostazioni e i pin

// ============================ OGGETTI GLOBALI & VARIABILI ============================
// --- Dispositivi & Librerie ---
CRGB leds[NUM_LEDS];
Adafruit_MPU6050 mpu;
SPIClass spi = SPIClass(HSPI);

// --- Oggetti Audio ---
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
unsigned long PULSE_timer, swing_timer, battery_timer;

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
const char strike3[] PROGMEM = "/SK3.wav"; const char strike4[] PROGMEM = "/SK4.wav";
const char strike5[] PROGMEM = "/SK5.wav"; const char strike6[] PROGMEM = "/SK6.wav";
const char strike7[] PROGMEM = "/SK7.wav"; const char strike8[] PROGMEM = "/SK8.wav";
const char* const strikes[] PROGMEM = { strike1, strike2, strike3, strike4, strike5, strike6, strike7, strike8 };

const char strike_s1[] PROGMEM = "/SKS1.wav"; const char strike_s2[] PROGMEM = "/SKS2.wav";
const char strike_s3[] PROGMEM = "/SKS3.wav"; const char strike_s4[] PROGMEM = "/SKS4.wav";
const char strike_s5[] PROGMEM = "/SKS5.wav"; const char strike_s6[] PROGMEM = "/SKS6.wav";
const char strike_s7[] PROGMEM = "/SKS7.wav"; const char strike_s8[] PROGMEM = "/SKS8.wav";
const char* const strikes_short[] PROGMEM = { strike_s1, strike_s2, strike_s3, strike_s4, strike_s5, strike_s6, strike_s7, strike_s8 };

const char swing1[] PROGMEM = "/SWS1.wav"; const char swing2[] PROGMEM = "/SWS2.wav";
const char swing3[] PROGMEM = "/SWS3.wav"; const char swing4[] PROGMEM = "/SWS4.wav";
const char swing5[] PROGMEM = "/SWS5.wav";
const char* const swings[] PROGMEM = { swing1, swing2, swing3, swing4, swing5 };

const char swingL1[] PROGMEM = "/SWL1.wav"; const char swingL2[] PROGMEM = "/SWL2.wav";
const char swingL3[] PROGMEM = "/SWL3.wav"; const char swingL4[] PROGMEM = "/SWL4.wav";
const char* const swings_L[] PROGMEM = { swingL1, swingL2, swingL3, swingL4 };

const char sound_on[] PROGMEM = "/ON.wav";
const char sound_off[] PROGMEM = "/OFF.wav";
const char sound_hum[] PROGMEM = "/HUM.wav";
char sound_buffer[12];

// Function prototypes
void playSound(const char* filename);
void handleSaberState();
void btnTick();
void getMotion();
void strikeTick();
void swingTick();
void randomPULSE();
void setPixel(int pixel, byte r, byte g, byte b);
void setAll(byte r, byte g, byte b);
void light_up();
void light_down();
void hit_flash();
void setColor(byte color);
void batteryTick();
byte voltage_measure();

// =================================== SETUP ===================================
void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println(F("Avvio GyverSaber ESP32..."));
  }

  pinMode(BTN_PIN, INPUT_PULLUP);

  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  
  spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
  
  // --- Inizializzazione MPU6050 ---
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
  out->SetPinout(25, SPEAKER_PIN, 27); // Standard I2S pins for ESP32
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
    Serial.print(F("Tensione Batteria: ")); Serial.print(voltage);
    Serial.print(F(" (")); Serial.print(capacity); Serial.println(F("%)"));
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
  // Gestisce la riproduzione audio in background
  if (wav && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      delete wav; wav = nullptr;
      delete file; file = nullptr;
    }
  }

  handleSaberState(); // Controlla se la spada deve accendersi o spegnersi

  if(ls_state){ // Se la spada è accesa...
    randomPULSE();
    getMotion();
    strikeTick();
    swingTick();
    batteryTick();
    
    // Se nessun altro suono è in riproduzione, riproduci il ronzio (hum)
    if (!wav || !wav->isRunning()){
      strcpy_P(sound_buffer, (char*)pgm_read_word(&(sound_hum)));
      playSound(sound_buffer);
    }
  }

  btnTick(); // Controlla lo stato del pulsante
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

/*
 * =================================================================================
 * === IMPORTANTE: CALIBRAZIONE DEL SENSORE                                      ===
 * =================================================================================
 * Le soglie (threshold) per i colpi (strike) e i movimenti (swing) dipendono
 * molto dal sensore specifico e da come è montato.
 * Per CALIBRARE:
 * 1. Abilita il DEBUG in `config.h`.
 * 2. Apri il Serial Monitor a 115200 baud.
 * 3. Muovi la spada e osserva i valori di ACC e GYR stampati.
 * 4. Modifica le soglie in `config.h` per adattarle ai tuoi valori.
 */

void strikeTick() {
    if (wav && wav->isRunning()) return;
    
    // Stampa i valori per il debug, se abilitato
    if (DEBUG && ACC > 15) {
      Serial.print("ACC: "); Serial.println(ACC);
    }

    if (ACC > STRIKE_THR) {
        if (ACC > STRIKE_S_THR) {
            strcpy_P(sound_buffer, (char*)pgm_read_word(&(strikes[random(8)])));
        } else {
            strcpy_P(sound_buffer, (char*)pgm_read_word(&(strikes_short[random(8)])));
        }
        playSound(sound_buffer);
        hit_flash();
    }
}

void swingTick() {
    unsigned long current_millis = millis();
    if (wav && wav->isRunning()) return;
    if (current_millis - swing_timer < SWING_TIMEOUT) return;
    
    // Stampa i valori per il debug, se abilitato
    if (DEBUG && GYR > 2) {
      Serial.print("GYR: "); Serial.println(GYR);
    }

    if (GYR > SWING_L_THR) {
        if (GYR > SWING_THR) {
            strcpy_P(sound_buffer, (char*)pgm_read_word(&(swings[random(5)])));
        } else {
            strcpy_P(sound_buffer, (char*)pgm_read_word(&(swings_L[random(4)])));
        }
        playSound(sound_buffer);
        swing_timer = current_millis;
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
    case 0: red_val = 255; green_val = 0;   blue_val = 0;   break; // Rosso
    case 1: red_val = 0;   green_val = 255; blue_val = 0;   break; // Verde
    case 2: red_val = 0;   green_val = 0;   blue_val = 255; break; // Blu
    case 3: red_val = 255; green_val = 0;   blue_val = 255; break; // Magenta
    case 4: red_val = 255; green_val = 255; blue_val = 0;   break; // Giallo
    case 5: red_val = 0;   green_val = 255; blue_val = 255; break; // Ciano
  }
}

// --- GESTIONE BATTERIA ---
// Questi valori dipendono dal partitore di tensione e dalla risoluzione dell'ADC
#define VALORE_ADC_PIENO 3150      // Valore ADC a batteria carica (es. 4.2V)
#define VALORE_ADC_SCARICO 2400    // Valore ADC a batteria scarica (es. 3.2V)

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
    delay(1);
  }
  int valoreLetto = sum / 10;

  // Calcola la tensione reale per il debug
  voltage = (valoreLetto / 4095.0) * 3.3 * ((R1 + R2) / R2);

  int percent = map(valoreLetto, VALORE_ADC_SCARICO, VALORE_ADC_PIENO, 0, 100);
  return constrain(percent, 0, 100);
}
