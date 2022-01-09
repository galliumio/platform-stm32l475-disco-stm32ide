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
#include "SimpleActInterface.h"
#include "SimpleAct.h"

FW_DEFINE_THIS_FILE("SimpleAct.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "SIMPLE_ACT_TIMER_EVT_START",
    SIMPLE_ACT_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "SIMPLE_ACT_INTERNAL_EVT_START",
    SIMPLE_ACT_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "SIMPLE_ACT_INTERFACE_EVT_START",
    SIMPLE_ACT_INTERFACE_EVT
};

SimpleAct::SimpleAct() :
    Active((QStateHandler)&SimpleAct::InitialPseudoState, SIMPLE_ACT, "SIMPLE_ACT"), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(GetHsmn(), STATE_TIMER) {
    SET_EVT_NAME(SIMPLE_ACT);
}

QState SimpleAct::InitialPseudoState(SimpleAct * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&SimpleAct::Root);
}

QState SimpleAct::Root(SimpleAct * const me, QEvt const * const e) {
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
            return Q_TRAN(&SimpleAct::Stopped);
        }
        case SIMPLE_ACT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SimpleActStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case SIMPLE_ACT_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&SimpleAct::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState SimpleAct::Stopped(SimpleAct * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case SIMPLE_ACT_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SimpleActStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case SIMPLE_ACT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&SimpleAct::Starting);
        }
    }
    return Q_SUPER(&SimpleAct::Root);
}

QState SimpleAct::Starting(SimpleAct * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SimpleActStartReq::TIMEOUT_MS;
            //FW_ASSERT(timeout > XxxStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            //me->SendReq(new XxxStartReq(), XXX, true);
            //me->SendReq(new YyyStartReq(), YYY, false);
            //me->SendReq(new ZzzStartReq(), ZZZ, false);
            //...
            // For testing, send DONE immediately. Do not use Raise() in entry action.
            me->Send(new Evt(DONE), me->GetHsmn());
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        /*
        case XXX_START_CFM: {
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
        */
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new SimpleActStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new SimpleActStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&SimpleAct::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new SimpleActStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&SimpleAct::Started);
        }
    }
    return Q_SUPER(&SimpleAct::Root);
}

QState SimpleAct::Stopping(SimpleAct * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = SimpleActStopReq::TIMEOUT_MS;
            //FW_ASSERT(timeout > XxxStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            //me->SendReq(new XxxStopReq(), XXX, true);
            //me->SendReq(new YyyStopReq(), YYY, false);
            //me->SendReq(new ZzzStopReq(), ZZZ, false);
            //...
            // For testing, send DONE immediately. Do not use Raise() in entry action.
            me->Send(new Evt(DONE), me->GetHsmn());
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case SIMPLE_ACT_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        /*
        case XXX_STOP_CFM: {
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
        */
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&SimpleAct::Stopped);
        }
    }
    return Q_SUPER(&SimpleAct::Root);
}

QState SimpleAct::Started(SimpleAct * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&SimpleAct::Root);
}

/*
QState SimpleAct::MyState(SimpleAct * const me, QEvt const * const e) {
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
            return Q_TRAN(&SimpleAct::SubState);
        }
    }
    return Q_SUPER(&SimpleAct::SuperState);
}
*/

} // namespace APP
