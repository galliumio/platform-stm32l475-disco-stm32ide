/*******************************************************************************
 * Copyright (C) Gallium Studio LLC. All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Alternatively, this program may be distributed and modified under the
 * terms of Gallium Studio LLC commercial licenses, which expressly supersede
 * the GNU General Public License and are specifically designed for licensees
 * interested in retaining the proprietary status of their code.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Contact information:
 * Website - https://www.galliumstudio.com
 * Source repository - https://github.com/galliumstudio
 * Email - admin@galliumstudio.com
 ******************************************************************************/

/**
  ******************************************************************************
  * @file    LTDC/LTDC_Display_1Layer/Src/main.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    25-June-2015
  * @brief   This example provides a description of how to configure LTDC peripheral
  *          to display BMP image on LCD using only one layer.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bsp.h"
#include "app_hsmn.h"
#include "fw.h"
#include "fw_macro.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "Console.h"
#include "System.h"
#include "GpioInAct.h"
#include "CompositeAct.h"
#include "SimpleAct.h"
#include "Demo.h"
#include "Ili9341Thread.h"
#include "SensorThread.h"
#include "WifiThread.h"
#include "Node.h"
#include "GpioOutAct.h"
#include "AOWashingMachine.h"
#include "Traffic.h"
#include "LevelMeter.h"
#include "UartAct.h"
#include "SystemInterface.h"
#include "ConsoleInterface.h"
#include "ConsoleCmd.h"

FW_DEFINE_THIS_FILE("main.cpp")

using namespace FW;
using namespace APP;

/** @addtogroup STM32F7xx_HAL_Examples
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
// Gallium - Test section placement.
// Todo - Create a memory pool for DMA use, with cache disabled.
//static System system  __attribute__ ((section (".dmatest")));
static System sys;
static Console consoleUart1(CONSOLE_UART1, "CONSOLE_UART1", "CMD_INPUT_UART1", "CMD_PARSER_UART1");
static CompositeAct compositeAct;
static SimpleAct simpleAct;
static Demo demo;
static GpioOutAct gpioOutAct;
static AOWashingMachine washingMachine;
static Traffic traffic;
static LevelMeter levelMeter;
static GpioInAct gpioInAct;
static UartAct uartAct1(UART1_ACT, "UART1_ACT", "UART1_IN", "UART1_OUT");
static Ili9341Thread ili9341Thread;
static SensorThread sensorThread;
static WifiThread wifiThread;
static Node node;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
int main(void)
{
    SystemClock_Config();

    // Initialize QP, framework and BSP (including HAL).
    Fw::Init();
    // Configure log settings.
    Log::SetVerbosity(4);
    Log::OnAll();
    Log::Off(UART1_IN);
    Log::Off(UART1_OUT);
    Log::Off(CMD_INPUT_UART1);
    Log::Off(CMD_PARSER_UART1);
    Log::Off(CONSOLE_UART1);
    Log::Off(ILI9341);
    Log::Off(SENSOR_ACCEL_GYRO);
    Log::Off(ACCEL_GYRO_INT);
    Log::Off(LEVEL_METER);
    Log::Off(NODE_PARSER);

    // Start active objects.
    compositeAct.Start(PRIO_COMPOSITE_ACT);
    simpleAct.Start(PRIO_SIMPLE_ACT);
    demo.Start(PRIO_DEMO);
    gpioOutAct.Start(PRIO_GPIO_OUT_ACT);
    washingMachine.Start(PRIO_AO_WASHING_MACHINE);
    traffic.Start(PRIO_TRAFFIC);
    levelMeter.Start(PRIO_LEVEL_METER);
    gpioInAct.Start(PRIO_GPIO_IN_ACT);
    uartAct1.Start(PRIO_UART1_ACT);
    consoleUart1.Start(PRIO_CONSOLE_UART1);
    ili9341Thread.Start(PRIO_ILI9341);
    sensorThread.Start(PRIO_SENSOR);
    wifiThread.Start(PRIO_WIFI);
    node.Start(PRIO_NODE);
    sys.Start(PRIO_SYSTEM);

    // Kick off the topmost active objects.
    Evt *evt;
    evt = new ConsoleStartReq(CONSOLE_UART1, HSM_UNDEF, 0, ConsoleCmd, UART1_ACT, true); //true);
    Fw::Post(evt);
    // CONSOLE_UART1 must not be started since it is used by WIFI (started in System).
    //evt = new ConsoleStartReq(CONSOLE_UART1, HSM_UNDEF, 0, ConsoleCmd, UART1_ACT, false);
    //Fw::Post(evt);
    evt = new SystemStartReq(SYSTEM, HSM_UNDEF, 0);
    Fw::Post(evt);
    return QP::QF::run();
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follows :
  *            System Clock source            = PLL (MSI)
  *            SYSCLK(Hz)                     = 80000000
  *            HCLK(Hz)                       = 80000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 4000000
  *            PLL_M                          = 1
  *            PLL_N                          = 40
  *            PLL_R                          = 2
  *            PLL_P                          = 7
  *            PLL_Q                          = 4
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* MSI is enabled after System reset, activate PLL with MSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLP = 7;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED4 on */
  BSP_LED_Init(LED2);
  BSP_LED_On(LED2);
  while(1)
  {
    /* Error if LED4 is slowly blinking (1 sec. period) */
    BSP_LED_Toggle(LED2);
    HAL_Delay(1000);
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
