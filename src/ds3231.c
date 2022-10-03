/**
 ******************************************************************************
 * @file    ds3231.c
 * @author  EonTeam
 * @version V1.0.1
 * @date    2020
 ******************************************************************************
 */

#include "ds3231.h"

// ===============================================================================
// Macros
// ===============================================================================

// DS3231 I2C Address
#define DS3231_ADDR ((uint8_t) 0xD0) // 0x68 << 1

// Registers
#define REG_TIMING_START      ((uint8_t) 0x00)
#define REG_ALARM1_START      ((uint8_t) 0x07)
#define REG_ALARM2_START      ((uint8_t) 0x0B)
#define REG_CONTROL           ((uint8_t) 0x0E)
#define REG_STATUS            ((uint8_t) 0x0F)
#define REG_AGING_OFFSET      ((uint8_t) 0x10)
#define REG_TEMPERATURE_START ((uint8_t) 0x11)

// ===============================================================================
// INT/SQW Functions
// ===============================================================================

bool ds3231_enableIntSqwPin(ds3231_t *ds) {
  if (ds->int_sqw_pin == IGNORE) {
    return false;
  }
  exti_attach(ds->int_sqw_pin, NOPULL, MODE_FALLING);
  return true;
}

bool ds3231_disableIntSqwPin(ds3231_t *ds) {
  exti_detach(ds->int_sqw_pin);
  return true;
}

bool ds3231_enable1HzOutput(ds3231_t *ds, bool en) {
  int8_t tries = 15;
  do {
    if (tries < 15) delay(10);
    ds->_i2cbuf[0] = REG_CONTROL;
    if (!i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 1, I2C_NOSTOP)) continue;
    if (!i2c_read(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 1, I2C_STOP)) continue;
    if (en) {
      bitClear(ds->_i2cbuf[0], 4); // needed for DS3231SN to select 1Hz output
      bitClear(ds->_i2cbuf[0], 3); // needed for DS3231SN to select 1Hz output
      bitClear(ds->_i2cbuf[0], 2); // enable 1Hz interrupt
    } else {
      bitSet(ds->_i2cbuf[0], 2); // disable 1Hz interrupt
    }
    ds->_i2cbuf[1] = ds->_i2cbuf[0];
    ds->_i2cbuf[0] = REG_CONTROL;
    if (!i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 2, I2C_STOP)) continue;
    return true;
  } while (tries--);
  return false;
}

// ===============================================================================
// Timing Functions
// ===============================================================================

bool ds3231_adjust(ds3231_t *ds, xtime_t *xt) {
  ds->_i2cbuf[0] = REG_TIMING_START; // start of timekeeping registers
  ds->_i2cbuf[1] = bin2bcd(xt->seconds) & 0x7F;
  ds->_i2cbuf[2] = bin2bcd(xt->minutes);
  ds->_i2cbuf[3] = bin2bcd(xt->hours) & 0x3F;
  ds->_i2cbuf[4] = bin2bcd(xt->weekday) & 0x07;
  ds->_i2cbuf[5] = bin2bcd(xt->day);
  ds->_i2cbuf[6] = bin2bcd(xt->month);
  ds->_i2cbuf[7] = bin2bcd(xt->year);
  return i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 8, I2C_STOP);
}

bool ds3231_adjustUnix(ds3231_t *ds, uint32_t u) {
  xtime_t xt;
  xtime_fromUnix(&xt, u);
  ds->_i2cbuf[0] = REG_TIMING_START; // start of timekeeping registers
  ds->_i2cbuf[1] = bin2bcd(xt.seconds) & 0x7F;
  ds->_i2cbuf[2] = bin2bcd(xt.minutes);
  ds->_i2cbuf[3] = bin2bcd(xt.hours) & 0x3F;
  ds->_i2cbuf[4] = bin2bcd(xt.weekday) & 0x07;
  ds->_i2cbuf[5] = bin2bcd(xt.day);
  ds->_i2cbuf[6] = bin2bcd(xt.month);
  ds->_i2cbuf[7] = bin2bcd(xt.year);
  return i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 8, I2C_STOP);
}

