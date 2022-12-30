///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file MyTypes.h
///
/// @note Project    : IIoT_Client/Gateway
/// @note Subsystem  : MyTypes
///
/// @author Frederik Bang
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MyTypes
#define MyTypes

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

// Struct typedef
typedef struct
{
  int16_t s16X_Axis;
  int16_t s16Y_Axis;
  int16_t s16Z_Axis;
}tBMX160_AccelType;

#endif // end MyTypes

// END OF FILE