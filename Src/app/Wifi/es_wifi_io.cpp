/**
  ******************************************************************************
  * @file    es_wifi_io.c
  * @author  MCD Application Team
  * @brief   This file implements the IO operations to deal with the es-wifi
  *          module. It mainly Inits and Deinits the SPI interface. Send and
  *          receive data over it.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "es_wifi.h"
#include "es_wifi_io.h"
#include <string.h>
#include "es_wifi_conf.h"
#include <core_cm4.h>
#include "Wifi.h"
#include "bsp.h"
#include "fw_log.h"

FW_DEFINE_THIS_FILE("es_wifi_io.cpp")
#define WIFI_WARNING(format_, ...)      Log::Debug(Log::TYPE_WARNING, WIFI, format_, ## __VA_ARGS__)
#define WIFI_LOG(format_, ...)          Log::Debug(Log::TYPE_LOG, WIFI, format_, ## __VA_ARGS__)
#define WIFI_INFO(format_, ...)         Log::Debug(Log::TYPE_INFO, WIFI, format_, ## __VA_ARGS__)

using namespace FW;
using namespace APP;

/* Private define ------------------------------------------------------------*/
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static  void SPI_WIFI_DelayUs(uint32_t);
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
                       COM Driver Interface (SPI)
*******************************************************************************/

/**
  * @brief  Initialize the SPI3
  * @param  None
  * @retval None
  */
int8_t SPI_WIFI_Init(uint16_t mode)
{
  int8_t  rc=0;

  if (mode == ES_WIFI_INIT)
  {
    /* first call used for calibration */
    SPI_WIFI_DelayUs(10);
  }
  
  rc= SPI_WIFI_ResetModule();

  return rc;
}


int8_t SPI_WIFI_ResetModule(void)
{
  uint8_t Prompt[6];
  uint8_t count = 0;

  // Deactivates NSS in case it is still activated since it is NOT deactivated at the end of SPI_WIFI_SendData().
  WIFI_DISABLE_NSS();
  SPI_WIFI_DelayUs(3);

  WIFI_RESET_MODULE();
  WIFI_ENABLE_NSS(); 
  SPI_WIFI_DelayUs(15);

  memset(Prompt, 0, sizeof(Prompt));
  while (WIFI_IS_CMDDATA_READY() && (count < (sizeof(Prompt) - 1)))
  {
    bool status = Wifi::SpiReadInt(&Prompt[count], 1, 10000);
    count += 2;
    if(!status)
    {
      WIFI_DISABLE_NSS();
      return -1;
    }    
  }
  
  if (WIFI_IS_CMDDATA_READY()) {
    WIFI_WARNING("SPI_WIFI_ResetModule - CmdDataRdy remains high after prompt[count=%d]", count);
    WIFI_DISABLE_NSS();
    return -1;
  }

  // Initializes semaphore to not-signalled. It will be signalled at the next rising edge.
  // It MUST be done before disabling NSS/CS to ensure CmdDataRdy remains low.
  Wifi::ClearCmdDataRdySem();

  WIFI_DISABLE_NSS();
  if((Prompt[0] != 0x15) ||(Prompt[1] != 0x15) ||(Prompt[2] != '\r')||
       (Prompt[3] != '\n') ||(Prompt[4] != '>') ||(Prompt[5] != ' '))
  {
    WIFI_WARNING("SPI_WIFI_ResetModule - Invalid prompt[%x %x %x %x %x %x]",
                 Prompt[0], Prompt[1], Prompt[2], Prompt[3], Prompt[4], Prompt[5]);
    return -1;
  }    
  return 0;
}

/**
  * @brief  DeInitialize the SPI
  * @param  None
  * @retval None
  */
int8_t SPI_WIFI_DeInit(void)
{
  return 0;
}

