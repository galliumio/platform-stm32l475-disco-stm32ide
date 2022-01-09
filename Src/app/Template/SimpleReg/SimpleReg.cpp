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
#include "SimpleRegInterface.h"
#include "SimpleReg.h"

FW_DEFINE_THIS_FILE("SimpleReg.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "SIMPLE_REG_TIMER_EVT_START",
    SIMPLE_REG_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "SIMPLE_REG_INTERNAL_EVT_START",
    SIMPLE_REG_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "SIMPLE_REG_INTERFACE_EVT_START",
    SIMPLE_REG_INTERFACE_EVT
};

SimpleReg::SimpleReg() :
    Region((QStateHandler)&SimpleReg::InitialPseudoState, SIMPLE_REG, "SIMPLE_REG"),
    m_stateTimer(GetHsmn(), STATE_TIMER) {
    SET_EVT_NAME(SIMPLE_REG);
}

QState SimpleReg::InitialPseudoState(SimpleReg * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&SimpleReg::Root);
}

QState SimpleReg::Root(SimpleReg * const me, QEvt const * const e) {
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
            return Q_TRAN(&SimpleReg::Stopped);
        }
        case SIMPLE_REG_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SimpleRegStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState SimpleReg::Stopped(SimpleReg * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case SIMPLE_REG_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SimpleRegStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case SIMPLE_REG_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SimpleRegStartCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&SimpleReg::Started);
        }
    }
    return Q_SUPER(&SimpleReg::Root);
}

QState SimpleReg::Started(SimpleReg * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case SIMPLE_REG_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new SimpleRegStopCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&SimpleReg::Stopped);
        }
    }
    return Q_SUPER(&SimpleReg::Root);
}

/*
QState SimpleReg::MyState(SimpleReg * const me, QEvt const * const e) {
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
            return Q_TRAN(&SimpleReg::SubState);
        }
    }
    return Q_SUPER(&SimpleReg::SuperState);
}
*/

} // namespace APP

