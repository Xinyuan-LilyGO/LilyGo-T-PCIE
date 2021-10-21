#define TINY_GSM_MODEM_SIM800 //The SIM7020 AT instruction is compatible with SIM800C


//Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

/*
   Tests enabled
*/
#define TINY_GSM_TEST_GPRS true
#define TINY_GSM_TEST_WIFI false
#define TINY_GSM_TEST_CALL false
#define TINY_GSM_TEST_SMS false
#define TINY_GSM_TEST_USSD false
#define TINY_GSM_TEST_BATTERY false
#define TINY_GSM_TEST_GPS false
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

// Range to attempt to autobaud
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

bool get_SIM();

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
    // delay(500);
    // digitalWrite(PWR_PIN, HIGH);

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


    delay(1000);

    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

/*
    //    // Uncomment below will perform loopback test
    while (1) {
        while (SerialMon.available()) {
              SerialAT.write(SerialMon.read());
          }
          while (SerialAT.available()) {
              SerialMon.write(SerialAT.read());
          }
      }*/
      // Restart takes quite some time
      // To skip it, call init() instead of restart()
      /* DBG("Initializing modem...");
       if (!modem.init()) {
           DBG("Failed to restart modem, delaying 10s and retrying");
           return;
       }*/
}

void loop()
{
/*
    while (1) {
        while (SerialMon.available()) {
              SerialAT.write(SerialMon.read());
          }
          while (SerialAT.available()) {
              SerialMon.write(SerialAT.read());
          }
      }*/



    getModemNameImpl();
    getModemInfoImpl();
    if(get_SIM()==false){ return; }

    //modem.getSimStatus();
    delay(1000);

    DBG("Waiting for network...");
    getRegistrationStatusXREG();//getSignalQualityImpl();
    DBG("Network connected...");



         modem.sendAT(GF("+CGACT?"));
        // check for any of the three for simplicity
        if (modem.waitResponse(1000L) != 1) {
            return ;
        }
        modem.sendAT(GF("+COPS?"));
        // check for any of the three for simplicity
        if (modem.waitResponse(1000L) != 1) {
            return ;
        }
        modem.sendAT(GF("+CGCONTRDP"));
        // check for any of the three for simplicity
        if (modem.waitResponse(1000L) != 1) {
            return ;
        }

        modem.sendAT(GF("+CDNSGIP=\"www.baidu.com\""));
        // check for any of the three for simplicity
        if (modem.waitResponse(1000L) != 1) {
            return ;
        }
        delay(3000);
#if TINY_GSM_POWERDOWN
    // Try to power-off (modem may decide to restart automatically)
    // To turn off modem completely, please use Reset/Enable pins
    modem.poweroff();
    DBG("Poweroff.");
    DBG("-----------TEST OK-----------.");
#endif

    // Test is complete Set it to sleep mode
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    delay(200);
    esp_deep_sleep_start();
}

bool get_SIM()
{

    modem.sendAT("+CPIN?");
    if (modem.waitResponse(10000L) != 1) {

         delay(1000);
        DBG(" SIM card not inserted ");
        return false;
    }

    DBG(" SIM card READY");
    return true;
}


String getModemNameImpl()
{
    String name = "";


    modem.sendAT(GF("+GMM"));
    String res2;
    if (modem.waitResponse(1000L, res2) != 1) {
        return name;
    }
    res2.replace(GSM_NL "OK" GSM_NL, "");
    res2.replace("_", " ");
    res2.trim();

    name = res2;
    DBG("### Modem:", name);
    return name;
}


String getModemInfoImpl()
{
    modem.sendAT(GF("+GMR"));
    String res;
    if (modem.waitResponse(1000L, res) != 1) {
        return "";
    }
    res.replace(GSM_NL "OK" GSM_NL, "");
    res.replace(GSM_NL, " ");
    res.trim();
    DBG("### Modem Info:", res);
    return res;
}

inline int16_t streamGetIntBefore(char lastChar)
{
    char   buf[7];
    size_t bytesRead = modem.stream.readBytesUntil(
                           lastChar, buf, static_cast<size_t>(7));
    // if we read 7 or more bytes, it's an overflow
    if (bytesRead && bytesRead < 7) {
        buf[bytesRead] = '\0';
        int16_t res    = atoi(buf);
        return res;
    }

    return -9999;
}
inline bool streamSkipUntil(const char c, const uint32_t timeout_ms = 1000L)
{
    uint32_t startMillis = millis();
    while (millis() - startMillis < timeout_ms) {
        while (millis() - startMillis < timeout_ms &&
                !modem.stream.available()) {
            TINY_GSM_YIELD();
        }
        if (modem.stream.read() == c) {
            return true;
        }
    }
    return false;
}

// Gets signal quality report according to 3GPP TS command AT+CSQ
int8_t getSignalQualityImpl()
{
    modem.sendAT(GF("+CSQ"));
    if (modem.waitResponse(1000L, GF("+CSQ:")) != 1) {
        return 99;
    }
    int8_t res = streamGetIntBefore(',');
    modem.waitResponse();
    DBG("### Signal Quality :", res);
    return res;
}

// Gets the modem's registration status via CREG/CGREG/CEREG

int8_t getRegistrationStatusXREG()
{
    int status = 0;
    do {
        modem.sendAT(GF("+CGREG?"));
        // check for any of the three for simplicity
        if (modem.waitResponse(1000L,GF("+CGREG:")) != 1) {
            return 99;
        }
        streamSkipUntil(','); /* Skip format (0) */
        status = streamGetIntBefore('\n');
        modem.waitResponse();
        if (status != 1) {
            DBG("### Registration Status :", status);
            delay(1000);
        }
    } while (status != 1);

    DBG("### Registration Status :", status);
    return status;
}

