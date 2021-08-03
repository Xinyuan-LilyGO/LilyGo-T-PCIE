
## Quick Step

1. To solder the USB to the upgrade solder joint, you can choose a flying lead or directly solder the USB to the contact. 
2. Power on the T-PCIE board, and at the same time connect the USB interface to the computer port (please note that you need to insert the SIM card during the upgrade process) 
    ![](../image/update_simxxxx_1.png)

3.  [SIM7080 Driver](https://github.com/Xinyuan-LilyGO/LilyGo-T-PCIE/tree/master/update_simxxxx_firmware/USB_driver)
4. Open the computer device manager and follow the steps below to add the driver. 
    ![](../image/update_simxxxx_2.png)
    ![](../image/update_simxxxx_3.png)
    ![](../image/update_simxxxx_4.png)
    ![](../image/update_sim7080_5.png)
    ![](../image/update_simxxxx_6.png)

    Follow the above steps to install the driver for the remaining ports that are not installed.
    ![](../image/update_simxxxx_7.png)

5. [upgrade_tool](https://github.com/Xinyuan-LilyGO/LilyGo-T-PCIE/tree/master/update_simxxxx_firmware/upgrade_tool/SIM7080_SIM7500_SIM7600_SIM7900_SIM8200%20QDL%20V1.58%20Only%20for%20Update)
6. Open `sim7080_sim7500_sim7600_sim7900_sim8200 qdl v1.58 only for update.exe` 
7.  Open the upgrade tool and follow the diagram below 

    ![](../image/update_simxxxx_8.png)
    ![](../image/update_simxxxx_9.png)
    ![](../image/update_simxxxx_10.png)
    ![](../image/update_simxxxx_11.png)
    ![](../image/update_simxxxx_12.png)
    ![](../image/update_simxxxx_13.png)
    ![](../image/update_simxxxx_15.png)



8. Open the serial terminal tool, or the built-in serial tool of `Arduino IDE`, select `AT Port` for the port, and enter `AT+CGMR` to view the firmware version 
    ![](../image/update_simxxxx_14.png)



