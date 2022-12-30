///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file D_BMX160.h
///
/// @note Project    : IIoT_SensorUnit
/// @note Subsystem  : BMX160
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef D_BMX160
#define D_BMX160

/* Includes ------------------------------------------------------------------*/
#include "D_I2C.h"

// Include FreeRTOS for delay
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// === External references ========================================================================
extern esp_err_t          D_BMX160_Init(void);


// === Definition of macro / constants ============================================================
// Defines
	// Delays
#define D_BMX160_INIT_DELAY1	50
#define D_BMX160_INIT_DELAY2	100
#define D_BMX160_INIT_DELAY3	10

	// I2C Adress
#define D_BMX160_SENSOR_ADDR          0x68        /*!< Slave address of the BMX160 sensor */

	// I2C Register Adress
#define D_BMX160_CMD_REG_ADDR        	0x7E        /*!< Register addresses of the "Command" register */
#define D_BMX160_ACCEL_DATA_REG_ADDR	0x12        /*!< Register addresses of the "ACCEL_DATA" register */
#define D_BMX160_ACC_CONF_REG_ADDR	  0x40        /*!< Register addresses of the "ACC_CONF" register */

	// I2C BMX160 Init
#define D_BMX160_INIT_DATA1	0x11
#define D_BMX160_INIT_DATA2	0x15
#define D_BMX160_INIT_DATA3	0x19

  // I2C BMX160 Accel setup
#define D_BMX160_ACC_CONF   0x2C

  // Accel calc
#define D_BMX160_ACCEL_RANGE_MULTIPLIER   100000
#define D_BMX160_ACCEL_RANGE              0.000061035F*D_BMX160_ACCEL_RANGE_MULTIPLIER

// === Definition of global variables =============================================================


// === Definition of classes/functions ============================================================

// Struct typedef
typedef struct
{
  int16_t s16X_Axis;
  int16_t s16Y_Axis;
  int16_t s16Z_Axis;
}tBMX160_AccelType;



// External references
extern tBMX160_AccelType  D_BMX160_Poll(void);

#endif // end D_BMX160

// END OF FILE