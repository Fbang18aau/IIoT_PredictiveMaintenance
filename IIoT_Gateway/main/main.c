///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file main.c
///
/// @note Project    : IIoT_SensorUnit
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "main.h"

// === Definition of macro / constants ============================================================


// === Definition of global variables =============================================================
tBMX160_AccelType tBMX160_Accel[VIBRATION_DATA_LENGTH];

SemaphoreHandle_t xVibrationSemaphore = NULL;

uint8_t u8SensorStatusMain = 0xFF;

// === Definition of local variables ==============================================================
uint8_t u8BLEmeshDataSetRecFlag =   0;

uint8_t u8BLEmeshDataRecFail    =   0;

// === Class/function implementation ==============================================================


///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief 
///
/// 
///
/// @param[in] 
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t GetTime_ms(void)
{
    struct      timeval tv_time_now;
    int64_t     u32Time_us = 0;
    int64_t     u32Time_ms = 0;

    // Get time of day and calculate starttime in microseconds
    gettimeofday(&tv_time_now, NULL);
    u32Time_us = (int64_t)tv_time_now.tv_sec * 1000000L + (int64_t)tv_time_now.tv_usec;

    u32Time_ms = (uint32_t)(u32Time_us/1000);

    return (u32Time_ms);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Task Delay using vTaskDelay
///
/// 
///
/// @param[in] u32Delay_ms: Delay in milliseconds
/// @param[out]
/// @return
///
/// @warning Can't be shorter than 10 ms
///////////////////////////////////////////////////////////////////////////////////////////////////
void TaskDelay(uint32_t u32Delay_ms)
{
    vTaskDelay(u32Delay_ms / portTICK_PERIOD_MS);
}


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

static void setTimeZone(long offset, int daylight)
{
    char cst[17] = {0};
    char cdt[17] = "DST";
    char tz[33] = {0};

    if(offset % 3600){
        sprintf(cst, "UTC%ld:%02u:%02u", offset / 3600, abs((offset % 3600) / 60), abs(offset % 60));
    } else {
        sprintf(cst, "UTC%ld", offset / 3600);
    }
    if(daylight != 3600){
        long tz_dst = offset - daylight;
        if(tz_dst % 3600){
            sprintf(cdt, "DST%ld:%02u:%02u", tz_dst / 3600, abs((tz_dst % 3600) / 60), abs(tz_dst % 60));
        } else {
            sprintf(cdt, "DST%ld", tz_dst / 3600);
        }
    }
    sprintf(tz, "%s%s", cst, cdt);
    setenv("TZ", tz, 1);
    tzset();
}

uint8_t configTime(long gmtOffset_sec, int daylightOffset_sec, const char* server)
{
    time_t      timeNow;
    struct tm   ts;
    char        buf[40];

    esp_netif_init();

    for(uint8_t u8NumOfTried = 0; u8NumOfTried < CONFIG_TIME_TRIES; u8NumOfTried++)
    {
        if(sntp_enabled())
        {
            sntp_stop();
        }

        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, (char*)server);
        sntp_init();
        setTimeZone(-gmtOffset_sec, daylightOffset_sec);

        TaskDelay(CONFIG_TIME_DELAY);

        // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
        time(&timeNow);

        ts = *localtime(&timeNow);
        strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);

        ESP_LOGI(MAIN_TAG, "Time: %s", buf);

        if(timeNow > CONFIG_TIME_UNIX_OFFSET)
        {
            return (true);
        }                    
    }

    return (false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Task to request data from Sensor Server
///
/// 
///
/// @param[in] 
/// @param[out]
/// @return
///
/// @warning 
///////////////////////////////////////////////////////////////////////////////////////////////////
void RequestSensorServerData_Task(void * pvParameters )
{
    while(1)
    {
        // When unit is provisioned
        if(u8ProvFlag == 1)
        {
            uint32_t u32TimeStart_ms = 0;

            // Request data segnemt 1 to n from Sensor Server
            for(uint8_t u8DP = 0; u8DP < VIBRATION_DATA_SEGMENTS; u8DP++)
            {
                example_ble_mesh_send_sensor_message(ESP_BLE_MESH_MODEL_OP_SENSOR_GET);
                ESP_LOGW(MAIN_TAG, "Request: %d", u8DP);

                u32TimeStart_ms = GetTime_ms();

                while(u8BLEmeshSegRecFlag == 0)
                {
                    if((GetTime_ms() - u32TimeStart_ms) > BLE_REC_SEG_WAIT_TIME)
                    {
                        // If data is not received
                        u8BLEmeshDataRecFail++;
                        
                        ESP_LOGE(MAIN_TAG, "!ERROR! Request: %d, Fail: %d", u8DP, u8BLEmeshDataRecFail);

                        u8BLEmeshSegRecFlag = 0;
                        u8SensorStatusMain  = 0xFF;

                        if(u8BLEmeshDataRecFail > BLE_REC_FAIL_AMOUNT)
                        {
                            u8ProvFlag = 0;
                            
                            D_GPIO_SetPinLevel(LED_R, LED_ON);
                            D_GPIO_SetPinLevel(LED_G, LED_ON);
                            D_GPIO_SetPinLevel(LED_B, LED_OFF);
                        }

                        goto ReqEnd;
                    }

                    TaskDelay(100);
                }

                u8BLEmeshSegRecFlag = 0;
            }

            u8BLEmeshDataRecFail = 0;

            if(xSemaphoreTake(xVibrationSemaphore, BLE_SEM_WAIT_TIME / portTICK_PERIOD_MS) == pdTRUE)
            {
                // Mirror data from tBMX160_Accel to BLEmesh_SensorServer struct 
                for(uint16_t u16X = 0; u16X < VIBRATION_DATA_LENGTH; u16X++)
                {
                    tBMX160_Accel[u16X] = BLEmesh_SensorClient_Update_VibrationData(u16X);
                }
                BLEmesh_SensorClient_Reset_VibrationData();
                ESP_LOGW(MAIN_TAG, "Data updated");
                
                // Give semaphore
                xSemaphoreGive(xVibrationSemaphore);
                
                u8BLEmeshDataSetRecFlag = 1;
                u8SensorStatusMain      = u8SensorStatusBLEmesh;
            }
            else
            {
                ESP_LOGE(MAIN_TAG, "Data update FAILED");
            }
        }

        ReqEnd:

        TaskDelay(REQUEST_SENSOR_SERVER_DATA_TASK_DELAY_MS);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Task to send MQTT task
///
/// 
///
/// @param[in] 
/// @param[out]
/// @return
///
/// @warning 
///////////////////////////////////////////////////////////////////////////////////////////////////
void SendMQTT_Task(void * pvParameters )
{
    while(1)
    {
        ESP_LOGW(MAIN_TAG, "Sensor Status: %d", u8SensorStatusMain);

        if(u8BLEmeshDataSetRecFlag == 1 && u8SensorStatusMain == 0)
        {
            int returnStatus = EXIT_SUCCESS;
            bool clientSessionPresent = false;
            /* Attempt to connect to the MQTT broker. If connection fails, retry after
            * a timeout. Timeout value will be exponentially increased till the maximum
            * attempts are reached or maximum timeout value is reached. The function
            * returns EXIT_FAILURE if the TCP connection cannot be established to
            * broker after configured number of attempts. */
            returnStatus = connectToServerWithBackoffRetries( &xNetworkContext );
            if( returnStatus == EXIT_FAILURE )
            {
                /* Log error to indicate connection failure after all
                * reconnect attempts are over. */
                LogError( ( "Failed to connect to MQTT broker %.*s.",
                            AWS_IOT_ENDPOINT_LENGTH,
                            AWS_IOT_ENDPOINT ) );
            }
            else
            {
                /* If TLS session is established, execute Subscribe/Publish loop. */
                //returnStatus = subscribePublishLoop( &mqttContext, &clientSessionPresent );
                returnStatus = EstablishAndSubscribe( &mqttContext, &clientSessionPresent );
                returnStatus = Publish( &mqttContext, &clientSessionPresent );
                returnStatus = DisconnectMQTT( &mqttContext, &clientSessionPresent );
            }

            if( returnStatus == EXIT_SUCCESS )
            {
                /* Log message indicating an iteration completed successfully. */
                LogInfo( ( "Completed successfully." ) );

                u8SensorStatusMain          = 0xFF;
                u8BLEmeshDataSetRecFlag = 0;
            }

            /* End TLS session, then close TCP connection. */
            ( void ) xTlsDisconnect( &xNetworkContext );

            LogInfo( ( "Short delay before starting the next iteration....\n" ) );
        }

        TaskDelay(SEND_MQTT_TASK_DELAY_MS);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Task to update time
///
/// 
///
/// @param[in] 
/// @param[out]
/// @return
///
/// @warning 
///////////////////////////////////////////////////////////////////////////////////////////////////
void UpdateTime_Task(void * pvParameters )
{
    while(1)
    {
        if(configTime(gmtOffset_sec, daylightOffset_sec, ntpServer) == true)
        {
            ESP_LOGI(MAIN_TAG, "UNIX Time OK");
        }
        else
        {
            ESP_LOGE(MAIN_TAG, "UNIX Time ERROR");
        }

        TaskDelay(UPDATE_TIME_TASK_DELAY_MS);
    }
}

// !!!!!!!!!!!!!!!!!!!Only while debugging!!!!!!!!!!!!!!!!!!!
// uint8_t u8Start = 0;
// !!!!!!!!!!!!!!!!!!!Only while debugging!!!!!!!!!!!!!!!!!!!

void app_main(void)
{
    // !!!!!!!!!!!!!!!!!!!Only while debugging!!!!!!!!!!!!!!!!!!!

    // while(u8Start == 0)
    // {
    //     TaskDelay(100);
    // }

    // !!!!!!!!!!!!!!!!!!!Only while debugging!!!!!!!!!!!!!!!!!!!


    // Init
    D_GPIO_Init();
    BLEmesh_SensorClient_Init();
    D_AWS_MQTT_Init();

    TaskDelay(INIT_SYS_DELAY_MS);
    
    
    // Create xVibrationSemaphore
    xVibrationSemaphore = xSemaphoreCreateMutex();

    if(xVibrationSemaphore != NULL)
    {
        ESP_LOGI(MAIN_TAG, "Vibration Semaphore OK");
    }
    else
    {
        ESP_LOGE(MAIN_TAG, "Vibration Semaphore ERROR");

    }

    if(u8ProvFlag == 0)
    {
        D_GPIO_SetPinLevel(LED_G, LED_ON);
    }

    // Create Tasks

    static uint8_t  ucParameterToPass1;
    static uint8_t  ucParameterToPass2;
    static uint8_t  ucParameterToPass3;

    TaskHandle_t    xHandle1 = NULL;
    TaskHandle_t    xHandle2 = NULL;
    TaskHandle_t    xHandle3 = NULL;

    xTaskCreate(RequestSensorServerData_Task, "RequestSensorServerData_Task", REQUEST_SENSOR_SERVER_DATA_TASK_STACK_SIZE, ucParameterToPass1, tskIDLE_PRIORITY,  &xHandle1);

    xTaskCreate(SendMQTT_Task, "SendMQTT_Task", SEND_MQTT_TASK_STACK_SIZE, ucParameterToPass2, tskIDLE_PRIORITY+1,  &xHandle2);

    xTaskCreate(UpdateTime_Task, "UpdateTime_Task", UPDATE_TIME_TASK_STACK_SIZE, ucParameterToPass3, tskIDLE_PRIORITY+2,  &xHandle3);

}
