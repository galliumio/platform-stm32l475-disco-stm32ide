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

#include <string.h>
#include "fw_log.h"
#include "fw_assert.h"
#include "Console.h"
#include "GpioOutInterface.h"
#include "GpioOutCmd.h"

// Assignment 2 - Provided helper includes.
#include "GpioPattern.h"
#include "periph.h"
#include "bsp.h"

FW_DEFINE_THIS_FILE("GpioOutCmd.cpp")

namespace APP {

// Assignment 2 - Provided helper functions.
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    bool activeHigh;
    uint32_t mode;
    uint32_t pull;
    uint32_t af;
    TIM_TypeDef *pwmTimer;
    uint32_t pwmChannel;
    bool pwmComplementary;
    GpioPatternSet const &patternSet;
} LedConfig;

// Define LED configurations.
// Set pwmTimer to NULL if PWM is not supported for an LED (no brightness control).
// If pwmTimer is NULL, af and pwmChannel are don't-care, and mode must be OUTPUT_PP or OUTPUT_OD.
static LedConfig const LED_CONFIG = {
    GPIOB, GPIO_PIN_14, true, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF1_TIM1, TIM1, TIM_CHANNEL_2, true, TEST_GPIO_PATTERN_SET
};

// API to control LED.
static void Delay(uint32_t ms);
static void InitGpio();
static void DeInitGpio();
static void ConfigPwm(uint32_t levelPermil);

// Helper functions not to be called directly.
static void StartPwm(TIM_HandleTypeDef *hal);
static void StopPwm(TIM_HandleTypeDef *hal);

// Delay should be avoided. It is only used here for assignment.
void Delay(uint32_t ms) {
    uint32_t startMs = GetSystemMs();
    while(1) {
        if ((GetSystemMs() - startMs) >= ms) {
            break;
        }
    }
}

// Corresponds to what was done in _msp.cpp file.
static void InitGpio() {
    LedConfig const *config = &LED_CONFIG;
    // Clock has been initialized by System via Periph.
    GPIO_InitTypeDef gpioInit;
    gpioInit.Pin = config->pin;
    gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpioInit.Mode = config->mode;
    gpioInit.Pull = config->pull;
    gpioInit.Alternate = config->af;
    if (config->pwmTimer) {
        FW_ASSERT((config->mode == GPIO_MODE_AF_PP) || (config->mode == GPIO_MODE_AF_OD));
    }
    else {
        FW_ASSERT((config->mode == GPIO_MODE_OUTPUT_PP) || (config->mode == GPIO_MODE_OUTPUT_OD));
    }
    HAL_GPIO_Init(config->port, &gpioInit);
}

static void DeInitGpio() {
    LedConfig const *config = &LED_CONFIG;
    if (config->pwmTimer) {
        StopPwm(Periph::GetHal(config->pwmTimer));
    }
    HAL_GPIO_DeInit(config->port, config->pin);
}

static void ConfigPwm(uint32_t levelPermil) {
    LedConfig const *config = &LED_CONFIG;
    // If PWM is not supported, turn off GPIO if level = 0; turn on GPIO if level > 0.
    if (config->pwmTimer == NULL) {
        if (levelPermil == 0) {
            HAL_GPIO_WritePin(config->port, config->pin, config->activeHigh ? GPIO_PIN_RESET : GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(config->port, config->pin, config->activeHigh ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
        return;
    }
    // Here PWM is supported.
    FW_ASSERT(levelPermil <= 1000);
    if (!config->activeHigh) {
        levelPermil = 1000 - levelPermil;
    }
    // Base PWM timer has been initialized by System via Periph.
    TIM_HandleTypeDef *hal = Periph::GetHal(config->pwmTimer);
    StopPwm(hal);
    TIM_OC_InitTypeDef timConfig;
    timConfig.OCMode       = TIM_OCMODE_PWM1;
    timConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
    timConfig.OCFastMode   = TIM_OCFAST_DISABLE;
    timConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    timConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    timConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
    timConfig.Pulse        = (hal->Init.Period + 1) * levelPermil / 1000;
    HAL_StatusTypeDef status = HAL_TIM_PWM_ConfigChannel(hal, &timConfig, config->pwmChannel);
    FW_ASSERT(status== HAL_OK);
    StartPwm(hal);
}

static void StartPwm(TIM_HandleTypeDef *hal) {
    FW_ASSERT(hal);
    LedConfig const *config = &LED_CONFIG;
    HAL_StatusTypeDef status;
    if (config->pwmComplementary) {
        status = HAL_TIMEx_PWMN_Start(hal, config->pwmChannel);
    } else {
        status = HAL_TIM_PWM_Start(hal, config->pwmChannel);
    }
    FW_ASSERT(status == HAL_OK);
}

static void StopPwm(TIM_HandleTypeDef *hal) {
    FW_ASSERT(hal);
    LedConfig const *config = &LED_CONFIG;
    HAL_StatusTypeDef status;
    if (config->pwmComplementary) {
        status = HAL_TIMEx_PWMN_Stop(hal, config->pwmChannel);
    } else {
        status = HAL_TIM_PWM_Stop(hal, config->pwmChannel);
    }
    FW_ASSERT(status == HAL_OK);
}

// Command handlers
static CmdStatus Test(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("GpioOutCmd Test\n\r");
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus On(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            Console::ConsoleCmd const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if (ind.Argc() >= 2) {
                uint32_t pattern = STRING_TO_NUM(ind.Argv(1), 0);
                bool repeat = true;
                if (ind.Argc() >= 3 && STRING_EQUAL(ind.Argv(2), "0")) {
                    repeat = false;
                }
                console.Print("pattern = %d, repeat = %d\n\r", pattern, repeat);
                // Assignment 2 - Implement the command to display the indexed pattern. If repeat is "0", it is shown once;
                //           otherwise it is shown 5 times. Handle the case when the index is out of range.
                //           As a reminder, the set of LED patterns is defined in the structure TEST_LED_PATTERN_SET.
                // Sample code to show how to use the API to control LED brightness and add delay. Please remove or comment
                // it when adding your own code.
                // Being sample code.
                InitGpio();
                ConfigPwm(1000);
                Delay(200);
                ConfigPwm(0);
                Delay(200);
                ConfigPwm(200);
                Delay(200);
                ConfigPwm(0);
                // End sample code.
                return CMD_DONE;
            }
            console.Print("led on <pattern idx> [0=once,*other=repeat]\n\r");
            return CMD_DONE;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus Off(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
        	(void)console;
        	ConfigPwm(0);
        	DeInitGpio();
            return CMD_DONE;
        }
    }
    return CMD_CONTINUE;
}


static CmdStatus List(Console &console, Evt const *e);
static CmdHandler const cmdHandler[] = {
    { "test",       Test,       "Test function", 0 },
    { "on",         On,         "Start a pattern", 0 },
    { "off",        Off,        "Stop a pattern", 0 },
    { "?",          List,       "List commands", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus GpioOutCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
