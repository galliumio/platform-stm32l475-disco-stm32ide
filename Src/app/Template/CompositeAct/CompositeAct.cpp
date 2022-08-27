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
#include "CompositeRegInterface.h"
#include "CompositeActInterface.h"
#include "CompositeAct.h"

FW_DEFINE_THIS_FILE("CompositeAct.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "COMPOSITE_ACT_TIMER_EVT_START",
    COMPOSITE_ACT_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "COMPOSITE_ACT_INTERNAL_EVT_START",
    COMPOSITE_ACT_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "COMPOSITE_ACT_INTERFACE_EVT_START",
    COMPOSITE_ACT_INTERFACE_EVT
};

CompositeAct::CompositeAct() :
    Active((QStateHandler)&CompositeAct::InitialPseudoState, COMPOSITE_ACT, "COMPOSITE_ACT"), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(GetHsmn(), STATE_TIMER) {
    SET_EVT_NAME(COMPOSITE_ACT);
}

QState CompositeAct::InitialPseudoState(CompositeAct * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&CompositeAct::Root);
}

QState CompositeAct::Root(CompositeAct * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            for (uint32_t i = 0; i < ARRAY_COUNT(me->m_compositeReg); i++) {
                me->m_compositeReg[i].Init(me);
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&CompositeAct::Stopped);
        }
        case COMPOSITE_ACT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new CompositeActStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case COMPOSITE_ACT_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&CompositeAct::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState CompositeAct::Stopped(CompositeAct * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case COMPOSITE_ACT_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new CompositeActStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case COMPOSITE_ACT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&CompositeAct::Starting);
        }
    }
    return Q_SUPER(&CompositeAct::Root);
}

QState CompositeAct::Starting(CompositeAct * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = CompositeActStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > CompositeRegStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            for (uint32_t i = 0; i < ARRAY_COUNT(me->m_compositeReg); i++) {
                me->SendReq(new CompositeRegStartReq(), COMPOSITE_REG + i, (i == 0));
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case COMPOSITE_REG_START_CFM: {
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
                me->SendCfm(new CompositeActStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new CompositeActStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&CompositeAct::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new CompositeActStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&CompositeAct::Started);
        }
    }
    return Q_SUPER(&CompositeAct::Root);
}

QState CompositeAct::Stopping(CompositeAct * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = CompositeActStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > CompositeRegStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            for (uint32_t i = 0; i < ARRAY_COUNT(me->m_compositeReg); i++) {
                me->SendReq(new CompositeRegStopReq(), COMPOSITE_REG + i, (i == 0));
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case COMPOSITE_ACT_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case COMPOSITE_REG_STOP_CFM: {
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
            return Q_TRAN(&CompositeAct::Stopped);
        }
    }
    return Q_SUPER(&CompositeAct::Root);
}

QState CompositeAct::Started(CompositeAct * const me, QEvt const * const e) {
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
    return Q_SUPER(&CompositeAct::Root);
}

/*
QState CompositeAct::MyState(CompositeAct * const me, QEvt const * const e) {
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
            return Q_TRAN(&CompositeAct::SubState);
        }
    }
    return Q_SUPER(&CompositeAct::SuperState);
}
*/

} // namespace APP