bool ds3231_now(ds3231_t *ds, xtime_t *xt) {
  ds->_i2cbuf[0] = REG_TIMING_START; // start of timekeeping registers
  if (!i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 1, I2C_NOSTOP)) { return false; }
  if (!i2c_read(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 7, I2C_STOP)) { return false; }
  xt->day     = bcd2bin(ds->_i2cbuf[4]);
  xt->month   = bcd2bin(ds->_i2cbuf[5]);
  xt->year    = bcd2bin(ds->_i2cbuf[6]);
  xt->weekday = bcd2bin(ds->_i2cbuf[3]);
  xt->hours   = bcd2bin(ds->_i2cbuf[2]);
  xt->minutes = bcd2bin(ds->_i2cbuf[1]);
  xt->seconds = bcd2bin(ds->_i2cbuf[0] & 0x7F);
  return true;
}

uint32_t ds3231_nowUnix(ds3231_t *ds) {
  xtime_t xt;
  int8_t tries = 10;
  do {
    if (tries < 10) delay(10);
    if (ds3231_now(ds, &xt)) { return xtime_toUnix(&xt); }
  } while (tries--);
  return 0;
}

// ===============================================================================
// Status Functions
// ===============================================================================

bool ds3231_writeSR(ds3231_t *ds, uint8_t sr) {
  ds->_i2cbuf[0] = REG_STATUS;
  ds->_i2cbuf[1] = sr; // status register bits
  return i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 2, I2C_STOP);
}

int16_t ds3231_readSR(ds3231_t *ds) {
  ds->_i2cbuf[0] = REG_STATUS;
  if (!i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 1, I2C_NOSTOP)) { return -1; }
  if (!i2c_read(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 1, I2C_STOP)) { return -1; }
  return (uint8_t) ds->_i2cbuf[0];
}

bool ds3231_clearSR(ds3231_t *ds) {
  return ds3231_writeSR(ds, 0x00);
}

bool ds3231_isRunning(ds3231_t *ds) {
  int8_t tries = 10;
  do {
    if (tries < 10) delay(10);
    int16_t sr = ds3231_readSR(ds);
    if (sr < 0) continue;
    sr &= ~(0x80);
    if (!ds3231_writeSR(ds, sr)) continue;
    delay(100);
    sr = ds3231_readSR(ds);
    if (sr < 0) continue;
    return (sr & 0x80) == 0;
  } while (tries--);
  return false;
}

// ===============================================================================
// Alarm Functions
// ===============================================================================

