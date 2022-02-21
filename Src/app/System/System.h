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

#ifndef SYSTEM_H
#define SYSTEM_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_pipe.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

class System : public Active {
public:
    System();

protected:
    static QState InitialPseudoState(System * const me, QEvt const * const e);
    static QState Root(System * const me, QEvt const * const e);
        static QState Stopped(System * const me, QEvt const * const e);
        static QState Starting(System * const me, QEvt const * const e);
            static QState Prestarting(System * const me, QEvt const * const e);
            static QState Starting1(System * const me, QEvt const * const e);
            static QState Starting2(System * const me, QEvt const * const e);
            static QState Starting3(System * const me, QEvt const * const e);
        static QState Stopping(System * const me, QEvt const * const e);
            static QState Stopping1(System * const me, QEvt const * const e);
            static QState Stopping2(System * const me, QEvt const * const e);
        static QState Started(System * const me, QEvt const * const e);


    uint32_t m_maxIdleCnt;
    uint32_t m_cpuUtilPercent;      // CPU utilization in percentage.
    Evt m_inEvt;                    // Static event copy of a generic incoming req to be confirmed. Added more if needed.
    Timer m_stateTimer;
    Timer m_idleCntTimer;
    Timer m_sensorDelayTimer;
    Timer m_testTimer;

    enum {
        IDLE_CNT_INIT_TIMEOUT_MS = 200,
        IDLE_CNT_POLL_TIMEOUT_MS = 2000,
        SENSOR_DELAY_TIMEOUT_MS = 200,
    };


#define SYSTEM_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(IDLE_CNT_TIMER) \
    ADD_EVT(SENSOR_DELAY_TIMER) \
    ADD_EVT(TEST_TIMER)

#define SYSTEM_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(NEXT) \
    ADD_EVT(FAILED)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        SYSTEM_TIMER_EVT_START = TIMER_EVT_START(SYSTEM),
        SYSTEM_TIMER_EVT
    };

    enum {
        SYSTEM_INTERNAL_EVT_START = INTERNAL_EVT_START(SYSTEM),
        SYSTEM_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };
};

} // namespace APP

#endif // SYSTEM_H
