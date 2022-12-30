///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file BLEmesh_SensorServer.c
///
/// @note Project    : IIoT_SensorUnit
/// @note Subsystem  : BLEmesh
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "main.h"

// === Definition of macro / constants ============================================================

    // Device UUID
static uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN] = { 0x32, 0x10 };

    // BLE mesh server config
static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_ENABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};

    // Init NET BUF for each sensor
NET_BUF_SIMPLE_DEFINE_STATIC(nbVibration_X, BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH + BLEMESH_SENSOR_SERVER_SHIFT_DATA_LENGTH);
NET_BUF_SIMPLE_DEFINE_STATIC(nbVibration_Y, BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH + BLEMESH_SENSOR_SERVER_SHIFT_DATA_LENGTH);
NET_BUF_SIMPLE_DEFINE_STATIC(nbVibration_Z, BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH + BLEMESH_SENSOR_SERVER_SHIFT_DATA_LENGTH);
NET_BUF_SIMPLE_DEFINE_STATIC(nbStatus, BLEMESH_SENSOR_SERVER_STATUS_DATA_LENGTH);

    // Setup Sensors for Vibrations X-, Y- and Z-axis
static esp_ble_mesh_sensor_state_t sensor_states[4] = {
    /* Mesh Model Spec:
     * Multiple instances of the Sensor states may be present within the same model,
     * provided that each instance has a unique value of the Sensor Property ID to
     * allow the instances to be differentiated. Such sensors are known as multisensors.
     * In this example, two instances of the Sensor states within the same model are
     * provided.
     */
    [0] = 
    {
        .sensor_property_id             = BLEMESH_SENSOR_SERVER_VIBRATION_X_ID,
        .descriptor.positive_tolerance  = BLEMESH_SENSOR_SERVER_VIBRATION_POSITIVE_TOLERANCE,
        .descriptor.negative_tolerance  = BLEMESH_SENSOR_SERVER_VIBRATION_NEGATIVE_TOLERANCE,
        .descriptor.sampling_function   = BLEMESH_SENSOR_SERVER_VIBRATION_SAMPLE_FUNCTION,
        .descriptor.measure_period      = BLEMESH_SENSOR_SERVER_VIBRATION_MEASURE_PERIOD,
        .descriptor.update_interval     = BLEMESH_SENSOR_SERVER_VIBRATION_UPDATE_INTERVAL,
        .sensor_data.format             = ESP_BLE_MESH_SENSOR_DATA_FORMAT_B,
        .sensor_data.length             = BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH + BLEMESH_SENSOR_SERVER_SHIFT_DATA_LENGTH-1,        // 0 represents the length is 1
        .sensor_data.raw_value          = &nbVibration_X,   
    },
    [1] = 
    {
        .sensor_property_id             = BLEMESH_SENSOR_SERVER_VIBRATION_Y_ID,
        .descriptor.positive_tolerance  = BLEMESH_SENSOR_SERVER_VIBRATION_POSITIVE_TOLERANCE,
        .descriptor.negative_tolerance  = BLEMESH_SENSOR_SERVER_VIBRATION_NEGATIVE_TOLERANCE,
        .descriptor.sampling_function   = BLEMESH_SENSOR_SERVER_VIBRATION_SAMPLE_FUNCTION,
        .descriptor.measure_period      = BLEMESH_SENSOR_SERVER_VIBRATION_MEASURE_PERIOD,
        .descriptor.update_interval     = BLEMESH_SENSOR_SERVER_VIBRATION_UPDATE_INTERVAL,
        .sensor_data.format             = ESP_BLE_MESH_SENSOR_DATA_FORMAT_B,
        .sensor_data.length             = BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH + BLEMESH_SENSOR_SERVER_SHIFT_DATA_LENGTH-1,        // 0 represents the length is 1
        .sensor_data.raw_value          = &nbVibration_Y,   
    },
    [2] = 
    {
        .sensor_property_id             = BLEMESH_SENSOR_SERVER_VIBRATION_Z_ID,
        .descriptor.positive_tolerance  = BLEMESH_SENSOR_SERVER_VIBRATION_POSITIVE_TOLERANCE,
        .descriptor.negative_tolerance  = BLEMESH_SENSOR_SERVER_VIBRATION_NEGATIVE_TOLERANCE,
        .descriptor.sampling_function   = BLEMESH_SENSOR_SERVER_VIBRATION_SAMPLE_FUNCTION,
        .descriptor.measure_period      = BLEMESH_SENSOR_SERVER_VIBRATION_MEASURE_PERIOD,
        .descriptor.update_interval     = BLEMESH_SENSOR_SERVER_VIBRATION_UPDATE_INTERVAL,
        .sensor_data.format             = ESP_BLE_MESH_SENSOR_DATA_FORMAT_B,
        .sensor_data.length             = BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH + BLEMESH_SENSOR_SERVER_SHIFT_DATA_LENGTH-1,        // 0 represents the length is 1
        .sensor_data.raw_value          = &nbVibration_Z,   
    },
    [3] = 
    {
        .sensor_property_id             = BLEMESH_SENSOR_SERVER_STATUS_ID,
        .descriptor.positive_tolerance  = BLEMESH_SENSOR_SERVER_STATUS_POSITIVE_TOLERANCE,
        .descriptor.negative_tolerance  = BLEMESH_SENSOR_SERVER_STATUS_NEGATIVE_TOLERANCE,
        .descriptor.sampling_function   = BLEMESH_SENSOR_SERVER_STATUS_SAMPLE_FUNCTION,
        .descriptor.measure_period      = BLEMESH_SENSOR_SERVER_STATUS_MEASURE_PERIOD,
        .descriptor.update_interval     = BLEMESH_SENSOR_SERVER_STATUS_UPDATE_INTERVAL,
        .sensor_data.format             = ESP_BLE_MESH_SENSOR_DATA_FORMAT_B,
        .sensor_data.length             = BLEMESH_SENSOR_SERVER_STATUS_DATA_LENGTH-1,        // 0 represents the length is 1
        .sensor_data.raw_value          = &nbStatus,   
    },
};

    // Setup BLE Mesh Node as Sensor server
