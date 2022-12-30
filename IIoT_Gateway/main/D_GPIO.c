///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file D_GPIO.c
///
/// @note Project    : IIoT_Client/Gateway
/// @note Subsystem  : GPIO
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "D_GPIO.h"


// === Definition of macro / constants ============================================================


// === Definition of global variables =============================================================


// === Definition of local variables ==============================================================


// === Class/function implementation ==============================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Set GPIO pin
///
/// 
///
/// @param[in]
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
void D_GPIO_SetPinLevel(uint8_t u8GPIO, uint8_t u8Level)
{
  
  gpio_set_level(u8GPIO, u8Level);

}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Initialize D_GPIO
///
/// 
///
/// @param[in]
/// @param[out]
/// @return
///
/// @warning
///////////////////////////////////////////////////////////////////////////////////////////////////
void D_GPIO_Init(void)
{
  
  gpio_reset_pin(LED_R);
  gpio_reset_pin(LED_G);
  gpio_reset_pin(LED_B);

  gpio_set_direction(LED_R, GPIO_MODE_OUTPUT);
  gpio_set_direction(LED_G, GPIO_MODE_OUTPUT);
  gpio_set_direction(LED_B, GPIO_MODE_OUTPUT);

  gpio_set_level(LED_R, LED_OFF);
  gpio_set_level(LED_G, LED_OFF);
  gpio_set_level(LED_B, LED_OFF);

}

// END OF FILE