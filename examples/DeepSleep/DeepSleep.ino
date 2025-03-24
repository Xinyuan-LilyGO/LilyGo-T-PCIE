/**
 * @file      DeepSleep.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2025  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2025-03-24
 * @note      https://github.com/Xinyuan-LilyGO/LilyGo-T-PCIE/issues/44#issuecomment-2745212934
 */


#include "XPowersLib.h"         // Arduino IDE -> Library Search -> XPowersLib v0.2.7 -> Install 

#define MODEM_POWER_ON      25
#define PMU_IRQ             35
#define LED_PIN             12
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP       15          /* Time ESP32 will go to sleep (in seconds) */

XPowersAXP2101 power;
const uint8_t i2c_sda = 21;
const uint8_t i2c_scl = 22;
bool pmu_irq = false;

void setup(void)
{
    Serial.begin(115200);

    // Turn on led
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Wire.begin(i2c_sda, i2c_scl);
    if (!power.init(Wire, AXP2101_SLAVE_ADDRESS, i2c_sda, i2c_scl)) {
        while (1) {
            Serial.println("Warning: Failed to find AXP2101 power management");
            delay(1000);
        }
    }
    Serial.println("AXP2101 PMU init succeeded, using AXP2101 PMU");

    /*
      The default setting is CHGLED is automatically controlled by the PMU.
    - XPOWERS_CHG_LED_OFF,
    - XPOWERS_CHG_LED_BLINK_1HZ,
    - XPOWERS_CHG_LED_BLINK_4HZ,
    - XPOWERS_CHG_LED_ON,
    - XPOWERS_CHG_LED_CTRL_CHG,
    * */
    power.setChargingLedMode(XPOWERS_CHG_LED_OFF);

    // T-PCI V1.2 Version
    // Disable unused PMU power channels
    power.disableDC2();
    power.disableDC3();
    power.disableDC4();
    power.disableDC5();

    power.disableALDO1();
    power.disableALDO2();
    power.disableALDO3();
    power.disableALDO4();
    power.disableBLDO1();
    power.disableBLDO2();
    power.disableDLDO1();
    power.disableDLDO2();
    power.disableButtonBatteryCharge();
    power.enableCPUSLDO();


    // Disable all interrupts
    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    // Clear all interrupt flags
    power.clearIrqStatus();


    Wire.end();

    // Set PMU I2C and interrupt pin to open drain
    pinMode(i2c_sda, OPEN_DRAIN);
    pinMode(i2c_scl, OPEN_DRAIN);
    pinMode(PMU_IRQ, OPEN_DRAIN);

    // Turn off MODEM DCDC booster
    pinMode(MODEM_POWER_ON, OUTPUT);
    digitalWrite(MODEM_POWER_ON, LOW);


    //Turning off LED
    digitalWrite(LED_PIN, HIGH);
    //Enable LED hold on high level
    gpio_hold_en((gpio_num_t)LED_PIN);


    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

    Serial.println("Going to sleep now");
    Serial.flush();
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
}

void loop(void)
{

}

