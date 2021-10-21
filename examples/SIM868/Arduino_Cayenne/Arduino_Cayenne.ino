#define TINY_GSM_MODEM_SIM868

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Range to attempt to autobaud
// NOTE:  DO NOT AUTOBAUD in production code.  Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 57600

#define USE_GSM  //! Uncomment will use SIM7000 for GSM communication

#ifdef USE_GSM
#include <CayenneMQTTGSM.h>
#else
#include <CayenneMQTTESP32.h>
#endif

#include <TinyGsmClient.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <Ticker.h>
Ticker tick;

#define TEMPERATURE_VIRTUAL_CHANNEL         1
#define BAROMETER_VIRTUAL_CHANNEL           2
#define ALTITUDE_VIRTUAL_CHANNEL            3
#define BATTERY_VIRTUAL_CHANNEL             4
#define LIGHTSENSOR_VIRTUAL_CHANNEL         5

#define PIN_TX      27
#define PIN_RX      26
#define UART_BAUD   115200
#define PWR_PIN     4
#define LED_PIN     12
#define POWER_PIN   25
#define IND_PIN     36

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif


Adafruit_BMP085 bmp;

#ifdef USE_GSM
// GSM connection info.
char apn[] = ""; // Access point name. Leave empty if it is not needed.
char gprsLogin[] = ""; // GPRS username. Leave empty if it is not needed.
char gprsPassword[] = ""; // GPRS password. Leave empty if it is not needed.
char pin[] = ""; // SIM pin number. Leave empty if it is not needed.
#else
// WiFi network info.
char ssid[] = "your wifi ssid";
char wifiPassword[] = "your wifi password";
#endif

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "Cayenne username";
char password[] = "Cayenne password";
char clientID[] = "Cayenne clientID";

bool bmpSensorDetected = true;



void setup()
{
    Serial.begin(UART_BAUD);
   SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);


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


    // Launch BMP085
    if (!bmp.begin()) {
        bmpSensorDetected = false;
        Serial.println("Could not find a valid BMP085 sensor, check wiring!");
        while (1) {}
    }


      // Restart takes quite some time
  // To skip it, call init() instead of restart()
  DBG("Initializing modem...");
  if (!modem.init()) {
    DBG("Failed to restart modem, delaying 10s and retrying");
    // restart autobaud in case GSM just rebooted
    // TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
    return;
  }

#ifdef USE_GSM
    Cayenne.begin(username, password, clientID, SerialAT, apn, gprsLogin, gprsPassword, pin);
#else
    Cayenne.begin(username, password, clientID, ssid, wifiPassword);
#endif
}


void loop()
{
    Cayenne.loop();
}




// Default function for processing actuator commands from the Cayenne Dashboard.
// You can also use functions for specific channels, e.g CAYENNE_IN(1) for channel 1 commands.
CAYENNE_IN_DEFAULT()
{
    CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
    //Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
}

CAYENNE_IN(1)
{
    CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
}


// This function is called at intervals to send temperature sensor data to Cayenne.
CAYENNE_OUT(TEMPERATURE_VIRTUAL_CHANNEL)
{

    if (bmpSensorDetected) {
        float temperature = bmp.readTemperature();

        Serial.print("Temperature = ");
        Serial.print(temperature);
        Serial.println(" *C");

        Cayenne.celsiusWrite(TEMPERATURE_VIRTUAL_CHANNEL, temperature);
    }
}

// This function is called at intervals to send barometer sensor data to Cayenne.
CAYENNE_OUT(BAROMETER_VIRTUAL_CHANNEL)
{
    if (bmpSensorDetected) {
        float pressure = bmp.readPressure() / 1000;

        Serial.print("Pressure = ");
        Serial.print(pressure );
        Serial.println(" hPa");

        Cayenne.hectoPascalWrite(BAROMETER_VIRTUAL_CHANNEL, pressure);
    }
}


CAYENNE_OUT(ALTITUDE_VIRTUAL_CHANNEL)
{
    if (bmpSensorDetected) {
        float altitude = bmp.readAltitude();

        Serial.print("Altitude = ");
        Serial.print(altitude);
        Serial.println(" meters");

        Cayenne.virtualWrite(ALTITUDE_VIRTUAL_CHANNEL, altitude, "meters", UNIT_METER);

    }
}


CAYENNE_OUT(BATTERY_VIRTUAL_CHANNEL)
{
    
  uint8_t  chargeState = -99;
  int8_t   percent     = -99;
  uint16_t milliVolts  = -9999;
  modem.getBattStats(chargeState, percent, milliVolts);
  DBG("Battery charge state:", chargeState);
  DBG("Battery charge 'percent':", percent);
  DBG("Battery voltage:", milliVolts / 1000.0F);
   Cayenne.virtualWrite(BATTERY_VIRTUAL_CHANNEL, milliVolts, TYPE_VOLTAGE, UNIT_MILLIVOLTS);

}

