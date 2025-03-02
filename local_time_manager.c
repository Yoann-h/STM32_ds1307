/*
 * local_time_manager.c
 *
 *  Created on: Mar 2, 2025
 *      Author: yoyos
 */

#include "local_time_manager.h"

ts_ltm LocalTime;

uint32_t ltm_verifTimer(TIM_HandleTypeDef* ftim);

eLtmRet LtmInit(TIM_HandleTypeDef* htime)
{
	eLtmRet eRet = ltm_error;
	LocalTime.u32LocalTime = 0;
	if(ltm_verifTimer(htime)*100==(htime->Instance->PSC +1)*1000000 && htime->Instance->ARR==10000){
		LocalTime.baseTimer = htime;
		eRet = ltm_success;
		if (HAL_TIM_Base_Start_IT(htime) != HAL_OK)
		{
			eRet = ltm_error;
		}
	}
	else
	{
		eRet = ltm_parameterError;
	}


return eRet;
}

eLtmRet LtmRefLocalTime(void)
{
	sRTCDateTime dt = getDateTime();
	LocalTime.u32LocalTime = dt.u32unixtime;
}

uint32_t ltm_verifTimer(TIM_HandleTypeDef* ftim)
{
  /* Get PCLK1 frequency */
  uint32_t pclkx;
  if(ftim->Instance == TIM3 || ftim->Instance == TIM4 || ftim->Instance == TIM5 || ftim->Instance == TIM6 || ftim->Instance == TIM7)
  {
	  pclkx = HAL_RCC_GetPCLK1Freq();
	  if((RCC->CFGR & RCC_CFGR_PPRE1) != 0)
	  {
		  pclkx *=2;
	  }
  }
  else
  {
	  pclkx = HAL_RCC_GetPCLK2Freq();
	  if((RCC->CFGR & RCC_CFGR_PPRE2) != 0)
	  {
		  pclkx *=2;
	  }
  }
  return pclkx;
}


//from https://www.oryx-embedded.com/doc/date__time_8c_source.html
eLtmRet ltm_UnixToDateTime(uint32_t u32UnixTime, sRTCDateTime* dateResult)
{
	 /**
	  * @brief Convert Unix timestamp to date
	  * @param[in] t Unix timestamp
	  * @param[out] date Pointer to a structure representing the date and time
	  **/
	eLtmRet eRet = ltm_error;
	    uint32_t a;
	    uint32_t b;
	    uint32_t c;
	    uint32_t d;
	    uint32_t e;
	    uint32_t f;

	    //Retrieve hours, minutes and seconds
	    dateResult->u8second = u32UnixTime % 60;
	    u32UnixTime /= 60;
	    dateResult->u8minute = u32UnixTime % 60;
	    u32UnixTime /= 60;
	    dateResult->u8hour = u32UnixTime % 24;
	    u32UnixTime /= 24;

	    //Convert Unix time to date
	    a = (uint32_t) ((4 * u32UnixTime + 102032) / 146097 + 15);
	    b = (uint32_t) (u32UnixTime + 2442113 + a - (a / 4));
	    c = (20 * b - 2442) / 7305;
	    d = b - 365 * c - (c / 4);
	    e = d * 1000 / 30601;
	    f = d - e * 30 - e * 601 / 1000;

	    //January and February are counted as months 13 and 14 of the previous year
	    if(e <= 13)
	    {
	       c -= 4716;
	       e -= 1;
	    }
	    else
	    {
	       c -= 4715;
	       e -= 13;
	    }

	    //Retrieve year, month and day
	    dateResult->u16year = c;
	    dateResult->u8month = e;
	    dateResult->u8day = f;

	 return eRet;
}