ESP_BLE_MESH_MODEL_PUB_DEFINE(sensor_pub, BLEMESH_SENSOR_SERVER_OCTETS, ROLE_NODE);
static esp_ble_mesh_sensor_srv_t sensor_server = 
{
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .state_count = ARRAY_SIZE(sensor_states),
    .states = sensor_states,
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(sensor_setup_pub, BLEMESH_SENSOR_SERVER_OCTETS, ROLE_NODE);
static esp_ble_mesh_sensor_setup_srv_t sensor_setup_server = 
{
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .state_count = ARRAY_SIZE(sensor_states),
    .states = sensor_states,
};

static esp_ble_mesh_model_t root_models[] = 
{
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_SENSOR_SRV(&sensor_pub, &sensor_server),
    ESP_BLE_MESH_MODEL_SENSOR_SETUP_SRV(&sensor_setup_pub, &sensor_setup_server),
};

static esp_ble_mesh_elem_t elements[] = 
{
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
};

static esp_ble_mesh_comp_t composition = 
{
    .cid = BLEMESH_SENSOR_SERVER_CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

static esp_ble_mesh_prov_t provision = 
{
    .uuid = dev_uuid,
};

// === Definition of global variables =============================================================


// === Definition of local variables ==============================================================
tBMX160_AccelType tBMX160_Accel_BLEmesh[VIBRATION_DATA_LENGTH];

uint8_t u8DataShift = 0;
uint8_t u8VibrationSemaphoreFlag = BLEMESH_SENSOR_SERVER_SEMAPHORE_NOT_OWNED;

// === Class/function implementation ==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Update the BLE Mesh NETBUFF
///
/// 
///
/// @param[in]
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
void BLEmesh_SensorServer_Update_NETBUFF(void)
{
    // Calculate data shift amount
    uint16_t u16ShiftArray = u8DataShift*(BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH / 2);
    
    // Reset NETBUFF for X-axis and add shift amount
    net_buf_simple_reset(&nbVibration_X);
    net_buf_simple_add_u8(&nbVibration_X, u8DataShift);

    // Add vibrationsdata to NETBUFF for X-axis 
    for(uint16_t u16X = 0; u16X < BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH / 2; u16X++)
    {
        net_buf_simple_add_le16(&nbVibration_X, tBMX160_Accel_BLEmesh[u16X+u16ShiftArray].s16X_Axis);
    }

    // Reset NETBUFF for Y-axis  and add shift amount
    net_buf_simple_reset(&nbVibration_Y);
    net_buf_simple_add_u8(&nbVibration_Y, u8DataShift);

    // Add vibrationsdata to NETBUFF for Y-axis 
    for(uint16_t u16X = 0; u16X < BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH / 2; u16X++)
    {
        net_buf_simple_add_le16(&nbVibration_Y, tBMX160_Accel_BLEmesh[u16X+u16ShiftArray].s16Y_Axis);
    }

    // Reset NETBUFF for Z-axis and add shift amount
    net_buf_simple_reset(&nbVibration_Z);
    net_buf_simple_add_u8(&nbVibration_Z, u8DataShift);

    // Add vibrationsdata to NETBUFF for Z-axis
    for(uint16_t u16X = 0; u16X < BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH / 2; u16X++)
    {
        net_buf_simple_add_le16(&nbVibration_Z, tBMX160_Accel_BLEmesh[u16X+u16ShiftArray].s16Z_Axis);
    }

    // Reset NETBUFF for status and add status
    ESP_LOGW(BLEMESH_SENSOR_SERVER_TAG, "SensorStatus: %d", u8SensorStatus);
    net_buf_simple_reset(&nbStatus);
    net_buf_simple_add_u8(&nbStatus, u8SensorStatus);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Called when provision is complete
///
/// 
///
/// @param[in] net_idx:     NET ID
/// @param[in] addr:        Adress
/// @param[in] flags:       Flags
/// @param[in] iv_index:    IV Index
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
static void BLEmesh_SensorServer_prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    // Log provision data
    ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "net_idx 0x%03x, addr 0x%04x", net_idx, addr);
    ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "flags 0x%02x, iv_index 0x%08x", flags, iv_index);

    // Turn green LED OFF and blue LED ON
    board_led_operation(LED_G, LED_OFF);
    board_led_operation(LED_B, LED_ON);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Provisioning callback
///
/// 
///
/// @param[in] event:   Event type
/// @param[in] *param:  Pointer to provisioning parameters
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
static void BLEmesh_SensorServer_provisioning_cb(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event) 
    {
        case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
            ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
            break;
        case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
            ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
            break;
        case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
            ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s", param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
            break;
        case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
            ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s", param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
            break;
        case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
            ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT");
            BLEmesh_SensorServer_prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr, param->node_prov_complete.flags, param->node_prov_complete.iv_index);
            break;
        case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
            ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_NODE_PROV_RESET_EVT");
            break;
        case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
            ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d", param->node_set_unprov_dev_name_comp.err_code);
            break;
        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Configure server callback
///
/// 
///
/// @param[in] event:   Event type
/// @param[in] *param:  Pointer to server parameters
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
static void BLEmesh_SensorServer_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event, esp_ble_mesh_cfg_server_cb_param_t *param)
{
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT)
    {
        switch (param->ctx.recv_op) 
        {
            case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
                ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
                ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "net_idx 0x%04x, app_idx 0x%04x",
                    param->value.state_change.appkey_add.net_idx,
                    param->value.state_change.appkey_add.app_idx);
                ESP_LOG_BUFFER_HEX("AppKey", param->value.state_change.appkey_add.app_key, 16);
                break;
            case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
                ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
                ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
                    param->value.state_change.mod_app_bind.element_addr,
                    param->value.state_change.mod_app_bind.app_idx,
                    param->value.state_change.mod_app_bind.company_id,
                    param->value.state_change.mod_app_bind.model_id);
                break;
            case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
                ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD");
                ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "elem_addr 0x%04x, sub_addr 0x%04x, cid 0x%04x, mod_id 0x%04x",
                    param->value.state_change.mod_sub_add.element_addr,
                    param->value.state_change.mod_sub_add.sub_addr,
                    param->value.state_change.mod_sub_add.company_id,
                    param->value.state_change.mod_sub_add.model_id);
                break;
            default:
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Get sensor data
///
/// 
///
/// @param[in] *state:  Pointer to sensor state
/// @param[in] *data:   Pointer to  where to store data
/// @param[out]
/// @return (mpid_len + data_len): Length of property ID and sensor data
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
static uint16_t BLEmesh_SensorServer_get_sensor_data(esp_ble_mesh_sensor_state_t *state, uint8_t *data)
{
    uint8_t     mpid_len    = 0;
    uint8_t     data_len    = 0;
    uint32_t    mpid        = 0;

    // Errorhandling
    if (state == NULL || data == NULL) 
    {
        ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "%s, Invalid parameter", __func__);
        return 0;
    }

    // Add sensor property ID and dataformat length
    if (state->sensor_data.format == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A) 
    {
        mpid = ESP_BLE_MESH_SENSOR_DATA_FORMAT_A_MPID(state->sensor_data.length, state->sensor_property_id);
        mpid_len = ESP_BLE_MESH_SENSOR_DATA_FORMAT_A_MPID_LEN;
    } 
    else 
    {
        mpid = ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID(state->sensor_data.length, state->sensor_property_id);
        mpid_len = ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID_LEN;
    }

    // Save length of sensor data, which is zero-based
    data_len = state->sensor_data.length + 1;

    // Memory copy property ID and dataformat length to data
    memcpy(data, &mpid, mpid_len);
    // Memory copy sensor data from NETBUFF to data
    memcpy(data + mpid_len, state->sensor_data.raw_value->data, data_len);

    // Return length of property ID and sensor data
    return (mpid_len + data_len);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Send sensor status
