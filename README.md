# GyverSaber ESP32 Edition

![GyverSaber](https://i.imgur.com/8B1Z9J8.jpg)

Questa è una versione aggiornata e migliorata del progetto **GyverSaber** di AlexGyver, portata su **ESP32-S3** e strutturata per l'ambiente di sviluppo **PlatformIO**.

Questo progetto ti permette di costruire una spada laser multifunzione con lama multicolore, effetti sonori realistici basati su scheda SD e rilevamento del movimento per un'esperienza immersiva.

## Funzionalità

- **Lama a LED Indirizzabili**: Effetti di accensione e spegnimento fluidi.
- **Colori Multipli**: Cambia colore con un triplo click sul pulsante (rosso, verde, blu, magenta, giallo, ciano).
- **Audio di Alta Qualità**: Suoni di accensione, spegnimento, ronzio (hum), movimento (swing) e colpi (clash) riprodotti da una scheda SD.
- **Rilevamento del Movimento**: Grazie al sensore MPU6050, la spada riproduce suoni diversi per movimenti lenti e veloci.
- **Effetto Flash on Clash**: La lama emette un lampo bianco brillante quando rileva un colpo.
- **Monitoraggio della Batteria**: Il sistema controlla la tensione della batteria e si spegne automaticamente per proteggerla. All'avvio, la lama mostra il livello di carica.
- **Memoria Interna**: L'ultimo colore selezionato viene salvato e ripristinato al riavvio.

## Componenti Hardware

Per costruire la spada laser, avrai bisogno dei seguenti componenti:

| Componente                 | Descrizione                                                                 | Link Esempio (AliExpress)                                     |
| -------------------------- | --------------------------------------------------------------------------- | ------------------------------------------------------------- |
| **Microcontrollore**       | ESP32-S3 DevKitC-1 o simile                                                 | [Link](https://ali.ski/k_5fS)                                 |
| **Striscia LED**           | WS2811 (12V) o WS2812B (5V), 60 LED/m, PCB bianco, IP30                      | [Link WS2811](https://ali.ski/NMjk8)                          |
| **Sensore di Movimento**   | MPU6050 Giroscopio/Accelerometro a 6 assi                                   | [Link](https://ali.ski/v5KZLL)                                |
| **Modulo Scheda SD**       | Modulo MicroSD Card Reader (interfaccia SPI)                                | [Link](https://ali.ski/JHIePz)                                |
| **Amplificatore Audio**    | PAM8403 (per batterie Li-Ion)                                               | [Link](https://ali.ski/w3a6s)                                 |
| **Altoparlante**           | 3W, 4 Ohm                                                                   | [Link](https://ali.ski/fkf3b5)                                |
| **Pulsante**               | Pulsante momentaneo con LED (opzionale)                                     | [Link](https://ali.ski/Mwcxt)                                 |
| **Batterie**               | 1 o più celle Li-Ion 18650                                                  | [Link](https://ali.ski/XBqThJ)                                |
| **Convertitore DC-DC**     | Step-down per alimentare l'ESP32 (se si usa una batteria > 5V)              | [Link](https://ali.ski/Gu17D)                                 |
| **Resistenze**             | 100kΩ e 33kΩ per il partitore di tensione                                   | [Link](https://ali.ski/DsRXU)                                 |

## Schema di Collegamento (ESP32-S3)

Ecco uno schema di collegamento di base. Assicurati di adattare i pin se li modifichi nel file `config.h`.

```
TODO: Add Fritzing Diagram Image
```

| Componente           | Pin ESP32-S3         | Note                                                               |
| -------------------- | -------------------- | ------------------------------------------------------------------ |
| **Striscia LED (Dati)** | `GPIO 18`            |                                                                    |
| **Pulsante**           | `GPIO 4`             | Collegato a GND                                                    |
| **MPU6050 SDA**        | `GPIO 8`             |                                                                    |
| **MPU6050 SCL**        | `GPIO 9`             |                                                                    |
| **SD Card CS**         | `GPIO 10`            |                                                                    |
| **SD Card MOSI**       | `GPIO 11`            |                                                                    |
| **SD Card MISO**       | `GPIO 13`            |                                                                    |
| **SD Card SCK**        | `GPIO 12`            |                                                                    |
| **Amplificatore (IN)** | `GPIO 17`            |                                                                    |
| **Partitore Tensione** | `GPIO 1`             | Tra `VCC Batteria` e `GND`, con il pin `GPIO 1` tra le due resistenze |

## Configurazione del Software

Questo progetto è ottimizzato per **Visual Studio Code** con l'estensione **PlatformIO**.

### 1. Installazione
1.  [Installa Visual Studio Code](https://code.visualstudio.com/).
2.  Apri VS Code, vai alla sezione "Estensioni" e cerca e installa **PlatformIO IDE**.
3.  Clona o scarica questo repository.
4.  Apri la cartella del progetto in VS Code (`File > Apri Cartella...`).

### 2. Preparazione della Scheda SD
1.  Formatta una scheda MicroSD (max 4GB) in formato **FAT** o **FAT32**.
2.  Copia tutti i file `.wav` dalla cartella `data` di questo progetto alla radice (root) della scheda SD.

### 3. Compilazione e Caricamento
1.  PlatformIO installerà automaticamente tutte le librerie necessarie al primo avvio.
2.  Collega il tuo ESP32-S3 al computer tramite USB.
3.  Clicca sull'icona della freccia (`->`) nella barra di stato di PlatformIO in basso per compilare e caricare il firmware.

## Personalizzazione e Calibrazione

### Configurazione
Tutte le impostazioni principali (pin, luminosità, sensibilità) si trovano nel file `src/config.h`. Modifica questo file per adattare il progetto al tuo hardware specifico.

### Calibrazione del Sensore di Movimento
Le soglie per i colpi (`STRIKE_THR`) e i movimenti (`SWING_THR`) sono cruciali per un funzionamento corretto. Per calibrarle:
1.  Nel file `src/config.h`, assicurati che `DEBUG` sia impostato su `1`.
2.  Carica il codice e apri il **Serial Monitor** di PlatformIO (icona a forma di spina nella barra di stato).
3.  Imposta la velocità a **115200 baud**.
4.  Muovi la spada ed esegui dei colpi. Osserva i valori di `ACC` (accelerazione) e `GYR` (giroscopio) stampati sul monitor.
5.  Modifica i valori `STRIKE_THR` e `SWING_THR` in `config.h` per farli corrispondere ai valori che vedi.

## Licenza

Questo progetto è rilasciato sotto la licenza MIT. Vedi il file [LICENSE](LICENSE) per maggiori dettagli.
