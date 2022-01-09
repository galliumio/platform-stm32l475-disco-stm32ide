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
#include "GpioInInterface.h"
#include "SensorMagInterface.h"
#include "SensorMag.h"
#include "stm32l475e_iot01_magneto.h"

FW_DEFINE_THIS_FILE("SensorMag.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "SENSOR_MAG_TIMER_EVT_START",
    SENSOR_MAG_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "SENSOR_MAG_INTERNAL_EVT_START",
    SENSOR_MAG_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "SENSOR_MAG_INTERFACE_EVT_START",
    SENSOR_MAG_INTERFACE_EVT
};

SensorMag::SensorMag(Hsmn drdyHsmn, I2C_HandleTypeDef &hal) :
    Region((QStateHandler)&SensorMag::InitialPseudoState, SENSOR_MAG, "SENSOR_MAG"),
    m_drdyHsmn(drdyHsmn), m_hal(hal), m_stateTimer(GetHsmn(), STATE_TIMER), m_handle(NULL), m_inEvt(QEvt::STATIC_EVT) {
    SET_EVT_NAME(SENSOR_MAG);
}

QState SensorMag::InitialPseudoState(SensorMag * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&SensorMag::Root);
}

QState SensorMag::Root(SensorMag * const me, QEvt const * const e) {
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
            return Q_TRAN(&SensorMag::Stopped);
        }
        case SENSOR_MAG_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SensorMagStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case SENSOR_MAG_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&SensorMag::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState SensorMag::Stopped(SensorMag * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case SENSOR_MAG_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SensorMagStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case SENSOR_MAG_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&SensorMag::Starting);
        }
    }
    return Q_SUPER(&SensorMag::Root);
}

QState SensorMag::Starting(SensorMag * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SensorMagStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > GpioInStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            // Disable debouncing to ensure we get the active indication even if the GpioIn region misses the deactive trigger.
            // If debouncing is enabled, the GpioIn region won't send the active indication if it hasn't detected the deactive
            // pin level. It will cause this region to stall (deadlock)

            // @todo Disabes GPIO for now until INT is enabled. For test, sends Done immediately.
            //me->SendReq(new GpioInStartReq(false), me->m_drdyHsmn, true);
            me->Raise(new Evt(DONE));

            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case GPIO_IN_START_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new SensorMagStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new SensorMagStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&SensorMag::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new SensorMagStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&SensorMag::Started);
        }
    }
    return Q_SUPER(&SensorMag::Root);
}

QState SensorMag::Stopping(SensorMag * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SensorMagStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > GpioInStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new GpioInStopReq(), me->m_drdyHsmn, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case SENSOR_MAG_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case GPIO_IN_STOP_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&SensorMag::Stopped);
        }
    }
    return Q_SUPER(&SensorMag::Root);
}

QState SensorMag::Started(SensorMag * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        // Test only.
        case GPIO_IN_ACTIVE_IND:
        case GPIO_IN_INACTIVE_IND: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&SensorMag::Root);
}

/*
QState SensorMag::MyState(SensorMag * const me, QEvt const * const e) {
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
            return Q_TRAN(&SensorMag::SubState);
        }
    }
    return Q_SUPER(&SensorMag::SuperState);
}
*/

} // namespace APP