///
/// 
///
/// @param[in] *param:  Pointer to server parameters
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
static void BLEmesh_SensorServer_send_sensor_status(esp_ble_mesh_sensor_server_cb_param_t *param)
{
    uint8_t     *status     = NULL;
    uint16_t    buf_size    = 0;
    uint16_t    length      = 0;
    uint32_t    mpid        = 0;
    esp_err_t   err;
    int         i;

    /**
     * Sensor Data state from Mesh Model Spec
     * |--------Field--------|-Size (octets)-|------------------------Notes-------------------------|
     * |----Property ID 1----|-------2-------|--ID of the 1st device property of the sensor---------|
     * |-----Raw Value 1-----|----variable---|--Raw Value field defined by the 1st device property--|
     * |----Property ID 2----|-------2-------|--ID of the 2nd device property of the sensor---------|
     * |-----Raw Value 2-----|----variable---|--Raw Value field defined by the 2nd device property--|
     * | ...... |
     * |----Property ID n----|-------2-------|--ID of the nth device property of the sensor---------|
     * |-----Raw Value n-----|----variable---|--Raw Value field defined by the nth device property--|
     */
    for (i = 0; i < ARRAY_SIZE(sensor_states); i++) 
    {
        esp_ble_mesh_sensor_state_t *state = &sensor_states[i];
        if (state->sensor_data.length == ESP_BLE_MESH_SENSOR_DATA_ZERO_LEN) 
        {
            buf_size += ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID_LEN;
        } 
        else 
        {
            /* Use "state->sensor_data.length + 1" because the length of sensor data is zero-based. */
            if (state->sensor_data.format == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A) 
            {
                buf_size += ESP_BLE_MESH_SENSOR_DATA_FORMAT_A_MPID_LEN + state->sensor_data.length + 1;
            } 
            else 
            {
                buf_size += ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID_LEN + state->sensor_data.length + 1;
            }
        }
    }

    status = calloc(1, buf_size);
    if (!status) 
    {
        ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "No memory for sensor status!");
        return;
    }

    if (param->value.get.sensor_data.op_en == false) 
    {
        /* Mesh Model Spec:
         * If the message is sent as a response to the Sensor Get message, and if the
         * Property ID field of the incoming message is omitted, the Marshalled Sensor
         * Data field shall contain data for all device properties within a sensor.
         */
        for (i = 0; i < ARRAY_SIZE(sensor_states); i++) 
        {
            length += BLEmesh_SensorServer_get_sensor_data(&sensor_states[i], status + length);
        }
        goto send;
    }

    /* Mesh Model Spec:
     * Otherwise, the Marshalled Sensor Data field shall contain data for the requested
     * device property only.
     */
    for (i = 0; i < ARRAY_SIZE(sensor_states); i++) 
    {
        if (param->value.get.sensor_data.property_id == sensor_states[i].sensor_property_id) 
        {
            length = BLEmesh_SensorServer_get_sensor_data(&sensor_states[i], status);
            goto send;
        }
    }

    /* Mesh Model Spec:
     * Or the Length shall represent the value of zero and the Raw Value field shall
     * contain only the Property ID if the requested device property is not recognized
     * by the Sensor Server.
     */
    mpid = ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID(ESP_BLE_MESH_SENSOR_DATA_ZERO_LEN, param->value.get.sensor_data.property_id);
    memcpy(status, &mpid, ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID_LEN);
    length = ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID_LEN;

