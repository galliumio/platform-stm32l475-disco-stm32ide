/*******************************************************************************
 * Copyright (C) 2018 Gallium Studio LLC (Lawrence Lo). All rights reserved.
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
  * @file    stm32f7xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2015 STMicroelectronics
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
#include "qpcpp.h"
#include "app_hsmn.h"
#include "UartAct.h"
#include "Ili9341.h"
#include "GpioIn.h"
#include "Sensor.h"
#include "Wifi.h"
#include "fw_log.h"

/* USER CODE BEGIN 0 */

using namespace FW;
using namespace APP;

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M7 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System tick timer.
*/

extern "C" void SysTick_Handler(void){
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  HAL_SYSTICK_IRQHandler();
  /* USER CODE BEGIN SysTick_IRQn 1 */
  QXK_ISR_ENTRY();
  QP::QF::tickX_(TICK_RATE_BSP);
  QXK_ISR_EXIT();
  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f7xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

// Common handler for UART interrupts.
// It handles RX error and RXNE interrupts. It does NOT handle TX interrupts.
// It does NOT pass control to HAL handler.
void HandleUartIrq(Hsmn hsmn) {
    UartIn::HwError error = UartIn::HW_ERROR_NONE;
    UART_HandleTypeDef *hal = UartAct::GetHal(hsmn);
    volatile uint32_t isrflags   = READ_REG(hal->Instance->ISR);
    if (isrflags & USART_ISR_NE) {
        // START bit Noise detection flag. Must clear it or it will cause ISR to be re-entered.
        __HAL_UART_CLEAR_NEFLAG(hal);
        error |= UartIn::HW_ERROR_NOISE;
    }
    if (isrflags & USART_ISR_FE) {
        // Frame error flag. Must clear it or it will cause ISR to be re-entered.
        __HAL_UART_CLEAR_FEFLAG(hal);
        error |= UartIn::HW_ERROR_FRAME;
    }
    if (isrflags & USART_ISR_ORE) {
        // Overrun error. Reset by setting ORECF in ICR. Alternative, disable ORE by setting OVRDIS.
        // ORE will trigger interrupt when RXNE interrupt is enabled.
        // With DMA, overrun should not occur.
        __HAL_UART_CLEAR_OREFLAG(hal);
        error |= UartIn::HW_ERROR_OVERRUN;
    }
    // Do not check RXNEIE bit as it may have been cleared automatically by DMA.
    // It is okay to not check as we don't use other UART interrupts (except errors above).
    // In case an error interrupt occur, it is okay to treat it as if RXNE has occurred.
    // Disable interrupt to avoid re-entering ISR before event is processed.
    CLEAR_BIT(hal->Instance->CR1, USART_CR1_RXNEIE);
    UartIn::RxCallback(UartAct::GetUartInHsmn(hsmn), error);
    // TX does not use ISR. Do not call HAL handler.
    //HAL_UART_IRQHandler(hal);
}

// UART1 TX DMA
// Must be declared as extern "C" in header.
extern "C" void DMA2_Channel6_IRQHandler(void) {
    QXK_ISR_ENTRY();
    UART_HandleTypeDef *hal = UartAct::GetHal(UART1_ACT);
    HAL_DMA_IRQHandler(hal->hdmatx);
    QXK_ISR_EXIT();
}

// UART1 RX DMA
// Must be declared as extern "C" in header.
extern "C" void DMA2_Channel7_IRQHandler(void) {
    QXK_ISR_ENTRY();
    UART_HandleTypeDef *hal = UartAct::GetHal(UART1_ACT);
    HAL_DMA_IRQHandler(hal->hdmarx);
    QXK_ISR_EXIT();
}

// UART1 RX
// Must be declared as extern "C" in header.
extern "C" void USART1_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HandleUartIrq(UART1_ACT);
    QXK_ISR_EXIT();
}

// GPIO IN
// Must be declared as extern "C" in header.
extern "C" void EXTI15_10_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    QXK_ISR_EXIT();
}
extern "C" void EXTI9_5_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    QXK_ISR_EXIT();
}
extern "C" void EXTI4_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
    QXK_ISR_EXIT();
}
extern "C" void EXTI3_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
    QXK_ISR_EXIT();
}
extern "C" void EXTI2_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
    QXK_ISR_EXIT();
}
extern "C" void EXTI1_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
    QXK_ISR_EXIT();
}
extern "C" void EXTI0_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
    QXK_ISR_EXIT();
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_1) {
        Wifi::SignalCmdDataRdySem();
    } else {
        GpioIn::GpioIntCallback(pin);
    }
}

// Sensors.
extern "C" void I2C2_EV_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_I2C_EV_IRQHandler(Sensor::GetHal());
    QXK_ISR_EXIT();
}

extern "C" void I2C2_ER_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_I2C_ER_IRQHandler(Sensor::GetHal());
    QXK_ISR_EXIT();
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hal) {
    if (hal == Sensor::GetHal()) {
        Sensor::SignalI2cSem();
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hal) {
    if (hal == Sensor::GetHal()) {
        Sensor::SignalI2cSem();
    }
}

// ILI9341 LCD.
// Must be declared as extern "C" in header.
extern "C" void SPI1_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_SPI_IRQHandler(Ili9341::GetHal());
    QXK_ISR_EXIT();
}

// SPI1 TX DMA
// Must be declared as extern "C" in header.
extern "C" void DMA2_Channel4_IRQHandler(void) {
    QXK_ISR_ENTRY();
    SPI_HandleTypeDef *hal = Ili9341::GetHal();
    HAL_DMA_IRQHandler(hal->hdmatx);
    QXK_ISR_EXIT();
}

// SPI1 RX DMA
// Must be declared as extern "C" in header.
extern "C" void DMA2_Channel3_IRQHandler(void) {
    QXK_ISR_ENTRY();
    SPI_HandleTypeDef *hal = Ili9341::GetHal();
    HAL_DMA_IRQHandler(hal->hdmarx);
    QXK_ISR_EXIT();
}

// ES_WIFI module.
// Must be declared as extern "C" in header.
extern "C" void SPI3_IRQHandler(void)
{
    QXK_ISR_ENTRY();
    HAL_SPI_IRQHandler(Wifi::GetHal());
    QXK_ISR_EXIT();
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hal) {
    if (hal == Ili9341::GetHal()) {
        Ili9341::SignalSpiSem();
    } else if (hal == Wifi::GetHal()) {
        Wifi::SignalSpiSem();
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hal) {
    if (hal == Ili9341::GetHal()) {
        Ili9341::SignalSpiSem();
    } else if (hal == Wifi::GetHal()) {
        Wifi::SignalSpiSem();
    }
}
/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
