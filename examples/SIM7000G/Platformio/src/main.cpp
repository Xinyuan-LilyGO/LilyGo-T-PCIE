/*
  FILE: AllFunctions.ino
  AUTHOR: Koby Hale
  PURPOSE: Test functionality
*/

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// See all AT commands, if wanted
 #define DUMP_AT_COMMANDS

/*
   Tests enabled
*/
#define TINY_GSM_TEST_GPRS    true
#define TINY_GSM_TEST_GPS     true
#define TINY_GSM_POWERDOWN    true

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]  = "YOUR-APN";     //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <TinyGsmClient.h>
#include <SPI.h>
#include <SD.h>
#include <Ticker.h>

#ifdef DUMP_AT_COMMANDS  // if enabled it requires the streamDebugger lib
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  60          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4

#define SD_MISO     2
#define SD_MOSI     15
#define SD_SCLK     14
#define SD_CS       13
#define LED_PIN     12


int counter, lastIndex, numberOfPieces = 24;
String pieces[24], input;


void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // Set LED OFF
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(300);
  digitalWrite(PWR_PIN, LOW);

  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SDCard MOUNT FAIL");
  } else {
    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    String str = "SDCard Size: " + String(cardSize) + "MB";
    Serial.println(str);
  }

  Serial.println("\nWait...");

  delay(10000);

  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  if (!modem.restart()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  String name = modem.getModemName();
  delay(500);
  Serial.println("Modem Name: " + name);

  String modemInfo = modem.getModemInfo();
  delay(500);
  Serial.println("Modem Info: " + modemInfo);

  // Set SIM7000G GPIO4 LOW ,turn off GPS power
  // CMD:AT+SGPIO=0,4,1,0
  // Only in version 20200415 is there a function to control GPS power
  modem.sendAT("+SGPIO=0,4,1,0");




  delay(5000);


}

void loop() {
Serial.println("loop...");

#if TINY_GSM_POWERDOWN
  // Try to power-off (modem may decide to restart automatically)
  // To turn off modem completely, please use Reset/Enable pins
      modem.sendAT("+CPOWD=1");
  if (modem.waitResponse(10000L) != 1) {
        DBG("+CPOWD=1");
    }

  modem.poweroff();
  Serial.println("Poweroff.");
#endif

  digitalWrite(LED_PIN, LOW);
  pinMode(LED_PIN, INPUT);
  pinMode(PWR_PIN, INPUT);
    pinMode(PIN_DTR, INPUT);
      pinMode(PIN_TX, INPUT);
        pinMode(PIN_RX, INPUT);
          pinMode(SD_MISO, INPUT);
            pinMode(SD_MOSI, INPUT);
          pinMode(SD_SCLK, INPUT);
            pinMode(SD_CS, INPUT);
  //esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  delay(200);
  esp_deep_sleep_start();

  
Serial.println("？？...");
/*
  // Do nothing forevermore
  while (true) {
    modem.maintain();
  }*/
}