send:
    //ESP_LOG_BUFFER_HEX("Sensor Data", status, length);
    ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "Send Sensor Data");

    err = esp_ble_mesh_server_model_send_msg(param->model, &param->ctx, ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS, length, status);
    if (err != ESP_OK) 
    {
        ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "Failed to send Sensor Status");
    }
    free(status);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Sensor server callback
///
/// 
///
/// @param[in] event:   Event type
/// @param[in] *param:  Pointer to server parameters
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
static void BLEmesh_SensorServer_sensor_server_cb(esp_ble_mesh_sensor_server_cb_event_t event, esp_ble_mesh_sensor_server_cb_param_t *param)
{
    ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "Sensor server, event %d, src 0x%04x, dst 0x%04x, model_id 0x%04x", event, param->ctx.addr, param->ctx.recv_dst, param->model->model_id);

    switch (event) 
    {
        case ESP_BLE_MESH_SENSOR_SERVER_RECV_GET_MSG_EVT:
            switch (param->ctx.recv_op) 
            {
                case ESP_BLE_MESH_MODEL_OP_SENSOR_GET:
                    ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "ESP_BLE_MESH_MODEL_OP_SENSOR_GET");
                    
                    if((GetTime_ms() - u32BLEmeshTimeStart_ms) > BLE_REQ_SEG_WAIT_TIME && u8VibrationSemaphoreFlag == BLEMESH_SENSOR_SERVER_SEMAPHORE_OWNED)
                    {
                        ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "!ERROR! Segment request timeout ");

                        xSemaphoreGive(xVibrationSemaphore);
                        u8VibrationSemaphoreFlag = BLEMESH_SENSOR_SERVER_SEMAPHORE_NOT_OWNED;   
                        u8DataShift = 0;      
                    }

                    // Take semaphore if transmitting first section of sensor data
                    if(u8DataShift == 0)
                    {
                        // Take semaphore or wait for BLEMESH_SENSOR_SERVER_UPDATE_TAKE_WAIT_MS
                        if(xSemaphoreTake(xVibrationSemaphore, BLEMESH_SENSOR_SERVER_UPDATE_TAKE_WAIT_MS / portTICK_PERIOD_MS) == pdTRUE)
                        {
                            ESP_LOGW(BLEMESH_SENSOR_SERVER_TAG, "Vibration Semaphore Taken");
                            
                            // Set flag to indicate that semaphore is owned by BLE Mesh
                            u8VibrationSemaphoreFlag = BLEMESH_SENSOR_SERVER_SEMAPHORE_OWNED;
                        }
                        else
                        {
                            ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "Vibration Semaphore ERROR");
                        }
                    }

                    // If BLE Mesh owns semaphore
                    if(u8VibrationSemaphoreFlag == BLEMESH_SENSOR_SERVER_SEMAPHORE_OWNED)
                    {
                        // Save segment request start time
                        u32BLEmeshTimeStart_ms = GetTime_ms();

                        // Update in NETBUFF and send sensor data
                        BLEmesh_SensorServer_Update_NETBUFF();
                        BLEmesh_SensorServer_send_sensor_status(param);

                        // Count up u8DataShift if last section haven't been send
                        if(u8DataShift < ((VIBRATION_DATA_LENGTH * 2) / BLEMESH_SENSOR_SERVER_VIBRATION_DATA_LENGTH) - 1)
                        {
                            u8DataShift++;
                        }
                        // Give semaphore, reset flag and reset u8DataShift if last section has been send 
                        else
                        {
                            u8SensorStatus = 0xFF;

                            xSemaphoreGive(xVibrationSemaphore);
                            u8VibrationSemaphoreFlag = BLEMESH_SENSOR_SERVER_SEMAPHORE_NOT_OWNED;
                            ESP_LOGW(BLEMESH_SENSOR_SERVER_TAG, "Vibration Semaphore Given");
                            
                            u8DataShift = 0;
                        }
                    }

                    break;
                default:
                    ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "Unknown Sensor Get opcode 0x%04x", param->ctx.recv_op);
                    return;
            }
            break;
        default:
            ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "Unknown Sensor Server event %d", event);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Update vibration sensor data
