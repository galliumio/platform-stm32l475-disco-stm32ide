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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "fw_active.h"
#include "GpioOutInterface.h"
#include "GpioOut.h"
#include "GpioPattern.h"
#include "periph.h"

FW_DEFINE_THIS_FILE("GpioOut.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "GPIO_OUT_TIMER_EVT_START",
    GPIO_OUT_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "GPIO_OUT_INTERNAL_EVT_START",
    GPIO_OUT_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "GPIO_OUT_INTERFACE_EVT_START",
    GPIO_OUT_INTERFACE_EVT
};

// The order below must match that in app_hsmn.h.
static char const * const hsmName[] = {
    "USER_LED",
    "TEST_LED",
    // Add more regions here.
};

static Hsmn &GetCurrHsmn() {
    static Hsmn hsmn = GPIO_OUT;
    FW_ASSERT(hsmn <= GPIO_OUT_LAST);
    return hsmn;
}

static char const * GetCurrName() {
    uint16_t inst = GetCurrHsmn() - GPIO_OUT;
    FW_ASSERT(inst < ARRAY_COUNT(hsmName));
    return hsmName[inst];
}

static void IncCurrHsmn() {
    Hsmn &currHsmn = GetCurrHsmn();
    ++currHsmn;
    FW_ASSERT(currHsmn > 0);
}

// Define GPIO output pin configurations.
// Set pwmTimer to NULL if PWM is not supported for an LED (no brightness control).
// If pwmTimer is NULL, af and pwmChannel are don't-care, and mode must be OUTPUT_PP or OUTPUT_OD.
GpioOut::Config const GpioOut::CONFIG[] = {
    { USER_LED, GPIOB, GPIO_PIN_14, true, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF1_TIM1, TIM1, TIM_CHANNEL_2, true, TEST_GPIO_PATTERN_SET },
    // Add more LED here.
};

// Corresponds to what was done in _msp.cpp file.
void GpioOut::InitGpio() {
    // Clock has been initialized by System via Periph.
    GPIO_InitTypeDef gpioInit;
    memset(&gpioInit, 0, sizeof(gpioInit));
    gpioInit.Pin = m_config->pin;
    gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpioInit.Mode = m_config->mode;
    gpioInit.Pull = m_config->pull;
    gpioInit.Alternate = m_config->af;
    if (m_config->pwmTimer) {
        FW_ASSERT((m_config->mode == GPIO_MODE_AF_PP) || (m_config->mode == GPIO_MODE_AF_OD));
    }
    else {
        FW_ASSERT((m_config->mode == GPIO_MODE_OUTPUT_PP) || (m_config->mode == GPIO_MODE_OUTPUT_OD));
    }
    HAL_GPIO_Init(m_config->port, &gpioInit);
}

void GpioOut::DeInitGpio() {
    if (m_config->pwmTimer) {
        StopPwm(Periph::GetHal(m_config->pwmTimer));
    }
    HAL_GPIO_DeInit(m_config->port, m_config->pin);
}

void GpioOut::ConfigPwm(uint32_t levelPermil) {
    // If PWM is not supported, turn off GPIO if level = 0; turn on GPIO if level > 0.
    if (m_config->pwmTimer == NULL) {
        if (levelPermil == 0) {
            HAL_GPIO_WritePin(m_config->port, m_config->pin, m_config->activeHigh ? GPIO_PIN_RESET : GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(m_config->port, m_config->pin, m_config->activeHigh ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
        return;
    }
    // Here PWM is supported.
    FW_ASSERT(levelPermil <= 1000);
    if (!m_config->activeHigh) {
        levelPermil = 1000 - levelPermil;
    }
    // Base PWM timer has been initialized by System via Periph.
    TIM_HandleTypeDef *hal = Periph::GetHal(m_config->pwmTimer);
    StopPwm(hal);
    TIM_OC_InitTypeDef timConfig;
    memset(&timConfig, 0, sizeof(timConfig));
    timConfig.OCMode       = TIM_OCMODE_PWM1;
    timConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
    timConfig.OCFastMode   = TIM_OCFAST_DISABLE;
    timConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    timConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    timConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
    timConfig.Pulse        = (hal->Init.Period + 1) * levelPermil / 1000;
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    HAL_StatusTypeDef status = HAL_TIM_PWM_ConfigChannel(hal, &timConfig, m_config->pwmChannel);
    FW_ASSERT(status== HAL_OK);
    StartPwm(hal);
    QF_CRIT_EXIT(crit);
}

void GpioOut::StartPwm(TIM_HandleTypeDef *hal) {
    FW_ASSERT(hal);
    HAL_StatusTypeDef status;
    if (m_config->pwmComplementary) {
        status = HAL_TIMEx_PWMN_Start(hal, m_config->pwmChannel);
    } else {
        status = HAL_TIM_PWM_Start(hal, m_config->pwmChannel);
    }
    FW_ASSERT(status == HAL_OK);
}


void GpioOut::StopPwm(TIM_HandleTypeDef *hal) {
    FW_ASSERT(hal);
    HAL_StatusTypeDef status;
    if (m_config->pwmComplementary) {
        status = HAL_TIMEx_PWMN_Stop(hal, m_config->pwmChannel);
    } else {
        status = HAL_TIM_PWM_Stop(hal, m_config->pwmChannel);
    }
    FW_ASSERT(status == HAL_OK);
}

GpioOut::GpioOut() :
    FW::Region((QStateHandler)&GpioOut::InitialPseudoState, GetCurrHsmn(), GetCurrName()),
    m_config(NULL), m_currPattern(NULL), m_intervalIndex(0), m_isRepeat(false),
    m_intervalTimer(GetHsmn(), INTERVAL_TIMER) {
    SET_EVT_NAME(GPIO_OUT);
    uint32_t i;
    for (i = 0; i < ARRAY_COUNT(CONFIG); i++) {
        if (CONFIG[i].hsmn == GetHsmn()) {
            m_config = &CONFIG[i];
            break;
        }
    }
    FW_ASSERT(i < ARRAY_COUNT(CONFIG));
    IncCurrHsmn();
}

QState GpioOut::InitialPseudoState(GpioOut * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&GpioOut::Root);
}

QState GpioOut::Root(GpioOut * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&GpioOut::Stopped);
        }
        case GPIO_OUT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new GpioOutStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState GpioOut::Stopped(GpioOut * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case GPIO_OUT_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new GpioOutStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case GPIO_OUT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new GpioOutStartCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&GpioOut::Started);
        }
    }
    return Q_SUPER(&GpioOut::Root);
}

QState GpioOut::Started(GpioOut * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->InitGpio();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->DeInitGpio();
            return Q_HANDLED();
        }
        case GPIO_OUT_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new GpioOutStopCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&GpioOut::Stopped);
        }
    }
    return Q_SUPER(&GpioOut::Root);
}

/*
QState GpioOut::MyState(GpioOut * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&GpioOut::SubState);
        }
    }
    return Q_SUPER(&GpioOut::SuperState);
}
*/

} // namespace APP
