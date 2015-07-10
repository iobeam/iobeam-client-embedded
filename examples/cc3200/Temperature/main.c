//*****************************************************************************
//
// Copyright (C) 2015 iobeam - https://www.iobeam.com
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup iobeam
//! @{
//
//****************************************************************************

// Standard includes
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Includes to setup board
#include "i2c_if.h"
#include "pinmux.h"
#include "tmp006drv.h"
#include "utils.h"
#include "udma_if.h"
#include "wifi.h"

// iobeam includes
#define API_DEFAULT_SERVER "api.iobeam.com"
#define DEBUG_LEVEL 2
#include "include/iobeam_log.h"
#include "include/cc3200/iobeam.h"
//#define NEW_ID 1  // Uncomment to get a new device ID.


#define APPLICATION_NAME        "iobeam temperature tracker"
#define APPLICATION_VERSION     "1.0.0"
#define MEASURE_DELAY_SECS       5

// iobeam constants
const char *PROJECT_TOKEN = "YOUR PROJECT TOKEN";
const uint32_t PROJECT_ID = 0;  // YOUR PROJECT ID
const char *DEVICE_ID = NULL;


static void Init()
{
    long lRetVal = -1;
    BoardInit();
    UDMAInit();
    PinMuxConfig();
    InitTerm();

    InitializeAppVariables();

    //
    // Following function configure the device to default state by cleaning
    // the persistent settings stored in NVMEM (viz. connection profiles &
    // policies, power policy etc)
    //
    // Applications may choose to skip this step if the developer is sure
    // that the device is in its default state at start of applicaton
    //
    // Note that all profiles and persistent settings that were done on the
    // device will be lost
    //
    lRetVal = ConfigureSimpleLinkToDefaultState();

    if (lRetVal < 0) {
        if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
            UART_PRINT(
                    "Failed to configure the device in its default state \n\r");

        LOOP_FOREVER()
        ;
    }

    //
    // Asumption is that the device is configured in station mode already
    // and it is in its default state
    //
    lRetVal = sl_Start(0, 0, 0);
    if (lRetVal < 0) {
        UART_PRINT("Failed to start the device \n\r");
        LOOP_FOREVER()
        ;
    }

    UART_PRINT("Connecting to AP: '%s'...\r\n", SSID_NAME);

    // Connecting to WLAN AP - Set with static parameters defined at common.h
    // After this call we will be connected and have IP address
    lRetVal = WlanConnect();
    if (lRetVal < 0) {
        UART_PRINT("Connection to AP failed \n\r");
        LOOP_FOREVER()
        ;
    }

    UART_PRINT("Connected to AP: '%s' \n\r", SSID_NAME);

#ifdef NEW_ID
    iobeam_Reset();
#endif
}

inline int initTemperatureSensor() {
    int ret = I2C_IF_Open(I2C_MASTER_MODE_FST);
    if (ret < 0) {
        UART_PRINT("i2c failure\r\n");
        return ret;
    }
    return TMP006DrvOpen();
}

inline int getTemperature(float *temperature)
{
    return TMP006DrvGetTemp(temperature);
}

// UtilsDelay uses loop counts which takes 3-5 CPU ticks on 80Mhz, this macro
// is a rough conversion from seconds.
#define SECS_TO_LOOPS(x) ((80000000 / 5) * x)

inline void delay(uint16_t seconds) {
    UtilsDelay(SECS_TO_LOOPS(seconds));
}

//****************************************************************************
//                            MAIN FUNCTION
//****************************************************************************
void main()
{
    Init();

    int ret = initTemperatureSensor();
    if (ret == 0) {  // 0 is success
        IOBEAM_LOG("Temperature sensor initialized.\r\n");

        Iobeam iobeam;
        ret = iobeam_Init(&iobeam, PROJECT_ID, PROJECT_TOKEN, DEVICE_ID);
        if (ret < 0) {
            IOBEAM_ERR("iobeam init failed, quitting...\r\n");
            goto err_iobeam;
        }
        iobeam.RegisterDevice();
        iobeam.StartTimeKeeping();

        float temperature = 0.0;
        while (1) {
            IOBEAM_LOG("---------\r\n");
            ret = getTemperature(&temperature);
            if (ret < 0) {
                IOBEAM_ERR("Error getting temp: %d\r\n", ret);
                break;
            }
            IOBEAM_LOG("temperature: %f\r\n", temperature);
            ret = iobeam.SendFloat("temperature", temperature);
            IOBEAM_LOG("success: %d\r\n", ret);

            delay(MEASURE_DELAY_SECS);
        }

        iobeam_Finish();
    }

err_iobeam:
    sl_Stop(SL_STOP_TIMEOUT);

    IOBEAM_LOG("Exiting Application ...\n\r");
    while (1) {
        _SlNonOsMainLoopTask();
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
