/*
 * ==================================================================================
 * === SUPER-DUPER COOL ARDUINO BASED MULTICOLOR SOUND PLAYING LIGHTSABER           ===
 * === Progetto Originale di AlexGyver                                            ===
 * ===                                                                            ===
 * === Questo codice è la versione STABILE E DEFINITIVA per ESP32-S3.              ===
 * === Utilizza una soluzione audio custom "Bare-Metal" per la massima stabilità.   ===
 * ==================================================================================
 */

// ============================ LIBRERIE ============================
#include <Arduino.h>
#include <EEPROM.h>
#include "Wire.h"
#include <SPI.h>
#include <SD.h>
#include "FastLED.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ========================= REGISTRI HARDWARE DEL DAC (ESP32-S3) =========================
#define SENS_SAR_DAC_CTRL1_REG      0x6000E018
#define SENS_SAR_DAC_CTRL2_REG      0x6000E01C
#define SENS_SAR_DAC_DATA1_REG      0x6000E024
#define SENS_DAC_CLK_EN             (BIT(18))
#define SENS_DAC_CW_EN1_M           (BIT(20))
#define SENS_DAC_DIG_FORCE_M        (BIT(19))

// ========================= DEFINIZIONE HARDWARE & PINOUT (ESP32-S3) =========================
#define LED_PIN 18
#define NUM_LEDS 30
#define BRIGHTNESS 255
#define BTN_PIN 4
#define SPEAKER_PIN 17
#define MPU_SDA_PIN 8
#define MPU_SCL_PIN 9
#define SD_CS_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SCK_PIN 12
#define VOLT_PIN 1
#define R1 100000.0f
#define R2 33000.0f

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
CRGB leds[NUM_LEDS];
Adafruit_MPU6050 mpu;
SPIClass spi = SPIClass(HSPI);
unsigned long ACC, GYR;
unsigned long mpuTimer;
boolean ls_state = false;
boolean ls_chg_state = false;
boolean btnState, btn_flag, hold_flag;
byte btn_counter;
unsigned long btn_timer;
unsigned long PULSE_timer, swing_timer, swing_timeout, battery_timer;
byte nowColor;
byte red_val, green_val, blue_val, redOffset, greenOffset, blueOffset;
int PULSEOffset;
float k = 0.2;
boolean eeprom_flag = false;
float voltage;
TaskHandle_t audioTaskHandle = NULL;
volatile bool isPlaying = false;
char sound_buffer[12];

// ============================ DEFINIZIONE FILE AUDIO (PROGMEM) ============================
const char strike1[] PROGMEM = "/SK1.wav"; const char strike2[] PROGMEM = "/SK2.wav";
const char* const strikes[] PROGMEM = { strike1, strike2 };
const char strike_s1[] PROGMEM = "/SKS1.wav"; const char strike_s2[] PROGMEM = "/SKS2.wav";
const char* const strikes_short[] PROGMEM = { strike_s1, strike_s2 };
const char swing1[] PROGMEM = "/SWS1.wav"; const char swing2[] PROGMEM = "/SWS2.wav";
const char* const swings[] PROGMEM = { swing1, swing2 };
const char swingL1[] PROGMEM = "/SWL1.wav"; const char swingL2[] PROGMEM = "/SWL2.wav";
const char* const swings_L[] PROGMEM = { swingL1, swingL2 };
const char sound_on[] PROGMEM = "/ON.wav";
const char sound_off[] PROGMEM = "/OFF.wav";
const char sound_hum[] PROGMEM = "/HUM.wav";

// =============================== FUNZIONI AUDIO BARE-METAL ================================
void bareMetalDacWrite(uint8_t value) {
    WRITE_PERI_REG(SENS_SAR_DAC_DATA1_REG, value);
}

void bareMetalDacEnable() {
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_DAC_CLK_EN);
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DIG_FORCE_M);
    pinMode(SPEAKER_PIN, ANALOG);
}

