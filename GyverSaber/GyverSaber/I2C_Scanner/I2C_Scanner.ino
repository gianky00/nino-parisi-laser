/*
 * ==================================================================================
 * === SCANNER I2C PER DIAGNOSTICA GYVERSABER                                     ===
 * ==================================================================================
 *
 * DESCRIZIONE:
 * Questo sketch esegue una scansione completa del bus I2C per rilevare qualsiasi
 * dispositivo collegato e riportarne l'indirizzo. È utile per diagnosticare
 * problemi di connessione con sensori come l'MPU6050.
 *
 * ISTRUZIONI:
 * 1. Assicurati che i pin MPU_SDA_PIN e MPU_SCL_PIN corrispondano a quelli
 *    usati nel tuo progetto.
 * 2. Carica questo sketch sul tuo ESP32-S3.
 * 3. Apri il Monitor Seriale con un baud rate di 115200.
 * 4. Osserva l'output:
 *    - Se trovi un dispositivo all'indirizzo 0x68, il cablaggio è corretto.
 *    - Se non trovi nessun dispositivo, c'è un problema di cablaggio (fili,
 *      alimentazione, o saldature).
 *    - Se trovi un dispositivo a un indirizzo diverso, il tuo sensore ne usa
 *      uno non standard.
 */

#include <Wire.h>

// --- DEFINIZIONE PIN I2C (IMPORTANTE!) ---
// Assicurati che questi pin siano gli stessi del tuo progetto principale.
#define MPU_SDA_PIN 8
#define MPU_SCL_PIN 9

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);

  Serial.println("\nScansione del bus I2C in corso...");
  byte count = 0;

  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Dispositivo I2C trovato all'indirizzo 0x");
      if (i < 16) {
        Serial.print("0");
      }
      Serial.println(i, HEX);
      count++;
      delay(1);
    }
  }

  Serial.println("------------------------------------");
  if (count == 0) {
    Serial.println("Nessun dispositivo I2C trovato. Controllare attentamente il cablaggio.");
  } else {
    Serial.print("Scansione completata. Trovati ");
    Serial.print(count);
    Serial.println(" dispositivi.");
  }
  Serial.println("------------------------------------");
}

void loop() {
  // Lo sketch si ferma qui dopo la scansione.
  delay(5000);
}
