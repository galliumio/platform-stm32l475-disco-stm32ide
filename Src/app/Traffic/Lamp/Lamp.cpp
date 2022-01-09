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
#include "DispInterface.h"
#include "LampInterface.h"
#include "Lamp.h"

FW_DEFINE_THIS_FILE("Lamp.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "LAMP_TIMER_EVT_START",
    LAMP_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "LAMP_INTERNAL_EVT_START",
    LAMP_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "LAMP_INTERFACE_EVT_START",
    LAMP_INTERFACE_EVT
};

// Helper functions.
void Lamp::Draw(Hsmn hsmn, bool redOn, bool yellowOn, bool greenOn) {
    char const *buf;
    uint32_t xOffset;
    if (hsmn == LAMP_NS) {
        buf = "NS";
        xOffset = 10;
    } else {
        buf = "EW";
        xOffset = 120;
    }
    // Draw label text.
    Send(new DispDrawTextReq(buf, xOffset, 50, COLOR24_BLACK, COLOR24_WHITE, 3), ILI9341);
    // Draw red lamp.
    Send(new DispDrawRectReq(xOffset, 100, 50, 50, redOn ? COLOR24_RED : COLOR24_BLACK), ILI9341);
    // Draw red lamp.
    Send(new DispDrawRectReq(xOffset, 155, 50, 50, yellowOn ? COLOR24_YELLOW : COLOR24_BLACK), ILI9341);
    // Draw red lamp.
    Send(new DispDrawRectReq(xOffset, 210, 50, 50, greenOn ? COLOR24_GREEN : COLOR24_BLACK), ILI9341);
}

Lamp::Lamp(Hsmn hsmn, char const *name) :
    Region((QStateHandler)&Lamp::InitialPseudoState, hsmn, name) {
    SET_EVT_NAME(LAMP);
}

QState Lamp::InitialPseudoState(Lamp * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Lamp::Root);
}

QState Lamp::Root(Lamp * const me, QEvt const * const e) {
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
            return Q_TRAN(&Lamp::Red);
        }
        case LAMP_RESET_REQ:
        case LAMP_RED_REQ: {
            EVENT(e);
            return Q_TRAN(&Lamp::Red);
        }
        case LAMP_OFF_REQ: {
            EVENT(e);
            return Q_TRAN(&Lamp::Off);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Lamp::Red(Lamp * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            LOG("[*][ ][ ]");
            me->Draw(me->GetHsmn(), true, false, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LAMP_GREEN_REQ: {
            EVENT(e);
            return Q_TRAN(&Lamp::Green);
        }
    }
    return Q_SUPER(&Lamp::Root);
}

QState Lamp::Green(Lamp * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            LOG("[ ][ ][*]");
            me->Draw(me->GetHsmn(), false, false, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LAMP_YELLOW_REQ: {
            EVENT(e);
            return Q_TRAN(&Lamp::Yellow);
        }
    }
    return Q_SUPER(&Lamp::Root);
}

QState Lamp::Yellow(Lamp * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            LOG("[ ][*][ ]");
            me->Draw(me->GetHsmn(), false, true, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Lamp::Root);
}

QState Lamp::Off(Lamp * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            LOG("[ ][ ][ ]");
            me->Draw(me->GetHsmn(), false, false, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Lamp::Root);
}

/*
QState Lamp::MyState(Lamp * const me, QEvt const * const e) {
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
            return Q_TRAN(&Lamp::SubState);
        }
    }
    return Q_SUPER(&Lamp::SuperState);
}
*/

} // namespace APP
