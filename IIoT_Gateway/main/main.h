///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file main.h
///
/// @note Project    : IIoT_Client/Gateway
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef main
#define main

/* Includes ------------------------------------------------------------------*/
#include "MyTypes.h"
#include "D_GPIO.h"

// #include "esp32-hal.h"
#include "lwip/apps/sntp.h"
#include "esp_netif.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "BLEmesh_SensorClient.h"

#include "D_AWS_MQTT.h"

// === External references ========================================================================

// === Definition of macro / constants ============================================================
// Defines
#define MAIN_TAG "main"

#define INIT_SYS_DELAY_MS       1000

#define VIBRATION_DATA_LENGTH   128
#define VIBRATION_DATA_SEGMENTS 4

    // Unix time
#define CONFIG_TIME_UNIX_OFFSET 946684800
#define CONFIG_TIME_TRIES       3
#define CONFIG_TIME_DELAY       5000

    // Task
#define REQUEST_SENSOR_SERVER_DATA_TASK_STACK_SIZE  4096
#define SEND_MQTT_TASK_STACK_SIZE                   8192
#define UPDATE_TIME_TASK_STACK_SIZE                 4096

#define REQUEST_SENSOR_SERVER_DATA_TASK_DELAY_MS    30000
#define SEND_MQTT_TASK_DELAY_MS                     2000
#define UPDATE_TIME_TASK_DELAY_MS                   (24*60*60*1000)

#define TASK_ASYNC_DELAY_MS     10000

#define BLE_SEM_WAIT_TIME       1000

#define BLE_REC_SEG_WAIT_TIME   6000
#define BLE_REC_FAIL_AMOUNT     3

// === Definition of global variables =============================================================


// === Definition of classes/functions ============================================================


// External references
extern SemaphoreHandle_t xVibrationSemaphore;
extern tBMX160_AccelType tBMX160_Accel[VIBRATION_DATA_LENGTH];

#endif // end main

// END OF FILE