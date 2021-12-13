#define TINY_GSM_MODEM_SIM7600   //A7670 Compatible with SIM7600 AT instructions

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

/*
 * Tests enabled
 */
#define TINY_GSM_TEST_GPRS true
#define TINY_GSM_TEST_WIFI false
#define  TINY_GSM_TEST_TCP true
#define TINY_GSM_TEST_GSM_LOCATION false
#define TINY_GSM_TEST_GPS true
// powerdown modem after tests
#define TINY_GSM_POWERDOWN true

// set GSM PIN, if any
#define GSM_PIN ""


// Your GPRS credentials, if any
const char apn[]  = "YourAPN";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details to test TCP/SSL
const char server[]   = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";

#include <TinyGsmClient.h>
#include <Ticker.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

Ticker tick;


#define uS_TO_S_FACTOR          1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP           60          /* Time ESP32 will go to sleep (in seconds) */

#define PIN_TX                  27
#define PIN_RX                  26
#define UART_BAUD               115200
#define PWR_PIN                 4
#define LED_PIN                 12
#define POWER_PIN               25
#define DTR                     32
#define RI                      33
#define PMU_IRQ                 35
#define IND_PIN                 36

void setup()
{
    // Set console baud rate
    SerialMon.begin(115200);
    delay(10);

    // Onboard LED light, it can be used freely
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // POWER_PIN : This pin controls the power supply of the SIM7600
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);

    // PWR_PIN ： This Pin is the PWR-KEY of the SIM7600
    // The time of active low level impulse of PWRKEY pin to power on module , type 500 ms
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    delay(500);
    digitalWrite(PWR_PIN, LOW);

    // IND_PIN: It is connected to the SIM7600 status Pin,
    // through which you can know whether the module starts normally.
    pinMode(IND_PIN, INPUT);

    attachInterrupt(IND_PIN, []() {
        detachInterrupt(IND_PIN);
        // If SIM7600 starts normally, then set the onboard LED to flash once every 1 second
        tick.attach_ms(1000, []() {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        });
    }, CHANGE);

    DBG("Wait...");

    delay(3000);

    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    DBG("Initializing modem...");
    if (!modem.init()) {
        DBG("Failed to restart modem, delaying 10s and retrying");
        return;
    }

/*
    
    2 Automatic
    13 GSM Only
    38 LTE Only
    */
    String result;
    result = modem.setNetworkMode(38);
    if (modem.waitResponse(10000L) != 1) {
        DBG(" setNetworkMode faill");
        return ;
    }



#if 0
    //https://github.com/vshymanskyy/TinyGSM/pull/405
    uint8_t mode = modem.getGNSSMode();
    Serial.print("Get GNSS Mode:");
    Serial.println(mode);

    /**
     *  CGNSSMODE: <gnss_mode>,<dpo_mode>
     *  This command is used to configure GPS, GLONASS, BEIDOU and QZSS support mode.
     *  gnss_mode:
     *      0 : GLONASS
     *      1 : BEIDOU
     *      2 : GALILEO
     *      3 : QZSS
     *  dpo_mode :
     *      0 disable
     *      1 enable
     */
    Serial.print("Set GNSS Mode BEIDOU :");
    String res = modem.setGNSSMode(1, 1);
    Serial.println(res);
    delay(1000);
#endif

}