///
/// 
///
/// @param[in] tBMX160_AccelData:   Struct with vibration sensor data
/// @param[in] u16idx:              Index in struct array to update
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
void BLEmesh_SensorServer_Update_VibrationData(tBMX160_AccelType tBMX160_AccelData, uint16_t u16idx)
{
    tBMX160_Accel_BLEmesh[u16idx] = tBMX160_AccelData; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Init ESP BLE mesh
///
/// 
///
/// @param[in]
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
static esp_err_t BLEmesh_SensorServer_esp_ble_mesh_init(void)
{
    esp_err_t err;

    // Register callbacks
    esp_ble_mesh_register_prov_callback(BLEmesh_SensorServer_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(BLEmesh_SensorServer_config_server_cb);
    esp_ble_mesh_register_sensor_server_callback(BLEmesh_SensorServer_sensor_server_cb);

    // Init ESP BLE Mesh
    err = esp_ble_mesh_init(&provision, &composition);
    if (err != ESP_OK) {
        ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "Failed to initialize mesh stack");
        return err;
    }

    // Enable node provisioning
    err = esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    if (err != ESP_OK) {
        ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "Failed to enable mesh node");
        return err;
    }

    // Turn on green LED
    board_led_operation(LED_G, LED_ON);

    ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "BLE Mesh sensor server initialized");

    return ESP_OK;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Initialize BLEmesh
///
/// 
///
/// @param[in]
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
void BLEmesh_SensorServer_Init(void)
{
    esp_err_t err;

    ESP_LOGI(BLEMESH_SENSOR_SERVER_TAG, "Initializing...");

    // Init NVS Flash
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Init board
    board_init();

    // Init Bluetooth
    err = bluetooth_init();
    if (err) 
    {
        ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }

    // Get BLE Mesh UID
    ble_mesh_get_dev_uuid(dev_uuid);

    // Initialize the Bluetooth Mesh Subsystem
    err = BLEmesh_SensorServer_esp_ble_mesh_init();
    if (err) 
    {
        ESP_LOGE(BLEMESH_SENSOR_SERVER_TAG, "Bluetooth mesh init failed (err %d)", err);
    }

}

// END OF FILE