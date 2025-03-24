/**
 * @file      PMUExample.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-06-17
 *
 */
#include "XPowersLib.h"

#ifndef PMU_WIRE_PORT
#define PMU_WIRE_PORT   Wire
#endif

XPowersLibInterface *PMU = NULL;
const uint8_t i2c_sda = 21;
const uint8_t i2c_scl = 22;
const uint8_t PMU_IRQ = 35;
bool pmu_irq = false;

void setup(void)
{
    Serial.begin(115200);

    Wire.begin(i2c_sda, i2c_scl);

    if (!PMU) {
        PMU = new XPowersAXP2101(PMU_WIRE_PORT);
        if (!PMU->init()) {
            Serial.println("Warning: Failed to find AXP2101 power management");
            delete PMU;
            PMU = NULL;
        } else {
            Serial.println("AXP2101 PMU init succeeded, using AXP2101 PMU");

            // Set the minimum common working voltage of the PMU VBUS input,
            // below this value will turn off the PMU
            PMU->setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);

            // Set the maximum current of the PMU VBUS input,
            // higher than this value will turn off the PMU
            PMU->setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_2000MA);
        }
    }

    if (!PMU) {
        PMU = new XPowersAXP192(PMU_WIRE_PORT);
        if (!PMU->init()) {
            Serial.println("Warning: Failed to find AXP192 power management");
            delete PMU;
            PMU = NULL;
        } else {
            Serial.println("AXP192 PMU init succeeded, using AXP192 PMU");
        }
    }

    while (!PMU) {
        Serial.println("The address of the power management device was not found. The power communication of this board failed! Please check.");
        delay(1000);
    }

    // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
    PMU->setSysPowerDownVoltage(2600);

    /*
     * The charging indicator can be turned on or off
     * * * */
    PMU->setChargingLedMode(XPOWERS_CHG_LED_BLINK_1HZ);


    pinMode(PMU_IRQ, INPUT_PULLUP);
    attachInterrupt(PMU_IRQ, [] {
        pmu_irq = true;
    }, FALLING);


    //T-PCI V1.1 Version use AXP192
    if (PMU->getChipModel() == XPOWERS_AXP192) {
        /*
        * The default ESP32 power supply has been turned on,
        * no need to set, please do not set it, if it is turned off,
        * it will not be able to program
        * * * */
        //protected esp32 power source
        PMU->setProtectedChannel(XPOWERS_DCDC3);
        PMU->disablePowerOutput(XPOWERS_DCDC2);
        PMU->disablePowerOutput(XPOWERS_DCDC3);
        PMU->disablePowerOutput(XPOWERS_LDO1);
        PMU->disablePowerOutput(XPOWERS_LDO2);


        //T-PCI V1.2 Version use AXP2101
    } else if (PMU->getChipModel() == XPOWERS_AXP2101) {
        /*
         * The default ESP32 power supply has been turned on,
         * no need to set, please do not set it, if it is turned off,
         * it will not be able to program
         * * * */
        //ESP32 VDD 3300mV ， protected esp32 power source
        PMU->setProtectedChannel(XPOWERS_DCDC1);

        //Unuse power channel
        PMU->disablePowerOutput(XPOWERS_DCDC2);
        PMU->disablePowerOutput(XPOWERS_DCDC3);
        PMU->disablePowerOutput(XPOWERS_DCDC4);
        PMU->disablePowerOutput(XPOWERS_DCDC5);
        PMU->disablePowerOutput(XPOWERS_ALDO1);
        PMU->disablePowerOutput(XPOWERS_ALDO4);
        PMU->disablePowerOutput(XPOWERS_BLDO1);
        PMU->disablePowerOutput(XPOWERS_BLDO2);
        PMU->disablePowerOutput(XPOWERS_DLDO1);
        PMU->disablePowerOutput(XPOWERS_DLDO2);
        PMU->disablePowerOutput(XPOWERS_VBACKUP);
    }

    PMU->clearIrqStatus();

    PMU->disableInterrupt(XPOWERS_ALL_INT);

    PMU->enableInterrupt(XPOWERS_USB_INSERT_INT |
                         XPOWERS_USB_REMOVE_INT |
                         XPOWERS_BATTERY_INSERT_INT |
                         XPOWERS_BATTERY_REMOVE_INT |
                         XPOWERS_PWR_BTN_CLICK_INT |
                         XPOWERS_PWR_BTN_LONGPRESSED_INT);

    // Set the time of pressing the button to turn off
    PMU->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);


}


void printPmuInfo()
{
    static uint32_t lastUpdate = 0;
    if (millis() > lastUpdate) {
        lastUpdate = millis() + 3000;
        Serial.print("isCharging:"); Serial.println(PMU->isCharging() ? "YES" : "NO");
        Serial.print("isDischarge:"); Serial.println(PMU->isDischarge() ? "YES" : "NO");
        Serial.print("isVbusIn:"); Serial.println(PMU->isVbusIn() ? "YES" : "NO");
        Serial.print("getBattVoltage:"); Serial.print(PMU->getBattVoltage()); Serial.println("mV");
        Serial.print("getVbusVoltage:"); Serial.print(PMU->getVbusVoltage()); Serial.println("mV");
        Serial.print("getSystemVoltage:"); Serial.print(PMU->getSystemVoltage()); Serial.println("mV");

        // The battery percentage may be inaccurate at first use, the PMU will automatically
        // learn the battery curve and will automatically calibrate the battery percentage
        // after a charge and discharge cycle
        if (PMU->isBatteryConnect()) {
            Serial.print("getBatteryPercent:"); Serial.print(PMU->getBatteryPercent()); Serial.println("%");
        }
        Serial.println();
    }
}

void loop(void)
{
    if (!PMU) {
        Serial.println("Error! PMU not found!"); delay(1000);
        return;
    }

    printPmuInfo();

    if (pmu_irq) {
        uint64_t status =  PMU->getIrqStatus();
        // Get PMU Interrupt Status Register
        Serial.print("STATUS => HEX:");
        Serial.print(status, HEX);
        Serial.print(" BIN:");
        Serial.println(status, BIN);

        if (PMU->isVbusInsertIrq()) {
            Serial.println("isVbusInsert");
        }
        if (PMU->isVbusRemoveIrq()) {
            Serial.println("isVbusRemove");
        }
        if (PMU->isBatInsertIrq()) {
            Serial.println("isBatInsert");
        }
        if (PMU->isBatRemoveIrq()) {
            Serial.println("isBatRemove");
        }
        if (PMU->isPekeyShortPressIrq()) {
            Serial.println("isPekeyShortPress");
        }
        if (PMU->isPekeyLongPressIrq()) {
            Serial.println("isPekeyLongPress");
        }
        if (PMU->isBatChargeDoneIrq()) {
            Serial.println("isBatChargeDoneIrq");
        }
        if (PMU->isBatChargeStartIrq()) {
            Serial.println("isBatChargeStartIrq");
        }
        // Clear PMU Interrupt Status Register
        PMU->clearIrqStatus();
        pmu_irq = false;
    }
}