void loop()
{
    /*
    while(1){
      while (SerialAT.available()) {
        Serial.write(SerialAT.read());
    }
    while (Serial.available()) {
        SerialAT.write(Serial.read());
    }
  }
  */
    String name = modem.getModemName();
    DBG("Modem Name:", name);

    String modemInfo = modem.getModemInfo();
    DBG("Modem Info:", modemInfo);

#if TINY_GSM_TEST_GPRS
    // Unlock your SIM card with a PIN if needed
    if ( GSM_PIN && modem.getSimStatus() != 3 ) {
        modem.simUnlock(GSM_PIN);
    }
#endif

#if TINY_GSM_TEST_GPRS && defined TINY_GSM_MODEM_XBEE
    // The XBee must run the gprsConnect function BEFORE waiting for network!
    modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

    DBG("Waiting for network...");
    if (!modem.waitForNetwork()) {
        delay(10000);
        return;
    }

    if (modem.isNetworkConnected()) {
        DBG("Network connected");
    }


#if TINY_GSM_TEST_GPRS
    DBG("Connecting to", apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        delay(10000);
        return;
    }

    bool res = modem.isGprsConnected();
    DBG("GPRS status:", res ? "connected" : "not connected");

    String ccid = modem.getSimCCID();
    DBG("CCID:", ccid);

    String imei = modem.getIMEI();
    DBG("IMEI:", imei);

    String cop = modem.getOperator();
    DBG("Operator:", cop);

    IPAddress local = modem.localIP();
    DBG("Local IP:", local);

    int csq = modem.getSignalQuality();
    DBG("Signal quality:", csq);

#endif

#if TINY_GSM_TEST_TCP 
  TinyGsmClient client(modem, 0);
  const int     port = 80;
  DBG("Connecting to", server);
  if (!client.connect(server, port)) {
    DBG("... failed");
  } else {
    // Make a HTTP GET request:
    client.print(String("GET ") + resource + " HTTP/1.0\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Connection: close\r\n\r\n");

    // Wait for data to arrive
    uint32_t start = millis();
    while (client.connected() && !client.available() &&
           millis() - start < 30000L) {
      delay(100);
    };

    // Read data
    start          = millis();
    char logo[640] = {
        '\0',
    };
    int read_chars = 0;
    while (client.connected() && millis() - start < 10000L) {
      while (client.available()) {
        logo[read_chars]     = client.read();
        logo[read_chars + 1] = '\0';
        read_chars++;
        start = millis();
      }
    }
    SerialMon.println(logo);
    DBG("#####  RECEIVED:", strlen(logo), "CHARACTERS");
    client.stop();
  }
#endif


#if TINY_GSM_TEST_GPRS
    modem.gprsDisconnect();
    if (!modem.isGprsConnected()) {
        DBG("GPRS disconnected");
    } else {
        DBG("GPRS disconnect: Failed.");
    }
#endif


#if TINY_GSM_TEST_GSM_LOCATION 
  float lat      = 0;
  float lon      = 0;
  float accuracy = 0;
  int   year     = 0;
  int   month    = 0;
  int   day      = 0;
  int   hour     = 0;
  int   min      = 0;
  int   sec      = 0;
  for (int8_t i = 15; i; i--) {
    DBG("Requesting current GSM location");
    if (modem.getGsmLocation(&lat, &lon, &accuracy, &year, &month, &day, &hour,
                             &min, &sec)) {
      DBG("Latitude:", String(lat, 8), "\tLongitude:", String(lon, 8));
      DBG("Accuracy:", accuracy);
      DBG("Year:", year, "\tMonth:", month, "\tDay:", day);
      DBG("Hour:", hour, "\tMinute:", min, "\tSecond:", sec);
      break;
    } else {
      DBG("Couldn't get GSM location, retrying in 15s.");
      delay(15000L);
    }
  }
  DBG("Retrieving GSM location again as a string");
  String location = modem.getGsmLocation();
  DBG("GSM Based Location String:", location);
#endif

#if TINY_GSM_TEST_GPS
    modem.enableGPS();

    modem.sendAT("+CGNSSPWR=1");
    if (modem.waitResponse(10000L) != 1) {
        while (1) {
            delay(1000);
            DBG("EnableGPS  false");
        }
        
    }

    float LAT,  LON;
    while (1) {
        if (modem.getGPS(&LAT, &LON)) {
            Serial.printf("LAT:%f LON:%f\n", LAT, LON);
            tick.attach_ms(200, []() {
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            });
            break;
        } else {
            Serial.print("getGPS ");
            Serial.println(millis());
        }
        delay(2000);
    }
    modem.disableGPS();
#endif


#if TINY_GSM_POWERDOWN
    // Try to power-off (modem may decide to restart automatically)
    // To turn off modem completely, please use Reset/Enable pins
    modem.poweroff();
    DBG("Poweroff.");
#endif

    // Test is complete Set it to sleep mode
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    delay(200);
    esp_deep_sleep_start();
}