bool ds3231_setAlarm1(ds3231_t *ds, uint32_t u) {
  xtime_t xt;
  xtime_fromUnix(&xt, u);
  // precalculate time in bcd for ds3231 reg using the xtime
  xt.hours       = bin2bcd(xt.hours) & 0x3F;   // A1M3: 0, and hours, 24 hr format
  xt.minutes     = bin2bcd(xt.minutes) & 0x7F; // A1M2: 0, and minutes
  xt.seconds     = bin2bcd(xt.seconds) & 0x7F; // A1M1: 0, and seconds
  ds->_i2cbuf[0] = REG_ALARM1_START;
  if (!i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 1, I2C_NOSTOP)) { return false; }
  if (!i2c_read(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 9, I2C_STOP)) { return false; }
  ds->_i2cbuf[9] = ds->_i2cbuf[8] & ~0x01; // Clear Alarm 1 Interrupt
  ds->_i2cbuf[8] = ds->_i2cbuf[7] | 0x05;  // Enable Alarm 1 Interrupt and Interrupt Control bit
  ds->_i2cbuf[7] = ds->_i2cbuf[6];         // Leave Alarm 2 registers unchanged
  ds->_i2cbuf[6] = ds->_i2cbuf[5];         // Leave Alarm 2 registers unchanged
  ds->_i2cbuf[5] = ds->_i2cbuf[4];         // Leave Alarm 2 registers unchanged
  // Alarm when hours, minutes, and seconds match
  // -> A1M4 = 1, A1M3 = 0, A1M2 = 0, A1M1 = 0
  ds->_i2cbuf[4] = 0x80;             // A1M4: 1
  ds->_i2cbuf[3] = xt.hours;         // A1M3: 0
  ds->_i2cbuf[2] = xt.minutes;       // A1M2: 0
  ds->_i2cbuf[1] = xt.seconds;       // A1M1: 0
  ds->_i2cbuf[0] = REG_ALARM1_START; // Alarm 1 register
  if (!i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 10, I2C_STOP)) { return false; }
  // Check if alarm is configured OK!
  ds->_i2cbuf[0] = REG_ALARM1_START;
  if (!i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 1, I2C_NOSTOP)) { return false; }
  if (!i2c_read(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 9, I2C_STOP)) { return false; }
  if (ds->_i2cbuf[0] != xt.seconds) { return false; }    // check alarm 1 seconds
  if (ds->_i2cbuf[1] != xt.minutes) { return false; }    // check alarm 1 minutes
  if (ds->_i2cbuf[2] != xt.hours) { return false; }      // check alarm 1 hours
  if ((ds->_i2cbuf[3] & 0x80) != 0x80) { return false; } // check A1M4
  if ((ds->_i2cbuf[7] & 0x05) != 0x05) { return false; } // check if alarm 1 interrupt is enable
  if ((ds->_i2cbuf[8] & 0x01) != 0x00) { return false; } // check if alarm 1 flag is clear
  return true;
}

bool ds3231_setAlarm2(ds3231_t *ds, uint32_t u) {
  xtime_t xt;
  xtime_fromUnix(&xt, u);
  // precalculate time in bcd for ds3231 reg using the xtime
  xt.hours       = bin2bcd(xt.hours) & 0x3F;   // A2M3: 0, and hours, 24 hr format
  xt.minutes     = bin2bcd(xt.minutes) & 0x7F; // A2M2: 0, and minutes
  ds->_i2cbuf[0] = REG_ALARM2_START;
  if (!i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 1, I2C_NOSTOP)) { return false; }
  if (!i2c_read(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 5, I2C_STOP)) { return false; }
  ds->_i2cbuf[5] = ds->_i2cbuf[4] & ~0x02; // Clear Alarm 2 Interrupt
  ds->_i2cbuf[4] = ds->_i2cbuf[3] | 0x06;  // Enable Alarm 2 Interrupt and Interrupt Control bit
  // Alarm when hours and minutes match
  // -> A2M4 = 1, A2M3 = 0, A2M2 = 0
  ds->_i2cbuf[3] = 0x80;             // A2M4: 1
  ds->_i2cbuf[2] = xt.hours;         // A2M3: 0
  ds->_i2cbuf[1] = xt.minutes;       // A2M2: 0
  ds->_i2cbuf[0] = REG_ALARM2_START; // Alarm 2 register
  if (!i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 6, I2C_STOP)) { return false; }
  // Check if alarm is configured OK!
  ds->_i2cbuf[0] = REG_ALARM2_START;
  if (!i2c_write(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 1, I2C_NOSTOP)) { return false; }
  if (!i2c_read(ds->I2Cx, DS3231_ADDR, &(ds->_i2cbuf[0]), 5, I2C_STOP)) { return false; }
  if (ds->_i2cbuf[0] != xt.minutes) { return false; }    // check alarm 2 minutes
  if (ds->_i2cbuf[1] != xt.hours) { return false; }      // check alarm 2 hours
  if ((ds->_i2cbuf[2] & 0x80) != 0x80) { return false; } // check A2M4
  if ((ds->_i2cbuf[3] & 0x06) != 0x06) { return false; } // check if alarm 2 interrupt is enable
  if ((ds->_i2cbuf[4] & 0x02) != 0x00) { return false; } // check if alarm 2 flag is clear
  return true;
}