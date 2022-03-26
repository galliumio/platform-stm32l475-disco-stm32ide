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

#ifndef HSM_NUM_H
#define HSM_NUM_H

#include "fw_def.h"

namespace APP {

#define APP_HSM \
    ADD_HSM(SYSTEM, 1) \
    ADD_HSM(CONSOLE, 1) \
    ADD_HSM(CMD_INPUT, 2) \
    ADD_HSM(CMD_PARSER, 2) \
    ADD_HSM(UART_ACT, 2) \
    ADD_HSM(UART_IN, 2) \
    ADD_HSM(UART_OUT, 2) \
    ADD_HSM(WIFI, 1) \
    ADD_HSM(NODE, 1) \
    ADD_HSM(NODE_PARSER, 1) \
    ADD_HSM(DISP, 2) \
    ADD_HSM(SENSOR, 1) \
    ADD_HSM(SENSOR_ACCEL_GYRO, 1) \
    ADD_HSM(SENSOR_HUMID_TEMP, 1) \
    ADD_HSM(SENSOR_MAG, 1) \
    ADD_HSM(SENSOR_PRESS, 1) \
    ADD_HSM(GPIO_IN_ACT, 1) \
    ADD_HSM(GPIO_IN, 5) \
    ADD_HSM(DEMO, 1) \
    ADD_HSM(GPIO_OUT_ACT, 1) \
    ADD_HSM(GPIO_OUT, 1) \
    ADD_HSM(AO_WASHING_MACHINE, 1) \
    ADD_HSM(TRAFFIC, 1) \
    ADD_HSM(LAMP, 2) \
    ADD_HSM(LEVEL_METER, 1) \
    ADD_HSM(SIMPLE_ACT, 1) \
    ADD_HSM(SIMPLE_REG, 1) \
    ADD_HSM(COMPOSITE_ACT, 1) \
    ADD_HSM(COMPOSITE_REG, 4)

#define ALIAS_HSM \
    ADD_ALIAS(CONSOLE_UART1,    CONSOLE) \
    ADD_ALIAS(CMD_INPUT_UART1,  CMD_INPUT) \
    ADD_ALIAS(CMD_PARSER_UART1, CMD_PARSER) \
    ADD_ALIAS(UART1_ACT, UART_ACT) \
    ADD_ALIAS(UART1_IN,  UART_IN) \
    ADD_ALIAS(UART1_OUT, UART_OUT) \
    ADD_ALIAS(ILI9341,   DISP) \
    ADD_ALIAS(USER_BTN,        GPIO_IN) \
    ADD_ALIAS(ACCEL_GYRO_INT,  GPIO_IN+1) \
    ADD_ALIAS(MAG_DRDY,        GPIO_IN+2) \
    ADD_ALIAS(HUMID_TEMP_DRDY, GPIO_IN+3) \
    ADD_ALIAS(PRESS_INT,       GPIO_IN+4) \
    ADD_ALIAS(USER_LED,        GPIO_OUT) \
    ADD_ALIAS(LAMP_NS, LAMP) \
    ADD_ALIAS(LAMP_EW, LAMP+1) \
    ADD_ALIAS(COMPOSITE_REG0, COMPOSITE_REG) \
    ADD_ALIAS(COMPOSITE_REG1, COMPOSITE_REG+1) \
    ADD_ALIAS(COMPOSITE_REG2, COMPOSITE_REG+2) \
    ADD_ALIAS(COMPOSITE_REG3, COMPOSITE_REG+3)

#undef ADD_HSM
#undef ADD_ALIAS
#define ADD_HSM(hsmn_, count_) hsmn_, hsmn_##_COUNT = count_, hsmn_##_LAST = hsmn_ + count_ - 1,
#define ADD_ALIAS(alias_, to_) alias_ = to_,

enum {
    APP_HSM_START = FW::HSM_UNDEF,
    APP_HSM
    HSM_COUNT,
    ALIAS_HSM
};

// Higher value corresponds to higher priority.
// The maximum priority is defined in qf_port.h as QF_MAX_ACTIVE (32)
enum
{
    PRIO_SENSOR         = 31,
    PRIO_UART1_ACT      = 29,
    PRIO_SYSTEM         = 26,
    PRIO_LEDPANEL       = 24,
    PRIO_ILI9341        = 22,
    PRIO_WIFI           = 20,
    PRIO_NODE           = 18,
    PRIO_GPIO_IN_ACT    = 16,
    PRIO_DEMO           = 10,
    PRIO_GPIO_OUT_ACT   = 9,
    PRIO_TEST_LED       = 8,
    PRIO_AO_WASHING_MACHINE = 7,
    PRIO_TRAFFIC        = 6,
    PRIO_LEVEL_METER    = 5,
    PRIO_SIMPLE_ACT     = 4,
    PRIO_COMPOSITE_ACT  = 3,
    PRIO_CONSOLE_UART1  = 1,
};

} // namespace APP

#endif // HSM_NUM_H
