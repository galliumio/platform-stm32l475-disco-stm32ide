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

#include "fw_log.h"
#include "fw_assert.h"
#include "app_hsmn.h"
#include "periph.h"
#include "System.h"
#include "SystemInterface.h"
#include "GpioInInterface.h"
#include "CompositeActInterface.h"
#include "SimpleActInterface.h"
#include "DemoInterface.h"
#include "GpioOutInterface.h"
#include "AOWashingMachineInterface.h"
#include "TrafficInterface.h"
#include "LevelMeterInterface.h"
#include "NodeInterface.h"
#include "SensorInterface.h"
#include "bsp.h"
#include <vector>
#include <memory>

// Compile options to enable demo application.
// Only one of the following can be enabled at a time.
//#define ENABLE_TRAFFIC
#define ENABLE_LEVEL_METER

#if (defined(ENABLE_TRAFFIC) && defined(ENABLE_LEVEL_METER))
#error ENABLE_TRAFFIC and ENABLE_LEVEL_METER cannot be both defined
#endif

FW_DEFINE_THIS_FILE("System.cpp")

#define SRV_DOMAIN      "192.168.1.81"  	// "192.168.1.114"		//"192.168.1.233"
#define SRV_PORT        60002

using namespace FW;

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "SYSTEM_TIMER_EVT_START",
    SYSTEM_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "SYSTEM_INTERNAL_EVT_START",
    SYSTEM_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "SYSTEM_INTERFACE_EVT_START",
    SYSTEM_INTERFACE_EVT
};

System::System() :
    Active((QStateHandler)&System::InitialPseudoState, SYSTEM, "SYSTEM"), m_maxIdleCnt(0), m_cpuUtilPercent(0), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(GetHsmn(), STATE_TIMER), m_idleCntTimer(GetHsmn(), IDLE_CNT_TIMER),
    m_sensorDelayTimer(GetHsmn(), SENSOR_DELAY_TIMER),
    m_testTimer(GetHsmn(), TEST_TIMER) {
    SET_EVT_NAME(SYSTEM);
}

QState System::InitialPseudoState(System * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&System::Root);
}

QState System::Root(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            Periph::SetupNormal();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&System::Stopped);
        }
        case SYSTEM_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SystemStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        // Test only.
        case SYSTEM_RESTART_REQ:
        case SYSTEM_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&System::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState System::Stopped(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case SYSTEM_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SystemStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        // Test only.
        case SYSTEM_RESTART_REQ:
        case SYSTEM_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&System::Starting);
        }
    }
    return Q_SUPER(&System::Root);
}

QState System::Starting(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SystemStartReq::TIMEOUT_MS;
            // @todo Add assert to check timeout is larger than any of the max timeout of the controlled HSMs.
            me->m_stateTimer.Start(timeout);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&System::Prestarting);
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new SystemStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()),
                            me->m_inEvt);
            } else {
                me->SendCfm(new SystemStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&System::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new SystemStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&System::Started);
        }
    }
    return Q_SUPER(&System::Root);
}

QState System::Prestarting(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // Reset idle count in bsp.cpp
            GetIdleCnt();
            me->m_idleCntTimer.Start(IDLE_CNT_INIT_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_idleCntTimer.Stop();
            return Q_HANDLED();
        }
        case IDLE_CNT_TIMER: {
            EVENT(e);
            me->m_maxIdleCnt = GetIdleCnt() * (IDLE_CNT_POLL_TIMEOUT_MS / IDLE_CNT_INIT_TIMEOUT_MS);
            LOG("maxIdleCnt = %d", me->m_maxIdleCnt);
            return Q_TRAN(&System::Starting1);
        }
    }
    return Q_SUPER(&System::Starting);
}


QState System::Starting1(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReq(new CompositeActStartReq(), COMPOSITE_ACT, true);
            me->SendReq(new SimpleActStartReq(), SIMPLE_ACT, false);
            me->SendReq(new DemoStartReq(), DEMO, false);
            me->SendReq(new WashStartReq(), AO_WASHING_MACHINE, false);
#ifdef ENABLE_TRAFFIC
            me->SendReq(new TrafficStartReq(), TRAFFIC, false);
#endif
            me->SendReq(new GpioInStartReq(), USER_BTN, false);
            me->SendReq(new GpioOutStartReq(), USER_LED, false);
            me->SendReq(new NodeStartReq(SRV_DOMAIN, SRV_PORT), NODE, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case COMPOSITE_ACT_START_CFM:
        case SIMPLE_ACT_START_CFM:
        case DEMO_START_CFM:
        case WASH_START_CFM:
        case TRAFFIC_START_CFM:
        case GPIO_IN_START_CFM:
        case GPIO_OUT_START_CFM:
        case NODE_START_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            ERROR_EVENT(cfm);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(NEXT));
            }
            return Q_HANDLED();
        }
        case NEXT: {
            EVENT(e);
            return Q_TRAN(&System::Starting2);
        }
    }
    return Q_SUPER(&System::Starting);
}