int16_t SPI_WIFI_ReceiveData(uint8_t *pData, uint16_t len, uint32_t timeout)
{
  int16_t length = 0;
  uint8_t tmp[2];
  
  // For logging.
  uint8_t *p = pData;

  // At the end of SPI_WIFI_SendData(), NSS is NOT deactivated to allow continued write.
  // NSS is therefore deactivated at the beginning or ReceiveData. It is OK if it is already deactivated.
  WIFI_DISABLE_NSS();
  UNLOCK_SPI();
  SPI_WIFI_DelayUs(3);

  // This is required for the current (old) version of STM32 SPI library.
  // The SPI Rx FIFO (4 bytes) has been filled in by previous SPI writes. If not flushed, the next SPI read
  // returns old data.
  Wifi::FlushSpiRxFifo();

  // CmdDataRdy must transition fro L to H to indicate data is ready to be read.
  if (!Wifi::WaitCmdDataRdyHigh(timeout))
  {
      WIFI_WARNING("SPI_WIFI_ReceiveData WaitCmdDataRdyHigh TIMEOUT (CmdDataRdy = %d)", WIFI_IS_CMDDATA_READY());
      return ES_WIFI_ERROR_WAITING_DRDY_FALLING;
  }
  LOCK_SPI();
  WIFI_ENABLE_NSS();
  SPI_WIFI_DelayUs(15);
  while (WIFI_IS_CMDDATA_READY())
  {
    // @todo MUST fix buffer overrun issue (len MUST be specified)
    if((length < len) || (!len))
    {
      if (!Wifi::SpiReadInt(tmp, 1, timeout)) {
        WIFI_DISABLE_NSS();
        UNLOCK_SPI();
        return ES_WIFI_ERROR_SPI_FAILED;
      }
      pData[0] = tmp[0];
      pData[1] = tmp[1];
      length += 2;
      pData  += 2;
      
      // It assumes if len == 0, it uses the CmdData[] buffer in ES_WIFIObject_t.
      if (length >= ES_WIFI_DATA_SIZE) {
        Log::DebugBuf(Log::TYPE_INFO, WIFI, pData, length, 1, 0);
        WIFI_DISABLE_NSS();
        UNLOCK_SPI();
        SPI_WIFI_ResetModule();
        return ES_WIFI_ERROR_STUFFING_FOREVER;
      }     
    }
    else
    {
      break;
    }
  }

  if (WIFI_IS_CMDDATA_READY()) {
    WIFI_WARNING("SPI_WIFI_ReceiveData - CmdDataRdy remains high after read[length=%d]", length);
  }
  Log::DebugBuf(Log::TYPE_INFO, WIFI, p, length, 1, 0);

  WIFI_DISABLE_NSS();
  UNLOCK_SPI();
  return length;
}

// Not used - Replaced by Wifi::WaitCmdDataRdyHigh().
/*
int wait_cmddata_rdy_high(int timeout)
{
  int tickstart = HAL_GetTick();
  while (WIFI_IS_CMDDATA_READY()==0)
  {
    if((HAL_GetTick() - tickstart ) > timeout)
    {
      return -1;
    }
  }
  return 0;
}
*/

/**
  * @brief  Send wifi Data thru SPI
  * @param  pdata : pointer to data
  * @param  len : Data length
  * @param  timeout : send timeout in mS
  * @retval Length of sent data
  */
int16_t SPI_WIFI_SendData( uint8_t *pdata,  uint16_t len, uint32_t timeout)
{
  uint8_t Padding[2];
  
  // Since SPI_WIFI_SendData() may be called again while CmdDataRdy stay active, an L->H transition may not arrive,
  // i.e. a semaphore may not be signalled. It therefore uses polling.
  uint32_t tickstart = HAL_GetTick();
  while (WIFI_IS_CMDDATA_READY()==0)
  {
    if((HAL_GetTick() - tickstart ) > timeout) {
      return ES_WIFI_ERROR_WAITING_DRDY_FALLING;
    }
    Wifi::DelayMs(1);
  }
  // Acquire/clear semaphore if it has been signalled, which is the case case of first write after read.
  // In case of continued write, semaphore should be cleared and it's okay to clear again.
  Wifi::ClearCmdDataRdySem();
    
  LOCK_SPI();
  WIFI_ENABLE_NSS();
  SPI_WIFI_DelayUs(15);
  if (len > 1)
  {
    if(!Wifi::SpiWriteInt(pdata, len/2, timeout))
    {
      WIFI_DISABLE_NSS();
      UNLOCK_SPI();
      return ES_WIFI_ERROR_SPI_FAILED;
    }
  }
  
  if ( len & 1)
  {
    Padding[0] = pdata[len-1];
    Padding[1] = '\n';

    if(!Wifi::SpiWriteInt(Padding, 1, timeout))
    {
      WIFI_DISABLE_NSS();
      UNLOCK_SPI();
      return ES_WIFI_ERROR_SPI_FAILED;
    }
  }
  // Does not deactivate NSS to allow continued write.
  // It will be deactivated at the beginning of the next read in SPI_WIFI_ReceiveData().
  Log::DebugBuf(Log::TYPE_INFO, WIFI, pdata, len, 1, 0);
  return len;
}

/**
  * @brief  Delay
  * @param  Delay in ms
  * @retval None
  */
void SPI_WIFI_Delay(uint32_t Delay)
{
    Wifi::DelayMs(Delay);
}

/**
   * @brief  Delay
  * @param  Delay in us
  * @retval None
  */
void SPI_WIFI_DelayUs(uint32_t n)
{
  volatile        uint32_t ct = 0;
  uint32_t        loop_per_us = 0;
  static uint32_t cycle_per_loop = 0;

  /* calibration happen on first call for a duration of 1 ms * nbcycle per loop */
  /* 10 cycle for STM32L4 */
  if (cycle_per_loop == 0 ) 
  {
     uint32_t cycle_per_ms = (SystemCoreClock/1000UL);
     uint32_t t = 0;
     ct = cycle_per_ms;
     t = GetSystemMs();
     while(ct) ct--;
     cycle_per_loop = GetSystemMs()-t;
     if (cycle_per_loop == 0) cycle_per_loop = 1;
  }

  loop_per_us = SystemCoreClock/1000000UL/cycle_per_loop;
  ct = n * loop_per_us;
  while(ct) ct--;
  return;
}


/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */

/**
  * @}
  */ 
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
