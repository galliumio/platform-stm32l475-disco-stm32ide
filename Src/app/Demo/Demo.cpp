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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "DemoInterface.h"
#include "Demo.h"

FW_DEFINE_THIS_FILE("Demo.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "DEMO_TIMER_EVT_START",
    DEMO_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "DEMO_INTERNAL_EVT_START",
    DEMO_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "DEMO_INTERFACE_EVT_START",
    DEMO_INTERFACE_EVT
};

Demo::Demo() :
    Active((QStateHandler)&Demo::InitialPseudoState, DEMO, "DEMO"),
    m_stateTimer(GetHsmn(), STATE_TIMER), m_inEvt(QEvt::STATIC_EVT) {
    SET_EVT_NAME(DEMO);
}

QState Demo::InitialPseudoState(Demo * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Demo::Root);
}

QState Demo::Root(Demo * const me, QEvt const * const e) {
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
            return Q_TRAN(&Demo::Stopped);
        }
        case DEMO_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new DemoStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case DEMO_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Demo::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Demo::Stopped(Demo * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DEMO_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new DemoStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case DEMO_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&Demo::Starting);
        }
    }
    return Q_SUPER(&Demo::Root);
}

QState Demo::Starting(Demo * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = DemoStartReq::TIMEOUT_MS;
            me->m_stateTimer.Start(timeout);
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new DemoStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new DemoStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&Demo::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new DemoStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Demo::Started);
        }
    }
    return Q_SUPER(&Demo::Root);
}

QState Demo::Stopping(Demo * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = DemoStopReq::TIMEOUT_MS;
            me->m_stateTimer.Start(timeout);
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case DEMO_STOP_REQ: {
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
            return Q_TRAN(&Demo::Stopped);
        }
    }
    return Q_SUPER(&Demo::Root);
}

QState Demo::Started(Demo * const me, QEvt const * const e) {
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
            return Q_TRAN(&Demo::S2);
        }
    }
    return Q_SUPER(&Demo::Root);
}

QState Demo::S(Demo * const me, QEvt const * const e) {
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
            return Q_TRAN(&Demo::S11);
        }
        case DEMO_E_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S11);
        }
        case DEMO_I_REQ: {
            EVENT(e);
            if (me->m_foo) {
                me->m_foo = 0;
                return Q_HANDLED();
            }
            break;
        }
    }
    return Q_SUPER(&Demo::Started);
}

QState Demo::S1(Demo * const me, QEvt const * const e) {
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
            return Q_TRAN(&Demo::S11);
        }
        case DEMO_A_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S1);
        }
        case DEMO_B_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S11);
        }
        case DEMO_C_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S2);
        }
        case DEMO_D_REQ: {
            EVENT(e);
            if (!me->m_foo) {
                me->m_foo = 1;
                return Q_TRAN(&Demo::S);
            }
            break;
        }
        case DEMO_F_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S211);
        }
        case DEMO_I_REQ: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Demo::S);
}

QState Demo::S11(Demo * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DEMO_D_REQ: {
            EVENT(e);
            if (me->m_foo) {
                me->m_foo = 0;
                return Q_TRAN(&Demo::S1);
            }
            break;
        }
        case DEMO_H_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S);
        }
        case DEMO_G_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S211);
        }
    }
    return Q_SUPER(&Demo::S1);
}

QState Demo::S2(Demo * const me, QEvt const * const e) {
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
            EVENT(e);
            return Q_TRAN(&Demo::S211);
        }
        case DEMO_C_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S1);
        }
        case DEMO_F_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S11);
        }
        case DEMO_I_REQ: {
            EVENT(e);
            if (!me->m_foo) {
                me->m_foo = 1;
                return Q_HANDLED();
            }
            break;
        }
    }
    return Q_SUPER(&Demo::S);
}

QState Demo::S21(Demo * const me, QEvt const * const e) {
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
            EVENT(e);
            return Q_TRAN(&Demo::S211);
        }
        case DEMO_A_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S21);
        }
        case DEMO_B_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S211);
        }
        case DEMO_G_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S11);
        }
    }
    return Q_SUPER(&Demo::S2);
}

QState Demo::S211(Demo * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DEMO_D_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S21);
        }
        case DEMO_H_REQ: {
            EVENT(e);
            return Q_TRAN(&Demo::S);
        }
    }
    return Q_SUPER(&Demo::S21);
}

/*
QState Demo::MyState(Demo * const me, QEvt const * const e) {
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
            return Q_TRAN(&Demo::SubState);
        }
    }
    return Q_SUPER(&Demo::SuperState);
}
*/

} // namespace APP
