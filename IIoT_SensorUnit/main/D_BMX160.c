///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file D_BMX160.c
///
/// @note Project    : IIoT_SensorUnit
/// @note Subsystem  : BMX160
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "D_BMX160.h"


// === Definition of macro / constants ============================================================


// === Definition of global variables =============================================================


// === Definition of local variables ==============================================================


// === Class/function implementation ==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Write BMX160 register byte
///
/// 
///
/// @param[in] u8reg_addr:  Register adress
/// @param[in] u8data:		Data byte
/// @param[out]	
/// @return Error code
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
esp_err_t D_BMX160_register_write_byte(uint8_t u8reg_addr, uint8_t u8data)
{
    uint8_t u8write_buf[] = {u8reg_addr, u8data};
	
    return i2c_master_write_to_device(D_I2C_MASTER_NUM, D_BMX160_SENSOR_ADDR, u8write_buf, sizeof(u8write_buf), D_I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);

}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Read BMX160 register
///
/// 
///
/// @param[in] u8reg_addr:  Register adress
/// @param[in] *u8data:	    Pointer to data array
/// @param[in] len:		    Length of data to read
/// @param[out]	
/// @return Error code
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
esp_err_t D_BMX160_register_read(uint8_t u8reg_addr, uint8_t *u8data, size_t len)
{
    return i2c_master_write_read_device(D_I2C_MASTER_NUM, D_BMX160_SENSOR_ADDR, &u8reg_addr, 1, u8data, len, D_I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Poll BMX160 Accel data
///
/// 
///
/// @param[in] u8reg_addr:  Register adress
/// @param[in] *u8data:	    Pointer to data array
/// @param[in] len:		    Length of data to read
/// @param[out]	
/// @return Error code
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
tBMX160_AccelType D_BMX160_Poll(void)
{
    tBMX160_AccelType tBMX160_Accel;

    uint8_t u8Data[7];

    D_BMX160_register_read(D_BMX160_ACCEL_DATA_REG_ADDR, u8Data, 6);

    tBMX160_Accel.s16X_Axis = ((int16_t) (((uint16_t)u8Data[1]<<8) | u8Data[0]) * D_BMX160_ACCEL_RANGE);
    tBMX160_Accel.s16Y_Axis = ((int16_t) (((uint16_t)u8Data[3]<<8) | u8Data[2]) * D_BMX160_ACCEL_RANGE);
    tBMX160_Accel.s16Z_Axis = ((int16_t) (((uint16_t)u8Data[5]<<8) | u8Data[4]) * D_BMX160_ACCEL_RANGE);

    return tBMX160_Accel;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Initialize BMX160
///
/// 
///
/// @param[in]
/// @param[out]
/// @return Error code
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
esp_err_t D_BMX160_Init(void)
{
    esp_err_t   err;

    err = D_BMX160_register_write_byte(D_BMX160_CMD_REG_ADDR, D_BMX160_INIT_DATA1);
    if(err != 0)
	{
		return err;
	}
    vTaskDelay(D_BMX160_INIT_DELAY1 / portTICK_PERIOD_MS);
    
    err = D_BMX160_register_write_byte(D_BMX160_CMD_REG_ADDR, D_BMX160_INIT_DATA2);
	if(err != 0)
	{
		return err;
	}
    vTaskDelay(D_BMX160_INIT_DELAY2 / portTICK_PERIOD_MS);

    err = D_BMX160_register_write_byte(D_BMX160_CMD_REG_ADDR, D_BMX160_INIT_DATA3);
    if(err != 0)
	{
		return err;
	}
    vTaskDelay(D_BMX160_INIT_DELAY3 / portTICK_PERIOD_MS);

    err = D_BMX160_register_write_byte(D_BMX160_ACC_CONF_REG_ADDR, D_BMX160_ACC_CONF);
	
	return err;
}

// END OF FILE