QState System::Starting2(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReq(new SensorStartReq(), SENSOR, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case SENSOR_START_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(NEXT));
            }
            return Q_HANDLED();
        }
        case NEXT: {
            EVENT(e);
            return Q_TRAN(&System::Starting3);
        }
    }
    return Q_SUPER(&System::Starting);
}

QState System::Starting3(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
#ifdef ENABLE_LEVEL_METER
            me->SendReq(new LevelMeterStartReq(), LEVEL_METER, true);
            return Q_HANDLED();
#else
            me->Send(new Evt(DONE), me->GetHsmn());
            return Q_HANDLED();
#endif
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LEVEL_METER_START_CFM: {
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
    }
    return Q_SUPER(&System::Starting);
}

QState System::Stopping(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SystemStopReq::TIMEOUT_MS;
            // @todo Add assert to check timeout is larger than any of the max timeout of the controlled HSMs.
            me->m_stateTimer.Start(timeout);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&System::Stopping1);
        }
        // Test only.
        case SYSTEM_RESTART_REQ:
        case SYSTEM_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
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
            return Q_TRAN(&System::Stopped);
        }
    }
    return Q_SUPER(&System::Root);
}

QState System::Stopping1(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReq(new LevelMeterStopReq(), LEVEL_METER, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LEVEL_METER_STOP_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(NEXT));
            }
            return Q_HANDLED();
        }
        case NEXT: {
            EVENT(e);
            return Q_TRAN(&System::Stopping2);
        }
    }
    return Q_SUPER(&System::Stopping);
}

QState System::Stopping2(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->SendReq(new CompositeActStopReq(), COMPOSITE_ACT, true);
            me->SendReq(new SimpleActStopReq(), SIMPLE_ACT, false);
            me->SendReq(new DemoStopReq(), DEMO, false);
            me->SendReq(new WashStopReq(), AO_WASHING_MACHINE, false);
            me->SendReq(new TrafficStopReq(), TRAFFIC, false);
            me->SendReq(new GpioInStopReq(), USER_BTN, false);
            me->SendReq(new GpioOutStopReq(), USER_LED, false);
            me->SendReq(new NodeStopReq(), NODE, false);
            me->SendReq(new SensorStopReq(), SENSOR, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case COMPOSITE_ACT_STOP_CFM:
        case SIMPLE_ACT_STOP_CFM:
        case DEMO_STOP_CFM:
        case WASH_STOP_CFM:
        case TRAFFIC_STOP_CFM:
        case GPIO_IN_STOP_CFM:
        case GPIO_OUT_STOP_CFM:
        case NODE_STOP_CFM:
        case SENSOR_STOP_CFM: {
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
    }
    return Q_SUPER(&System::Stopping);
}

QState System::Started(System * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_idleCntTimer.Stop();
            return Q_HANDLED();
        }
        case IDLE_CNT_TIMER: {
            uint32_t idleCnt = GetIdleCnt();
            idleCnt = LESS(idleCnt, me->m_maxIdleCnt);
            me->m_cpuUtilPercent = 100 - (idleCnt*100 / me->m_maxIdleCnt);
            PRINT("Utilization = %d\n\r", me->m_cpuUtilPercent);
            return Q_HANDLED();
        }
        case SYSTEM_CPU_UTIL_REQ: {
            SystemCpuUtilReq const &req = static_cast<SystemCpuUtilReq const &>(*e);
            if (req.GetEnable()) {
                LOG("CPU util enabled");
                // Reset idle count in bsp.cpp
                GetIdleCnt();
                me->m_idleCntTimer.Restart(IDLE_CNT_POLL_TIMEOUT_MS, Timer::PERIODIC);
            } else {
                LOG("CPU util disabled");
                me->m_idleCntTimer.Stop();
            }
            return Q_HANDLED();
        }
        // Hooks up USER_BTN to Traffic for testing. 
        case GPIO_IN_PULSE_IND: {
            EVENT(e);
            LOG("Car arriving in NS direction");
            me->Send(new TrafficCarNSReq(), TRAFFIC);
            return Q_HANDLED();
        }
        case GPIO_IN_HOLD_IND: {
            EVENT(e);
            LOG("Car arriving in EW direction");
            me->Send(new TrafficCarEWReq(), TRAFFIC);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&System::Root);
}

/*
QState System::MyState(System * const me, QEvt const * const e) {
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
            return Q_TRAN(&System::SubState);
        }
    }
    return Q_SUPER(&System::SuperState);
}
*/

} // namespace APP