void audioTask(void *parameter) {
    char* filename = (char*)parameter;
    File audioFile = SD.open(filename);

    if (!audioFile) { vTaskDelete(NULL); return; }

    uint32_t sampleRate = 0;
    audioFile.seek(24);
    audioFile.read((byte*)&sampleRate, 4);
    uint16_t bitsPerSample = 0;
    audioFile.seek(34);
    audioFile.read((byte*)&bitsPerSample, 2);
    audioFile.seek(44);

    if ((bitsPerSample != 8 && bitsPerSample != 16) || sampleRate == 0) {
        audioFile.close();
        vTaskDelete(NULL);
        return;
    }

    isPlaying = true;
    uint32_t delay_us = 1000000 / sampleRate;
    uint8_t buffer[512];
    int bytesRead;

    while (isPlaying && (bytesRead = audioFile.read(buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytesRead; i++) {
            uint8_t sample = (bitsPerSample == 8) ? buffer[i] : ((int16_t)(buffer[i+1] << 8 | buffer[i]) >> 8) + 128;
            bareMetalDacWrite(sample);
            delayMicroseconds(delay_us);
            if (bitsPerSample == 16) i++;
        }
    }

    bareMetalDacWrite(128);
    audioFile.close();
    isPlaying = false;
    vTaskDelete(NULL);
}

void playSound(const char* filename) {
    if (isPlaying) {
        isPlaying = false;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    strcpy(sound_buffer, filename);
    xTaskCreate(audioTask, "AudioTask", 2048, sound_buffer, 5, &audioTaskHandle);
}

// =================================== SETUP ===================================
void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println(F("Avvio GyverSaber su ESP32 (Bare-Metal Audio)..."));
  }

  pinMode(BTN_PIN, INPUT_PULLUP);
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);
  
  if (!mpu.begin()) {
    if(DEBUG) Serial.println("MPU6050 non trovato!");
    while (1);
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
  
  bareMetalDacEnable();

  EEPROM.begin(16);
  if (EEPROM.read(0) <= 5) { nowColor = EEPROM.read(0); }
  else { nowColor = 0; EEPROM.write(0, nowColor); EEPROM.commit(); }
  setColor(nowColor);

  voltage_measure();
  int ledsToShow = map(analogRead(VOLT_PIN), 2400, 3150, 0, (NUM_LEDS / 2));
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
  handleSaberState();
  if(ls_state){
    randomPULSE();
    getMotion();
    strikeTick();
    swingTick();
    batteryTick();
    
    if (!isPlaying){
      strcpy_P(sound_buffer, (char*)pgm_read_word(&(sound_hum)));
      playSound(sound_buffer);
    }
  }
  btnTick();
}

// =============================== ALTRE FUNZIONI ================================
void handleSaberState() {
  if (!ls_chg_state) return;
  ls_state = !ls_state;
  if (ls_state) {
    voltage_measure();
    if (voltage > 3.4 || !BATTERY_SAFE) {
      strcpy_P(sound_buffer, (char*)pgm_read_word(&(sound_on)));
      playSound(sound_buffer);
      delay(200);
      light_up();
    } else { ls_state = false; }
  } else {
    strcpy_P(sound_buffer, (char*)pgm_read_word(&(sound_off)));
    playSound(sound_buffer);
    delay(300);
    light_down();
    if (eeprom_flag) { eeprom_flag = false; EEPROM.write(0, nowColor); EEPROM.commit(); }
  }
  ls_chg_state = false;
}

void btnTick() {
  btnState = !digitalRead(BTN_PIN);
  if (btnState && !btn_flag) { btn_flag = true; btn_counter++; btn_timer = millis(); }
  if (!btnState && btn_flag) { btn_flag = false; hold_flag = false; }
  if (btn_flag && btnState && (millis() - btn_timer > BTN_TIMEOUT) && !hold_flag) {
    ls_chg_state = true;
    hold_flag = true;
    btn_counter = 0;
  }
  if ((millis() - btn_timer > BTN_TIMEOUT) && (btn_counter != 0)) {
    if (ls_state && btn_counter == 3) {
        nowColor = (nowColor + 1) % 6;
        setColor(nowColor);
        setAll(red_val, green_val, blue_val);
        eeprom_flag = true;
    }
    btn_counter = 0;
  }
}

void getMotion() {
  if (millis() - mpuTimer > 10) {
    mpuTimer = millis();
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    ACC = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));
    GYR = sqrt(sq(g.gyro.x) + sq(g.gyro.y) + sq(g.gyro.z));
  }
}

void strikeTick() {
    if (isPlaying) return;
    if (ACC > 30) {
        if (ACC > 60) strcpy_P(sound_buffer, (char*)pgm_read_word(&(strikes[random(2)])));
        else strcpy_P(sound_buffer, (char*)pgm_read_word(&(strikes_short[random(2)])));
        playSound(sound_buffer);
        hit_flash();
    }
}

void swingTick() {
    if (isPlaying) return;
    if (millis() - swing_timer < SWING_TIMEOUT) return;
    if (GYR > 4) {
        if (GYR > 8) strcpy_P(sound_buffer, (char*)pgm_read_word(&(swings[random(2)])));
        else strcpy_P(sound_buffer, (char*)pgm_read_word(&(swings_L[random(2)])));
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

void setPixel(int p, byte r, byte g, byte b) { leds[p].setRGB(r, g, b); }
void setAll(byte r, byte g, byte b) { for (int i = 0; i < NUM_LEDS; i++) { leds[i].setRGB(r, g, b); } FastLED.show(); }
void light_up() { for (int i=0;i<(NUM_LEDS/2)+1;i++) { setPixel(i,red_val,green_val,blue_val); setPixel((NUM_LEDS-1-i),red_val,green_val,blue_val); FastLED.show(); delay(15); } }
void light_down() { for (int i=(NUM_LEDS/2);i>=0;i--) { setPixel(i,0,0,0); setPixel((NUM_LEDS-1-i),0,0,0); FastLED.show(); delay(15); } }
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

void batteryTick() {
  if (BATTERY_SAFE && (millis() - battery_timer > 30000)) {
    battery_timer = millis();
    voltage_measure();
    if (voltage < 3.4) { ls_chg_state = true; }
  }
}

void voltage_measure() {
  uint32_t sum = 0;
  for (int i = 0; i < 10; i++) sum += analogRead(VOLT_PIN);
  int valoreLetto = sum / 10;
  voltage = (float)valoreLetto * 3.3f / 4095.0f * (R1 + R2) / R2;
}
