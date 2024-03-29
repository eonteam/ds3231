/**
 ******************************************************************************
 * @file    ds3231.h
 * @author  EonTeam
 * @version V1.0.1
 * @date    2020
 ******************************************************************************
 */

#ifndef __DS3231_H_
#define __DS3231_H_

#include "eonOS.h"

// ===============================================================================
// Type
// ===============================================================================

typedef struct {
  // DS3231 I2Cx bus
  I2C_TypeDef *I2Cx;
  // interrupt pin
  pin_t int_sqw_pin;
  // private variable do not modify outside
  uint8_t _i2cbuf[10];
} ds3231_t;

// ===============================================================================
// Functions
// ===============================================================================

// Adjust the time through a xtime_t obj
bool ds3231_adjust(ds3231_t *ds, xtime_t *xt);
// Adjust the time through a unix time
bool ds3231_adjustUnix(ds3231_t *ds, uint32_t u);
// Returns the current time in a xtime_t object
bool ds3231_now(ds3231_t *ds, xtime_t *xt);
// Returns the current time as unix
// @return Return 0 if fails
uint32_t ds3231_nowUnix(ds3231_t *ds);

// -- Interrupt SQW functions
// Enable external interrupt on INT pin.
bool ds3231_enableIntSqwPin(ds3231_t *ds);
// Disable external interrupt on INT pin.
bool ds3231_disableIntSqwPin(ds3231_t *ds);
// Enable 1Hz output on INT/SQW pin.
bool ds3231_enable1HzOutput(ds3231_t *ds, bool en);

// -- Status register

// Clear status register
bool ds3231_clearSR(ds3231_t *ds);
// Writes status register.
bool ds3231_writeSR(ds3231_t *ds, uint8_t sr);
// Read status register
// @return Return -1 if fails
int16_t ds3231_readSR(ds3231_t *ds);
// Checks if ds3231 is running or not.
bool ds3231_isRunning(ds3231_t *ds);

// -- Alarms

// Set alarm 1 when hours, minutes, and seconds match, and enable INT pin
bool ds3231_setAlarm1(ds3231_t *ds, uint32_t u);
// Set alarm 2 when hours and minutes match, and enable INT pin
bool ds3231_setAlarm2(ds3231_t *ds, uint32_t u);

#endif