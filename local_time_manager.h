/*
 * local_time_manager.h
 *
 *  Created on: Mar 2, 2025
 *      Author: yoyos
 */

#ifndef DS1307_LOCAL_TIME_MANAGER_H_
#define DS1307_LOCAL_TIME_MANAGER_H_

#include "DS1307.h"

typedef struct {
	TIM_HandleTypeDef* baseTimer;
	uint32_t u32LocalTime;
}ts_ltm;

typedef enum
{
	ltm_success = 0,
	ltm_error,
	ltm_parameterError,
	ltm_notReady
}eLtmRet;

eLtmRet LtmInit(TIM_HandleTypeDef* htime);
eLtmRet LtmRefLocalTime(void);
eLtmRet ltm_UnixToDateTime(uint32_t u32UnixTime, sRTCDateTime* dateResult);

#endif /* DS1307_LOCAL_TIME_MANAGER_H_ */
