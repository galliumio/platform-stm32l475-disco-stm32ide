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
#include "SensorHumidTempInterface.h"
#include "SensorHumidTemp.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"

FW_DEFINE_THIS_FILE("SensorHumidTemp.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "SENSOR_HUMID_TEMP_TIMER_EVT_START",
    SENSOR_HUMID_TEMP_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "SENSOR_HUMID_TEMP_INTERNAL_EVT_START",
    SENSOR_HUMID_TEMP_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "SENSOR_HUMID_TEMP_INTERFACE_EVT_START",
    SENSOR_HUMID_TEMP_INTERFACE_EVT
};

SensorHumidTemp::SensorHumidTemp(Hsmn drdyHsmn, I2C_HandleTypeDef &hal) :
    Region((QStateHandler)&SensorHumidTemp::InitialPseudoState, SENSOR_HUMID_TEMP, "SENSOR_HUMID_TEMP"),
    m_drdyHsmn(drdyHsmn), m_pipe(NULL), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(GetHsmn(), STATE_TIMER), m_pollTimer(GetHsmn(), POLL_TIMER) {
    SET_EVT_NAME(SENSOR_HUMID_TEMP);
}

QState SensorHumidTemp::InitialPseudoState(SensorHumidTemp * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&SensorHumidTemp::Root);
}

QState SensorHumidTemp::Root(SensorHumidTemp * const me, QEvt const * const e) {
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
            return Q_TRAN(&SensorHumidTemp::Stopped);
        }
        case SENSOR_HUMID_TEMP_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SensorHumidTempStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case SENSOR_HUMID_TEMP_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&SensorHumidTemp::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState SensorHumidTemp::Stopped(SensorHumidTemp * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case SENSOR_HUMID_TEMP_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SensorHumidTempStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case SENSOR_HUMID_TEMP_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&SensorHumidTemp::Starting);
        }
    }
    return Q_SUPER(&SensorHumidTemp::Root);
}

QState SensorHumidTemp::Starting(SensorHumidTemp * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SensorHumidTempStartReq::TIMEOUT_MS;
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
                me->SendCfm(new SensorHumidTempStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new SensorHumidTempStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&SensorHumidTemp::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new SensorHumidTempStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&SensorHumidTemp::Started);
        }
    }
    return Q_SUPER(&SensorHumidTemp::Root);
}

QState SensorHumidTemp::Stopping(SensorHumidTemp * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SensorHumidTempStopReq::TIMEOUT_MS;
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
        case SENSOR_HUMID_TEMP_STOP_REQ: {
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
            return Q_TRAN(&SensorHumidTemp::Stopped);
        }
    }
    return Q_SUPER(&SensorHumidTemp::Root);
}

QState SensorHumidTemp::Started(SensorHumidTemp * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            HSENSOR_Status_TypDef hStatus = static_cast<HSENSOR_Status_TypDef>(BSP_HSENSOR_Init());
            FW_ASSERT(hStatus == HSENSOR_OK);
            TSENSOR_Status_TypDef tStatus = static_cast<TSENSOR_Status_TypDef>(BSP_TSENSOR_Init());
            FW_ASSERT(tStatus == TSENSOR_OK);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            // @todo Missing API in BSP to de-initialize humidity and temperature sensor.
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&SensorHumidTemp::Off);
        }
    }
    return Q_SUPER(&SensorHumidTemp::Root);
}

QState SensorHumidTemp::Off(SensorHumidTemp * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case SENSOR_HUMID_TEMP_ON_REQ: {
            EVENT(e);
            SensorHumidTempOnReq const &req = static_cast<SensorHumidTempOnReq const &>(*e);
            if (!req.GetPipe()) {
                me->SendCfm(new SensorHumidTempOnCfm(ERROR_PARAM), req);
            } else {
                me->m_pipe = req.GetPipe();
                me->SendCfm(new SensorHumidTempOnCfm(ERROR_SUCCESS), req);
                me->Raise(new Evt(TURNED_ON));
            }
            return Q_HANDLED();
        }
        case TURNED_ON: {
             EVENT(e);
             return Q_TRAN(&SensorHumidTemp::On);
         }
    }
    return Q_SUPER(&SensorHumidTemp::Started);
}

QState SensorHumidTemp::On(SensorHumidTemp * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_pollTimer.Start(POLL_TIMEOUT_MS, Timer::PERIODIC);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_pipe = NULL;
            me->m_pollTimer.Stop();
            return Q_HANDLED();
        }
        case POLL_TIMER: {
            FW_ASSERT(me->m_pipe);
            float humidity = BSP_HSENSOR_ReadHumidity();
            float temperature = BSP_TSENSOR_ReadTemp();
            LOG("Humidity = %f Temp = %f", humidity, temperature);
            HumidTempReport report(humidity, temperature);
            uint32_t count = me->m_pipe->Write(&report, 1);
            if (count != 1) {
                WARNING("Pipe full");
            }
            return Q_HANDLED();
        }
        case SENSOR_HUMID_TEMP_OFF_REQ: {
            EVENT(e);
            SensorHumidTempOffReq const &req = static_cast<SensorHumidTempOffReq const &>(*e);
            me->SendCfm(new SensorHumidTempOffCfm(ERROR_SUCCESS), req);
            me->Raise(new Evt(TURNED_OFF));
            return Q_HANDLED();
        }
        case TURNED_OFF: {
             EVENT(e);
             return Q_TRAN(&SensorHumidTemp::Off);
        }
    }
    return Q_SUPER(&SensorHumidTemp::Started);
}

/*
QState SensorHumidTemp::MyState(SensorHumidTemp * const me, QEvt const * const e) {
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
            return Q_TRAN(&SensorHumidTemp::SubState);
        }
    }
    return Q_SUPER(&SensorHumidTemp::SuperState);
}
*/

} // namespace APP
