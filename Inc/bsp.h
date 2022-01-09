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

#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include "qpcpp.h"
#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"
#include "stm32l475e_iot01.h"
#include "stm32l4xx_it.h"

#define BSP_TICKS_PER_SEC            (1000)
#define BSP_MSEC_PER_TICK            (1000 / BSP_TICKS_PER_SEC)
#define BSP_MSEC_TO_TICK(ms_)        ((ms_) / BSP_MSEC_PER_TICK)

// Timer tick rates
#define TICK_RATE_BSP       0       // Timer tick driven by SysTick_Handler() at a rate of BSP_TICKS_PER_SEC.

enum KernelUnawareISRs { // see NOTE00
    // ...
    MAX_KERNEL_UNAWARE_CMSIS_PRI  // keep always last
};
// "kernel-unaware" interrupts can't overlap "kernel-aware" interrupts
Q_ASSERT_COMPILE(MAX_KERNEL_UNAWARE_CMSIS_PRI <= (int)QF_AWARE_ISR_CMSIS_PRI);

// Lower numerical value indicates higher priority.
enum KernelAwareISRs {
    SYSTICK_PRIO            = QF_AWARE_ISR_CMSIS_PRI,
    DMA1_CHANNEL4_PRIO      = QF_AWARE_ISR_CMSIS_PRI + 1,   // I2C2/USART1 TX DMA
    DMA1_CHANNEL5_PRIO      = QF_AWARE_ISR_CMSIS_PRI + 1,   // I2C2/USART1 RX DMA
    USART1_IRQ_PRIO         = QF_AWARE_ISR_CMSIS_PRI + 1,   // USART1 IRQ
    I2C1_EV_PRIO            = QF_AWARE_ISR_CMSIS_PRI + 1,   // I2C1 Event IRQ
    I2C1_ER_PRIO            = QF_AWARE_ISR_CMSIS_PRI + 1,   // I2C1 Error IRQ
    I2C2_EV_PRIO            = QF_AWARE_ISR_CMSIS_PRI + 1,   // I2C2 Event IRQ (Sensor)
    I2C2_ER_PRIO            = QF_AWARE_ISR_CMSIS_PRI + 1,   // I2C2 Error IRQ (Sensor)
    SPI1_PRIO               = QF_AWARE_ISR_CMSIS_PRI + 1,   // SPI1 IRQ
    SPI3_PRIO               = QF_AWARE_ISR_CMSIS_PRI + 1,   // SPI3 IRQ
    DMA2_CHANNEL4_PRIO      = QF_AWARE_ISR_CMSIS_PRI + 1,   // SPI1 TX DMA
    DMA2_CHANNEL3_PRIO      = QF_AWARE_ISR_CMSIS_PRI + 1,   // SPI1 RX DMA
    DMA2_CHANNEL6_PRIO      = QF_AWARE_ISR_CMSIS_PRI + 1,   // USART1 TX DMA
    DMA2_CHANNEL7_PRIO      = QF_AWARE_ISR_CMSIS_PRI + 1,   // USART1 RX DMA
    EXTI0_PRIO              = QF_AWARE_ISR_CMSIS_PRI + 10,
    EXTI1_PRIO              = QF_AWARE_ISR_CMSIS_PRI + 10,  // WIFI CMD DATA READY
    EXTI2_PRIO              = QF_AWARE_ISR_CMSIS_PRI + 10,
    EXTI3_PRIO              = QF_AWARE_ISR_CMSIS_PRI + 10,
    EXTI4_PRIO              = QF_AWARE_ISR_CMSIS_PRI + 10,
    EXTI9_5_PRIO            = QF_AWARE_ISR_CMSIS_PRI + 10,
    EXTI15_10_PRIO          = QF_AWARE_ISR_CMSIS_PRI + 10,
    // ...
    MAX_KERNEL_AWARE_CMSIS_PRI // keep always last
};
// "kernel-aware" interrupts should not overlap the PendSV priority
Q_ASSERT_COMPILE(MAX_KERNEL_AWARE_CMSIS_PRI <= (0xFF >>(8-__NVIC_PRIO_BITS)));

void BspInit();
void BspWrite(char const *buf, uint32_t len);
// Use C linkage as it is required by C-based emwin library.
extern "C" uint32_t GetSystemMs();
extern "C" void DelayMs(uint32_t ms);
uint32_t GetIdleCnt();

#endif // BSP_H
