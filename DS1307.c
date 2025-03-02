/*
 * DS1307.c
 *
 *  Created on: Jul 14, 2024
 *      Author: Yoann Hervagault
 */

//sources derived from: https://github.com/jarzebski/Arduino-DS1307/tree/master

#include "DS1307.h"

sRTCDateTime t;
I2C_HandleTypeDef* sRTCI2C;

const uint8_t daysArray[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
const uint8_t dowArray[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

char *strDayOfWeek(uint8_t dayOfWeek);
char *strMonth(uint8_t month);
char *strAmPm(uint8_t hour, uint8_t uppercase);
char *strDaySufix(uint8_t day);

void readPacket(uint8_t offset, uint8_t * buff, uint8_t size);
void writePacket(uint8_t offset, uint8_t * buff, uint8_t size);

uint8_t hour12(uint8_t hour24);
uint8_t bcd2dec(uint8_t bcd);
uint8_t dec2bcd(uint8_t dec);

long time2long(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);
uint16_t date2days(uint16_t year, uint8_t month, uint8_t day);
uint8_t daysInMonth(uint16_t year, uint8_t month);
uint16_t dayInYear(uint16_t year, uint8_t month, uint8_t day);
uint8_t isLeapYear(uint16_t year);
uint8_t dow(uint16_t y, uint8_t m, uint8_t d);

uint32_t unixtime(void);
uint8_t conv2d(const char* p);

void writeRegister8(uint8_t reg, uint8_t value);


eDS1307Ret DS1307setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
	HAL_StatusTypeDef i2cResult;
	uint8_t dta[8];
	eDS1307Ret u8RetVal;
	dta[0] = DS1307_REG_TIME;
	dta[1] = dec2bcd(second);
	dta[2] = dec2bcd(minute);
	dta[3] = dec2bcd(hour);
	dta[4] = dec2bcd(dow(year, month, day));
	dta[5] = dec2bcd(day);
	dta[6] = dec2bcd(month);
	dta[7] = dec2bcd(year-2000);
	i2cResult = HAL_I2C_Master_Transmit(sRTCI2C, DS1307_ADDRESS, dta, 8, 100);
	if(i2cResult == HAL_OK)
	{
		u8RetVal = DS1307_success;
	}
	else{
		u8RetVal = DS1307_error;
	}
	return(u8RetVal);

}

void setDateTimeFromInt(uint32_t t)
{
    t -= 946681200;

    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    second = t % 60;
    t /= 60;

    minute = t % 60;
    t /= 60;

    hour = t % 24;
    uint16_t days = t / 24;
    uint8_t leap;

    for (year = 0; ; ++year)
    {
        leap = year % 4 == 0;
        if (days < 365 + leap)
        {
            break;
        }
        days -= 365 + leap;
    }

    for (month = 1; ; ++month)
    {
        uint8_t daysPerMonth = daysArray[month - 1];

        if (leap && month == 2)
        {
            ++daysPerMonth;
        }

        if (days < daysPerMonth)
        {
            break;
        }
        days -= daysPerMonth;
    }

    day = days + 1;

    DS1307setDateTime(year+2000, month, day, hour, minute, second);
}

eDS1307Ret setDateTime(const char* date, const char* time)
{
	eDS1307Ret u8RetVal;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    year = conv2d(date + 9);

    switch (date[0])
    {
        case 'J':
        	month = (date[1] == 'a' ? 1 : (month = date[2] == 'n' ? 6 : 7));
        	break;
        case 'F': month = 2; break;
        case 'A': month = date[2] == 'r' ? 4 : 8; break;
        case 'M': month = date[2] == 'r' ? 3 : 5; break;
        case 'S': month = 9; break;
        case 'O': month = 10; break;
        case 'N': month = 11; break;
        case 'D': month = 12; break;
    }

    day = conv2d(date + 4);
    hour = conv2d(time);
    minute = conv2d(time + 3);
    second = conv2d(time + 6);


    u8RetVal = DS1307setDateTime(year+2000, month, day, hour, minute, second);
    return(u8RetVal);
}

void dateFormat(const char* dateFormat, sRTCDateTime dt, char* buffer)
{

    buffer[0] = 0;

    char helper[11];

    while (*dateFormat != '\0')
    {
        switch (dateFormat[0])
        {
            // Day decoder
            case 'd':
                sprintf(helper, "%02d", dt.u8day);
                strcat(buffer, (const char *)helper);
                break;
            case 'j':
                sprintf(helper, "%d", dt.u8day);
                strcat(buffer, (const char *)helper);
                break;
            case 'l':
                strcat(buffer, (const char *)strDayOfWeek(dt.u8dayOfWeek));
                break;
            case 'D':
                strncat(buffer, strDayOfWeek(dt.u8dayOfWeek), 3);
                break;
            case 'N':
                sprintf(helper, "%d", dt.u8dayOfWeek);
                strcat(buffer, (const char *)helper);
                break;
            case 'w':
                sprintf(helper, "%d", (dt.u8dayOfWeek + 7) % 7);
                strcat(buffer, (const char *)helper);
                break;
            case 'z':
                sprintf(helper, "%d", dayInYear(dt.u16year, dt.u8month, dt.u8day));
                strcat(buffer, (const char *)helper);
                break;
            case 'S':
                strcat(buffer, (const char *)strDaySufix(dt.u8day));
                break;

            // Month decoder
            case 'm':
                sprintf(helper, "%02d", dt.u8month);
                strcat(buffer, (const char *)helper);
                break;
            case 'n':
                sprintf(helper, "%d", dt.u8month);
                strcat(buffer, (const char *)helper);
                break;
            case 'F':
                strcat(buffer, (const char *)strMonth(dt.u8month));
                break;
            case 'M':
                strncat(buffer, (const char *)strMonth(dt.u8month), 3);
                break;
            case 't':
                sprintf(helper, "%d", daysInMonth(dt.u16year, dt.u8month));
                strcat(buffer, (const char *)helper);
                break;

            // Year decoder
            case 'Y':
                sprintf(helper, "%d", dt.u16year);
                strcat(buffer, (const char *)helper);
                break;
            case 'y': sprintf(helper, "%02d", dt.u16year-2000);
                strcat(buffer, (const char *)helper);
                break;
            case 'L':
                sprintf(helper, "%d", isLeapYear(dt.u16year));
                strcat(buffer, (const char *)helper);
                break;

            // Hour decoder
            case 'H':
                sprintf(helper, "%02d", dt.u8hour);
                strcat(buffer, (const char *)helper);
                break;
            case 'G':
                sprintf(helper, "%d", dt.u8hour);
                strcat(buffer, (const char *)helper);
                break;
            case 'h':
                sprintf(helper, "%02d", hour12(dt.u8hour));
                strcat(buffer, (const char *)helper);
                break;
            case 'g':
                sprintf(helper, "%d", hour12(dt.u8hour));
                strcat(buffer, (const char *)helper);
                break;
            case 'A':
                strcat(buffer, (const char *)strAmPm(dt.u8hour, 1));
                break;
            case 'a':
                strcat(buffer, (const char *)strAmPm(dt.u8hour, 0));
                break;

            // Minute decoder
            case 'i':
                sprintf(helper, "%02d", dt.u8minute);
                strcat(buffer, (const char *)helper);
                break;

            // Second decoder
            case 's':
                sprintf(helper, "%02d", dt.u8second);
                strcat(buffer, (const char *)helper);
                break;

            // Misc decoder
            case 'U':
                sprintf(helper, "%lu", dt.u32unixtime);
                strcat(buffer, (const char *)helper);
                break;

            default:
                strncat(buffer, dateFormat, 1);
                break;
        }
        dateFormat++;
    }
}

sRTCDateTime getDateTime(void)
{
    int values[7];
	uint8_t u8RData[7]={0};
	uint8_t u8TData[1];
	u8TData[0] = DS1307_REG_TIME;
	HAL_I2C_Master_Transmit(sRTCI2C, DS1307_ADDRESS, u8TData, 1, 100);
	HAL_I2C_Master_Receive(sRTCI2C, DS1307_ADDRESS|0x01, u8RData, 7, 100);
	/*for(int i=0;i<7;i++)
	{
		uint8_t u8TData[1] = {DS1307_REG_TIME+i};
		uint8_t u8RData[1] = {0};
		HAL_I2C_Master_Transmit(sRTCI2C, DS1307_ADDRESS, u8TData, 1, 100);
		HAL_I2C_Master_Receive(sRTCI2C, DS1307_ADDRESS|0x01, u8RData, 1, 100);
		if (i == 3)
		{
			values[i] = (int)u8RData[0];
		}
		else if (i==0)
		{
			//ignore CH bit
			values[i] = (int)(u8RData[0]&0x7F);
		}
		else
		{
			values[i] = bcd2dec(u8RData[0]);
        }
	}*/
    for (int i = 0; i < 7; i++)
    {
		if (i == 3)
		{
			values[i] = (int)u8RData[i];
		}
		else if (i==0)
		{
			//ignore CH bit
			values[i] = bcd2dec(u8RData[i]&0x7F);
		}
		else
		{
			values[i] = bcd2dec(u8RData[i]);
        }
    }

    t.u16year = values[6] + 2000;
    t.u8month = values[5];
    t.u8day = values[4];
    t.u8dayOfWeek = values[3];
    t.u8hour = values[2];
    t.u8minute = values[1];
    t.u8second = values[0];
    t.u32unixtime = unixtime();

    return t;
}

uint8_t isReady(void)
{
	HAL_StatusTypeDef sRetVal;
	sRetVal = HAL_I2C_IsDeviceReady(sRTCI2C, DS1307_ADDRESS,1,100);
	if(sRetVal==HAL_OK)
	{
		return(DS1307_success);
	}else
	{
		return(DS1307_notReady);
	}
	//uint8_t ss = readRegister8(DS1307_REG_TIME);
	//return !(ss>>7);
}

uint8_t readByte(uint8_t offset)
{
    uint8_t buff[1];

    readPacket(offset, buff, 1);

    return (uint8_t)buff[0];
}

uint8_t writeByte(uint8_t offset, uint8_t data)
{
    uint8_t buff[1];

    buff[0] = data;

    writePacket(offset, buff, 1);
}

void readPacket(uint8_t offset, uint8_t * buff, uint8_t size)
{
    uint8_t RegBuf = DS1307_REG_RAM + offset;
    HAL_I2C_Master_Transmit(sRTCI2C, DS1307_ADDRESS, &RegBuf, 1, 100);
    HAL_I2C_Master_Receive(sRTCI2C, DS1307_ADDRESS|0x01, buff, size, 100);
}

void writePacket(uint8_t offset, uint8_t * buff, uint8_t size)
{
	uint8_t data = DS1307_REG_RAM + offset;
	HAL_I2C_Master_Transmit(sRTCI2C, DS1307_ADDRESS, &data, size, 100);
    HAL_I2C_Master_Transmit(sRTCI2C, DS1307_ADDRESS, buff, size, 100);
}

void readMemory(uint8_t offset, uint8_t * buff, uint8_t size)
{
    if (size > 56)
    {
        size = 56;
    }

    if (size > 31)
    {
        readPacket(offset, buff, size);
        readPacket(offset + 31, buff + 31, size - 31);
    } else
    {
        readPacket(offset, buff, size);
    }
}

void writeMemory(uint8_t offset, uint8_t * buff, uint8_t size)
{
    if (size > 56)
    {
        size = 56;
    }

    if (size > 31)
    {
        writePacket(offset, buff, 31);
        writePacket(offset+31, buff+31, size-31);
    } else
    {
        writePacket(offset, buff, size);
    }
}

void clearMemory(void)
{
    for (uint8_t offset = 0; offset < 56; offset++)
    {
        writeByte(offset, 0);
    }
}

void setOutputMode(ds1307_sqwOut_t mode)
{
    writeRegister8(DS1307_REG_CONTROL, mode);
}

void setOutput(uint8_t high)
{
    if (high)
    {
        writeRegister8(DS1307_REG_CONTROL, DS1307_HIGH);
    } else
    {
        writeRegister8(DS1307_REG_CONTROL, DS1307_LOW);
    }
}

ds1307_sqwOut_t getOutput(void)
{
    uint8_t value;
    value = readRegister8(DS1307_REG_CONTROL);
    return (ds1307_sqwOut_t)value;
}

uint8_t bcd2dec(uint8_t bcd)
{
    return ((bcd / 16) * 10) + (bcd % 16);
}

uint8_t dec2bcd(uint8_t dec)
{
    return ((dec / 10) * 16) + (dec % 10);
}

char *strDayOfWeek(uint8_t dayOfWeek)
{
    switch (dayOfWeek) {
        case 1:
            return "Monday";
            break;
        case 2:
            return "Tuesday";
            break;
        case 3:
            return "Wednesday";
            break;
        case 4:
            return "Thursday";
            break;
        case 5:
            return "Friday";
            break;
        case 6:
            return "Saturday";
            break;
        case 7:
            return "Sunday";
            break;
        default:
            return "Unknown";
    }
}

char *strMonth(uint8_t month)
{
    switch (month) {
        case 1:
            return "January";
            break;
        case 2:
            return "February";
            break;
        case 3:
            return "March";
            break;
        case 4:
            return "April";
            break;
        case 5:
            return "May";
            break;
        case 6:
            return "June";
            break;
        case 7:
            return "July";
            break;
        case 8:
            return "August";
            break;
        case 9:
            return "September";
            break;
        case 10:
            return "October";
            break;
        case 11:
            return "November";
            break;
        case 12:
            return "December";
            break;
        default:
            return "Unknown";
    }
}

char *strAmPm(uint8_t hour, uint8_t uppercase)
{
    if (hour < 12)
    {
        if (uppercase)
        {
            return "AM";
        } else
        {
            return "am";
        }
    } else
    {
        if (uppercase)
        {
            return "PM";
        } else
        {
            return "pm";
        }
    }
}

char *strDaySufix(uint8_t day)
{
    if (day % 10 == 1)
    {
        return "st";
    } else
    if (day % 10 == 2)
    {
        return "nd";
    }
    if (day % 10 == 3)
    {
        return "rd";
    }

    return "th";
}

uint8_t hour12(uint8_t hour24)
{
    if (hour24 == 0)
    {
        return 12;
    }

    if (hour24 > 12)
    {
       return (hour24 - 12);
    }

    return hour24;
}

long time2long(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    return ((days * 24L + hours) * 60 + minutes) * 60 + seconds;
}

uint16_t dayInYear(uint16_t year, uint8_t month, uint8_t day)
{
    uint16_t fromDate;
    uint16_t toDate;

    fromDate = date2days(year, 1, 1);
    toDate = date2days(year, month, day);

    return (toDate - fromDate);
}

uint8_t isLeapYear(uint16_t year)
{
    return (year % 4 == 0);
}

uint8_t daysInMonth(uint16_t year, uint8_t month)
{
    uint8_t days;

    days = daysArray[month - 1];

    if ((month == 2) && isLeapYear(year))
    {
        ++days;
    }

    return days;
}

uint16_t date2days(uint16_t year, uint8_t month, uint8_t day)
{
    year = year - 2000;

    uint16_t days16 = day;

    for (uint8_t i = 1; i < month; ++i)
    {
        days16 += daysArray[i - 1];
    }

    if ((month == 2) && isLeapYear(year))
    {
        ++days16;
    }

    return days16 + 365 * year + (year + 3) / 4 - 1;
}

uint32_t unixtime(void)
{
    uint32_t u;

    u = time2long(date2days(t.u16year, t.u8month, t.u8day), t.u8hour, t.u8minute, t.u8second);
    u += 946681200+3600;

    return u;
}

uint8_t conv2d(const char* p)
{
    uint8_t v = 0;

    if ('0' <= *p && *p <= '9')
    {
        v = *p - '0';
    }

    return 10 * v + *++p - '0';
}

uint8_t dow(uint16_t y, uint8_t m, uint8_t d)
{
    uint8_t dow;

    y -= m < 3;
    dow = ((y + y/4 - y/100 + y/400 + (dowArray[m-1]) + d) % 7);

    if (dow == 0)
    {
        return 7;
    }

    return dow;
}

void  writeRegister8(uint8_t reg, uint8_t value)
{
	uint8_t u8TData[2];
	u8TData[0] = reg;
	u8TData[1] = value;
	HAL_I2C_Master_Transmit(sRTCI2C, DS1307_ADDRESS, u8TData, 1, 100);
}

uint8_t readRegister8(uint8_t reg)
{
	uint8_t u8RData[1];
	uint8_t u8TData[1];
	u8TData[0] = reg;
	HAL_I2C_Master_Transmit(sRTCI2C, DS1307_ADDRESS, u8TData, 1, 100);
	HAL_I2C_Master_Receive(sRTCI2C, DS1307_ADDRESS|0x01, u8RData, 1, 100);
    return(u8RData[0]);
}

void DS1307Init(I2C_HandleTypeDef* fi2cHandle)
{
	sRTCI2C = fi2cHandle;
    t.u16year = 2000;
    t.u8month = 1;
    t.u8day = 1;
    t.u8hour = 0;
    t.u8minute = 0;
    t.u8second = 0;
    t.u8dayOfWeek = 6;
    t.u32unixtime = 946681200;
}

uint8_t DS1307GetCHbit()
{
	uint8_t reg0 = readRegister8(0);
	return(reg0>>7);
}
