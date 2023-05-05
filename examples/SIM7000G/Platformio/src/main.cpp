#define TINY_GSM_MODEM_SIM7000
// #define TINY_GSM_MODEM_SIM7600

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
#define TINY_GSM_TEST_CALL false
#define TINY_GSM_TEST_SMS false
#define TINY_GSM_TEST_USSD false
#define TINY_GSM_TEST_BATTERY false
#define TINY_GSM_TEST_GPS true
// powerdown modem after tests
#define TINY_GSM_POWERDOWN true

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]  = "YourAPN";
const char gprsUser[] = "";
const char gprsPass[] = "";

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
#define IND_PIN                 36


void setup()
{
    // Set console baud rate
    SerialMon.begin(115200);
    delay(10);

    // Onboard LED light, it can be used freely
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // POWER_PIN : This pin controls the power supply of the Modem
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);

    // PWR_PIN ： This Pin is the PWR-KEY of the Modem
    // The time of active low level impulse of PWRKEY pin to power on module , type 500 ms
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    delay(500);
    digitalWrite(PWR_PIN, LOW);

    // IND_PIN: It is connected to the Modem status Pin,
    // through which you can know whether the module starts normally.
    pinMode(IND_PIN, INPUT);

    attachInterrupt(IND_PIN, []() {
        detachInterrupt(IND_PIN);
        // If Modem starts normally, then set the onboard LED to flash once every 1 second
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

    //Set to GSM mode, please refer to manual 5.11 AT+CNMP Preferred mode selection for more parameters
    bool result;
    do {
        result = modem.setNetworkMode(13);
        delay(500);
    } while (result != "OK");

    /*
     2 Automatic
     13 GSM only
     38 LTE only
     51 GSM and LTE only
     * * * */
    String res;
    do {
        res = modem.setNetworkMode(13);
        delay(500);
    } while (res != "OK");

    /*
    1 CAT-M
    2 NB-Iot
    3 CAT-M and NB-IoT
    * * */
    do {
        res = modem.setPreferredMode(2);
        delay(500);
    } while (res != "OK");
}

void loop()
{
    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    DBG("Initializing modem...");
    if (!modem.restart()) {
        DBG("Failed to restart modem, delaying 10s and retrying");
        return;
    }

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

#if TINY_GSM_TEST_GPRS
    modem.gprsDisconnect();
    if (!modem.isGprsConnected()) {
        DBG("GPRS disconnected");
    } else {
        DBG("GPRS disconnect: Failed.");
    }
#endif

#if TINY_GSM_TEST_GPS

    // Set SIM7000G GPIO4 HIGH ,Open GPS power
    // CMD:AT+SGPIO=0,4,1,1
    modem.sendAT("+SGPIO=0,4,1,1");
    if (modem.waitResponse(20000L) != 1) {
        DBG(" AT+SGPIO=0,4,1,1 faill");
        return ;
    }
    modem.enableGPS();
    float lat,  lon;
    while (1) {
        if (modem.getGPS(&lat, &lon)) {
            Serial.printf("lat:%f lon:%f\n", lat, lon);
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

    // Set SIM7000G GPIO4 HIGH ,Close GPS power
    // CMD:AT+SGPIO=0,4,1,0
    modem.sendAT("+SGPIO=0,4,1,0");
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
