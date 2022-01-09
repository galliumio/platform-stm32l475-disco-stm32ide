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

#ifndef TRAFFIC_H
#define TRAFFIC_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "Lamp.h"

using namespace QP;
using namespace FW;

namespace APP {

class Traffic : public Active {
public:
    Traffic();

protected:
    static QState InitialPseudoState(Traffic * const me, QEvt const * const e);
    static QState Root(Traffic * const me, QEvt const * const e);
        static QState Stopped(Traffic * const me, QEvt const * const e);
        static QState Started(Traffic * const me, QEvt const * const e);
           static QState NSGo(Traffic * const me, QEvt const * const e);
               static QState NSMinTimeWait(Traffic * const me, QEvt const * const e);
               static QState NSMinTimeExceeded(Traffic * const me, QEvt const * const e);
           static QState NSSlow(Traffic * const me, QEvt const * const e);
           static QState EWGo(Traffic * const me, QEvt const * const e);
               static QState EWMinTimeWait(Traffic * const me, QEvt const * const e);
               static QState EWMinTimeExceeded(Traffic * const me, QEvt const * const e);
           static QState EWSlow(Traffic * const me, QEvt const * const e);
           static QState StopSign(Traffic * const me, QEvt const * const e);
               static QState StopSignOn(Traffic * const me, QEvt const * const e);
               static QState StopSignOff(Traffic * const me, QEvt const * const e);

    Lamp m_lampNS;          // Orthogonal region for the NS lamp.
    Lamp m_lampEW;          // Orthogonal region for the EW lamp.
    bool m_carWaiting;      // Remembers if a car is waiting in either NS or EW direction.

    enum {
        NS_MIN_WAIT_TIMEOUT_MS = 20000,
        NS_SLOW_TIMEOUT_MS     = 3000,
        EW_MIN_WAIT_TIMEOUT_MS = 10000,
        EW_SLOW_TIMEOUT_MS     = 3000,
        EW_IDLE_TIMEOUT_MS     = 15000,
        BLINK_TIMEOUT_MS       = 1000
    };
    Timer m_waitTimer;       // Timer used to wait for the minimum wait duration or the yellow light (slow-down) duration in either direction.
    Timer m_idleTimer;       // Timer used to detect idle condition in EW direction.
    Timer m_blinkTimer;      // Timer used to blink a stop sign.

#define TRAFFIC_TIMER_EVT \
    ADD_EVT(WAIT_TIMER) \
    ADD_EVT(IDLE_TIMER) \
    ADD_EVT(BLINK_TIMER)

// Placeholder only.
#define TRAFFIC_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        TRAFFIC_TIMER_EVT_START = TIMER_EVT_START(TRAFFIC),
        TRAFFIC_TIMER_EVT
    };

    enum {
        TRAFFIC_INTERNAL_EVT_START = INTERNAL_EVT_START(TRAFFIC),
        TRAFFIC_INTERNAL_EVT
    };

    // Placeholder only.
    class Failed : public ErrorEvt {
    public:
        Failed(Hsmn hsmn, Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, hsmn, hsmn, 0, error, origin, reason) {}
    };
};

} // namespace APP

#endif // TRAFFIC_H
