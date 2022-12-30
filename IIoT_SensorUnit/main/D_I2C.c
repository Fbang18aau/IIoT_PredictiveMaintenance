///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file D_I2C.c
///
/// @note Project    : IIoT_SensorUnit
/// @note Subsystem  : I2C
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "D_I2C.h"


// === Definition of macro / constants ============================================================


// === Definition of global variables =============================================================


// === Definition of local variables ==============================================================


// === Class/function implementation ==============================================================



///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Initialize I2C
///
/// 
///
/// @param[in]
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
esp_err_t D_I2C_Init(void)
{
    int16_t s16i2c_master_port = D_I2C_MASTER_NUM;

    i2c_config_t conf = 
    {
        .mode               = I2C_MODE_MASTER,
        .sda_io_num         = D_I2C_MASTER_SDA_IO,
        .scl_io_num         = D_I2C_MASTER_SCL_IO,
        .sda_pullup_en      = GPIO_PULLUP_ENABLE,
        .scl_pullup_en      = GPIO_PULLUP_ENABLE,
        .master.clk_speed   = D_I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(s16i2c_master_port, &conf);

    return i2c_driver_install(s16i2c_master_port, conf.mode, D_I2C_MASTER_RX_BUF_DISABLE, D_I2C_MASTER_TX_BUF_DISABLE, 0);
}

// END OF FILE