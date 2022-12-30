///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file D_I2C.h
///
/// @note Project    : IIoT_SensorUnit
/// @note Subsystem  : I2C
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef D_I2C
#define D_I2C

/* Includes ------------------------------------------------------------------*/
#include "driver/i2c.h"

// === External references ========================================================================
extern esp_err_t D_I2C_Init(void);

// === Definition of macro / constants ============================================================
// Defines

#define D_I2C_MASTER_SCL_IO           19      /*!< GPIO number used for I2C master clock */
#define D_I2C_MASTER_SDA_IO           18      /*!< GPIO number used for I2C master data  */
#define D_I2C_MASTER_NUM              0       /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define D_I2C_MASTER_FREQ_HZ          400000  /*!< I2C master clock frequency */
#define D_I2C_MASTER_TX_BUF_DISABLE   0       /*!< I2C master doesn't need buffer */
#define D_I2C_MASTER_RX_BUF_DISABLE   0       /*!< I2C master doesn't need buffer */
#define D_I2C_MASTER_TIMEOUT_MS       1000


// === Definition of global variables =============================================================


// === Definition of classes/functions ============================================================



// External references


#endif // end D_I2C

// END OF FILE