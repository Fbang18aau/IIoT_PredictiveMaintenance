///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file main.h
///
/// @note Project    : IIoT_SensorUnit
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef main
#define main

#include <stdio.h>
#include <sys/time.h>

#include "BLEmesh_SensorServer.h"
#include "D_I2C.h"
#include "D_BMX160.h"

// === External references ========================================================================
extern uint8_t u8SensorStatus;
extern uint32_t GetTime_ms(void);
extern uint32_t u32BLEmeshTimeStart_ms;

// === Definition of macro / constants ============================================================

// Defines
#define VIBRATION_DATA_LENGTH 128

#define MAIN_TAG "main"

    // Task
#define UPDATE_SENSOR_DATA_TASK_STACK_SIZE  4096
#define UPDATE_SENSOR_DATA_TASK_DELAY_MS    2000

    // Motor
#define MOTOR_WAIT_MS       20000

    // Sample data
#define SAMPLE_INTERVAL_US  8000
#define SAMPLE_TIME_US      352
#define SAMPLE_TAKE_WAIT_MS 6000
#define SAMPLE_DELAY_US     SAMPLE_INTERVAL_US-SAMPLE_TIME_US

    // BLE Mesh
#define BLE_REQ_SEG_WAIT_TIME   5000

// === Definition of global variables =============================================================


// === Definition of classes/functions ============================================================


// External references
extern SemaphoreHandle_t xVibrationSemaphore;

#endif // end main

// END OF FILE