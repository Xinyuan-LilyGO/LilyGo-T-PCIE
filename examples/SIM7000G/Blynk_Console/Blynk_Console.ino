/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  Attention! Please check out TinyGSM guide:
    https://tiny.cc/tinygsm-readme

  Change GPRS apm, user, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!

 *************************************************************/

/* Fill-in your Template ID (only if using Blynk.Cloud) */
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_DEVICE_NAME ""
#define BLYNK_AUTH_TOKEN "";

// Select your modem:
#define TINY_GSM_MODEM_SIM7000

// Default heartbeat interval for GSM is 60
// If you want override this value, uncomment and set this option:
//#define BLYNK_HEARTBEAT 30

#include <TinyGsmClient.h>
#include <BlynkSimpleTinyGSM.h>

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;
BlynkTimer timer;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = BLYNK_AUTH_TOKEN;

// Your GPRS credentials
// Leave empty, if missing user or pass
char apn[]  = "YourAPN";
char user[] = "";
char pass[] = "";

#define SerialAT Serial1
#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4
#define LED_PIN     12   
#define BAT_ADC     35
#define POWER_PIN   25
#define IND_PIN     36

bool reply = false;

TinyGsm modem(SerialAT);

BLYNK_WRITE(V3) {
  if(param.asInt()==1)
  {

   digitalWrite(LED_PIN, LOW);
   Blynk.logEvent("LED STATE", "OFF");//Sending Events
  }
  else{
    digitalWrite(LED_PIN, HIGH);
     Blynk.logEvent("LED STATE", "ON");//Sending Events
  }
}

//Syncing the output state with the app at startup
BLYNK_CONNECTED()
{
Blynk.syncVirtual(V3);  // will cause BLYNK_WRITE(V0) to be executed
}

 // This function sends Arduino's up time every second to Virtual Pin (5).
// In the app, Widget's reading frequency should be set to PUSH. This means
// that you define how often to send data to Blynk App.
void sendSensor()
{
  float h = bmp.readPressure() / 1000;
  float t = bmp.readTemperature(); // or dht.readTemperature(true) for Fahrenheit


  
    uint8_t  chargeState = -99;
  int8_t   percent     = -99;
  uint16_t milliVolts  = -9999;
  modem.getBattStats(chargeState, percent, milliVolts);
  /*DBG("Battery charge state:", chargeState);
  DBG("Battery charge 'percent':", percent);
  DBG("Battery voltage:", milliVolts / 1000.0F);*/

  Serial.print("mv :");  Serial.println(percent);
 Serial.print("Pressure :");  Serial.println(h);
 Serial.print("Temperature :");  Serial.println(t);

  if (isnan(h) || isnan(t)|| isnan(percent)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  // You can send any value at any time.
  // Please don't send more that 10 values per second.

  Blynk.virtualWrite(V0, t); 
  Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V2, percent);
}




void setup() {
  Serial.begin(115200); // Set console baud rate
  SerialAT.begin(115200 , SERIAL_8N1, PIN_RX, PIN_TX);
  delay(100);

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


    DBG("Wait...");
    delay(3000);


   // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  if (!modem.restart()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  String name = modem.getModemName();
  delay(500);
  Serial.println("Modem Name: " + name);

      // Launch BMP085
    if (!bmp.begin()) {
        Serial.println("Could not find a valid BMP085 sensor, check wiring!");
        while (1) {}
    }


  
   Blynk.begin(auth, modem, apn, user, pass);
   // Setup a function to be called every second
  timer.setInterval(2000L, sendSensor);
}

void loop() {
 
  Blynk.run();
  timer.run();

}
