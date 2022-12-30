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

uint8_t u8SensorStatus = 0xFF;

uint32_t u32BLEmeshTimeStart_ms = 0;

// === Definition of local variables ==============================================================
uint32_t u32SensorTimeStart_ms = 0;

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
/// @brief Real Delay using internal timer
///
/// 
///
/// @param[in] u32Delay_us: Delay in microseconds
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
void RealDelay_us(uint32_t u32Delay_us)
{
    struct      timeval tv_time_now;
    int64_t     s64TimeStart_us = 0;
    int64_t     s64TimeNow_us   = 0;
    uint32_t    u32TimeDiff_us  = 0;

    // Get time of day and calculate starttime in microseconds
    gettimeofday(&tv_time_now, NULL);
    s64TimeStart_us = (int64_t)tv_time_now.tv_sec * 1000000L + (int64_t)tv_time_now.tv_usec;

    // Wait untill the specified time u32Delay_us has passed
    while(u32TimeDiff_us < u32Delay_us)
    {
        gettimeofday(&tv_time_now, NULL);
        s64TimeNow_us   = (int64_t)tv_time_now.tv_sec * 1000000L + (int64_t)tv_time_now.tv_usec;
        u32TimeDiff_us  = (uint32_t)(s64TimeNow_us - s64TimeStart_us);
    }

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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Task to get sensordata from BMX160 and store
///
/// 
///
/// @param[in] 
/// @param[out]
/// @return
///
/// @warning 
///////////////////////////////////////////////////////////////////////////////////////////////////
void UpdateSensorData_Task(void * pvParameters )
{
    while(1)
    {
        if(gpio_get_level(MOTOR_ON) == true && (GetTime_ms() - u32SensorTimeStart_ms) > MOTOR_WAIT_MS)
        {
            u32SensorTimeStart_ms = GetTime_ms();

            // Get sensor data from BMX160 and store in tBMX160_Accel
            for(uint16_t u16X = 0; u16X < VIBRATION_DATA_LENGTH; u16X++)
            {
                tBMX160_Accel[u16X] = D_BMX160_Poll();

                // ESP_LOGW(MAIN_TAG, "Data %d: %d", u16X, tBMX160_Accel[u16X].s16X_Axis);
                RealDelay_us(SAMPLE_DELAY_US);
            }
            ESP_LOGW(MAIN_TAG, "Data sampled");
            
            // Take semaphore or wait for SAMPLE_TAKE_WAIT_MS
            if(xSemaphoreTake(xVibrationSemaphore, SAMPLE_TAKE_WAIT_MS / portTICK_PERIOD_MS) == pdTRUE)
            {
                // Mirror data from tBMX160_Accel to BLEmesh_SensorServer struct 
                for(uint16_t u16X = 0; u16X < VIBRATION_DATA_LENGTH; u16X++)
                {
                    BLEmesh_SensorServer_Update_VibrationData(tBMX160_Accel[u16X], u16X);
                }

                u8SensorStatus = 0;
                
                ESP_LOGW(MAIN_TAG, "Data updated");
                
                // Give semaphore
                xSemaphoreGive(xVibrationSemaphore);
            }
            else
            {
                ESP_LOGE(MAIN_TAG, "Data update FAILED");
            }
        }
        else
        {
            ESP_LOGW(MAIN_TAG, "Motor not running");
        }

        // if((GetTime_ms() - u32BLEmeshTimeStart_ms) > BLE_REQ_SEG_WAIT_TIME && u8VibrationSemaphoreFlag == BLEMESH_SENSOR_SERVER_SEMAPHORE_OWNED)
        // {
        //     ESP_LOGE(MAIN_TAG, "!ERROR! Segment request timeout ");

        //     xSemaphoreGive(xVibrationSemaphore);
        //     u8VibrationSemaphoreFlag = BLEMESH_SENSOR_SERVER_SEMAPHORE_NOT_OWNED;   
        //     u8DataShift = 0;      
        // }
        
        // Wait
        TaskDelay(UPDATE_SENSOR_DATA_TASK_DELAY_MS);
    }
}


void app_main(void)
{
    // !!!!!!!!!!!!!!!!!!!Only while debugging!!!!!!!!!!!!!!!!!!!

    // uint8_t u8Start = 0;
    // while(u8Start == 0)
    // {
    //     TaskDelay(100);
    // }

    // !!!!!!!!!!!!!!!!!!!Only while debugging!!!!!!!!!!!!!!!!!!!

    // Init
    D_I2C_Init();
    D_BMX160_Init();
    BLEmesh_SensorServer_Init();

    // Create UpdateSensorData_Task
    static uint8_t  ucParameterToPass;
    TaskHandle_t    xHandle = NULL;

    xTaskCreate(UpdateSensorData_Task, "UpdateSensorData_Task", UPDATE_SENSOR_DATA_TASK_STACK_SIZE, ucParameterToPass, configMAX_PRIORITIES-1,  &xHandle);

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

}