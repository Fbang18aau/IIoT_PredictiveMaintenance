///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file D_GPIO.h
///
/// @note Project    : IIoT_Client/Gateway
/// @note Subsystem  : GPIO
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef D_GPIO
#define D_GPIO

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "driver/gpio.h"

// === External references ========================================================================
extern  void D_GPIO_Init(void);
extern  void D_GPIO_SetPinLevel(uint8_t u8GPIO, uint8_t u8Level);

// === Definition of macro / constants ============================================================
// Defines
#define LED_R GPIO_NUM_0
#define LED_G GPIO_NUM_2
#define LED_B GPIO_NUM_4

#define LED_ON  1
#define LED_OFF 0

// === Definition of global variables =============================================================


// === Definition of classes/functions ============================================================



// External references


#endif // end D_GPIO

// END OF FILE