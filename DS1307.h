/*
 * DS1307.h
 *
 *  Created on: Jul 14, 2024
 *      Author: Yoann Hervagault
 */

//sources derived from: https://github.com/jarzebski/Arduino-DS1307/tree/master
//hardware used: https://wiki.keyestudio.com/KS0516_Keyestudio_Expansion_Board%2BTF_Card_for_Data_Recording

#ifndef RTC_DS1307_H_
#define RTC_DS1307_H_

#include "stm32l4xx_hal.h"

#define DS1307_ADDRESS              (0xD0)

#define DS1307_REG_TIME             (0x00)
#define DS1307_REG_CONTROL          (0x07)
#define DS1307_REG_RAM              (0x08)

#ifndef RTCDATETIME_STRUCT_H
#define RTCDATETIME_STRUCT_H
typedef struct
{
    uint16_t u16year;
    uint8_t u8month;
    uint8_t u8day;
    uint8_t u8hour;
    uint8_t u8minute;
    uint8_t u8second;
    uint8_t u8dayOfWeek;
    uint32_t u32unixtime;
} sRTCDateTime;
#endif

typedef enum
{
    DS1307_LOW          = 0x00,
    DS1307_HIGH         = 0x80,
    DS1307_1HZ          = 0x10,
    DS1307_4096HZ       = 0x11,
    DS1307_8192HZ       = 0x12,
    DS1307_32768HZ      = 0x13
} ds1307_sqwOut_t;

typedef enum
{
	DS1307_success = 0,
	DS1307_error,
	DS1307_notReady
}eDS1307Ret;

uint8_t readRegister8(uint8_t reg);

void DS1307Init(I2C_HandleTypeDef* fi2cHandle);
eDS1307Ret DS1307setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
void setDateTimeFromInt(uint32_t t);
eDS1307Ret setDateTime(const char* date, const char* time);
sRTCDateTime getDateTime(void);
uint8_t isReady(void);

uint8_t readByte(uint8_t offset);
uint8_t writeByte(uint8_t offset, uint8_t data);

void readMemory(uint8_t offset, uint8_t * buff, uint8_t size);
void writeMemory(uint8_t offset, uint8_t * buff, uint8_t size);

void clearMemory(void);

ds1307_sqwOut_t getOutput(void);
void setOutputMode(ds1307_sqwOut_t mode);
void setOutput(uint8_t mode);

void dateFormat(const char* dateFormat, sRTCDateTime dt, char* buffer);
uint8_t DS1307GetCHbit();

#endif /* RTC_DS1307_H_ */
