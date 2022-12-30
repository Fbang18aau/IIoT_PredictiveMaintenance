///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file BLEmesh_SensorServer.h
///
/// @note Project    : IIoT_SensorUnit
/// @note Subsystem  : BLEmesh
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BLEmesh_SensorServer
#define BLEmesh_SensorServer

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_sensor_model_api.h"

#include "ble_mesh_example_init.h"
#include "board.h"

#include "D_BMX160.h"

// === External references ========================================================================
extern  void BLEmesh_SensorServer_Init(void);
extern  void BLEmesh_SensorServer_Update_VibrationData(tBMX160_AccelType, uint16_t);

// === Definition of macro / constants ============================================================
// Defines

#define BLEMESH_SENSOR_SERVER_TAG       "BLEmesh_SensorServer"

#define BLEMESH_SENSOR_SERVER_CID_ESP   0x02E5

    // Sensor Property ID
#define BLEMESH_SENSOR_SERVER_VIBRATION_X_ID        0x0090
#define BLEMESH_SENSOR_SERVER_VIBRATION_Y_ID        0x0091
#define BLEMESH_SENSOR_SERVER_VIBRATION_Z_ID        0x0092 
#define BLEMESH_SENSOR_SERVER_STATUS_ID             0x0093

    // Sensor setup parameters
#define BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH             64
#define BLEMESH_SENSOR_SERVER_SHIFT_DATA_LENGTH                 1
#define BLEMESH_SENSOR_SERVER_VIBRATION_POSITIVE_TOLERANCE      ESP_BLE_MESH_SENSOR_UNSPECIFIED_POS_TOLERANCE
#define BLEMESH_SENSOR_SERVER_VIBRATION_NEGATIVE_TOLERANCE      ESP_BLE_MESH_SENSOR_UNSPECIFIED_NEG_TOLERANCE
#define BLEMESH_SENSOR_SERVER_VIBRATION_SAMPLE_FUNCTION         ESP_BLE_MESH_SAMPLE_FUNC_UNSPECIFIED
#define BLEMESH_SENSOR_SERVER_VIBRATION_MEASURE_PERIOD          ESP_BLE_MESH_SENSOR_NOT_APPL_MEASURE_PERIOD
#define BLEMESH_SENSOR_SERVER_VIBRATION_UPDATE_INTERVAL         ESP_BLE_MESH_SENSOR_NOT_APPL_UPDATE_INTERVAL

    // Sensor setup parameters
#define BLEMESH_SENSOR_SERVER_STATUS_DATA_LENGTH             1
#define BLEMESH_SENSOR_SERVER_STATUS_POSITIVE_TOLERANCE      ESP_BLE_MESH_SENSOR_UNSPECIFIED_POS_TOLERANCE
#define BLEMESH_SENSOR_SERVER_STATUS_NEGATIVE_TOLERANCE      ESP_BLE_MESH_SENSOR_UNSPECIFIED_NEG_TOLERANCE
#define BLEMESH_SENSOR_SERVER_STATUS_SAMPLE_FUNCTION         ESP_BLE_MESH_SAMPLE_FUNC_UNSPECIFIED
#define BLEMESH_SENSOR_SERVER_STATUS_MEASURE_PERIOD          ESP_BLE_MESH_SENSOR_NOT_APPL_MEASURE_PERIOD
#define BLEMESH_SENSOR_SERVER_STATUS_UPDATE_INTERVAL         ESP_BLE_MESH_SENSOR_NOT_APPL_UPDATE_INTERVAL

    // Data transfer parameters
#define BLEMESH_SENSOR_SERVER_OCTETS    20

    // Semaphore
#define BLEMESH_SENSOR_SERVER_UPDATE_TAKE_WAIT_MS   100
#define BLEMESH_SENSOR_SERVER_SEMAPHORE_NOT_OWNED   0
#define BLEMESH_SENSOR_SERVER_SEMAPHORE_OWNED       1

// === Definition of global variables =============================================================


// === Definition of classes/functions ============================================================



// External references


#endif // end BLEmesh_SensorServer

// END OF FILE