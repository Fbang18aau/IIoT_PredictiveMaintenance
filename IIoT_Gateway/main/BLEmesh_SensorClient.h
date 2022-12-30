///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file BLEmesh_SensorClient.h
///
/// @note Project    : IIoT_Client/Gateway
/// @note Subsystem  : BLEmesh
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BLEmesh_SensorClient
#define BLEmesh_SensorClient

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "MyTypes.h"
#include "D_GPIO.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_sensor_model_api.h"

#include "ble_mesh_example_init.h"

// === External references ========================================================================
extern void BLEmesh_SensorClient_Init(void);
extern void example_ble_mesh_send_sensor_message(uint32_t opcode);
extern void BLEmesh_SensorClient_Reset_VibrationData(void);

extern uint8_t u8SensorStatusBLEmesh;

// === Definition of macro / constants ============================================================
// Defines
#define BLEMESH_SENSOR_CLIENT_TAG "BLEmesh_SensorClient"

#define CID_ESP             0x02E5

#define PROV_OWN_ADDR       0x0001

#define VIBRATION_X_ID  0x90
#define VIBRATION_Y_ID  0x91
#define VIBRATION_Z_ID  0x92
#define STATUS_ID       0x93

#define MSG_SEND_TTL        3
#define MSG_SEND_REL        false
#define MSG_TIMEOUT         0
#define MSG_ROLE            ROLE_PROVISIONER

#define COMP_DATA_PAGE_0    0x00

#define APP_KEY_IDX         0x0000
#define APP_KEY_OCTET       0x12

#define COMP_DATA_1_OCTET(msg, offset)      (msg[offset])
#define COMP_DATA_2_OCTET(msg, offset)      (msg[offset + 1] << 8 | msg[offset])


// === Definition of global variables =============================================================


// === Definition of classes/functions ============================================================

// Struct typedef

// External references
extern  uint8_t u8ProvFlag;
extern  uint8_t u8BLEmeshSegRecFlag;

extern  tBMX160_AccelType  BLEmesh_SensorClient_Update_VibrationData(uint16_t u16idx);

#endif // end BLEmesh_SensorClient

// END OF FILE