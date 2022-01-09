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

#ifndef AO_WASHING_MACHINE_H
#define AO_WASHING_MACHINE_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

class AOWashingMachine : public Active {
public:
    AOWashingMachine();

protected:
    static QState InitialPseudoState(AOWashingMachine * const me, QEvt const * const e);
    static QState Root(AOWashingMachine * const me, QEvt const * const e);
        static QState Stopped(AOWashingMachine * const me, QEvt const * const e);
        static QState Started(AOWashingMachine * const me, QEvt const * const e);
            static QState DoorOpen(AOWashingMachine * const me, QEvt const * const e);
            static QState DoorClosed(AOWashingMachine * const me, QEvt const * const e);
                static QState DoorUnlocked(AOWashingMachine * const me, QEvt const * const e);
                static QState DoorLocked(AOWashingMachine * const me, QEvt const * const e);
                    static QState FillingWash(AOWashingMachine * const me, QEvt const * const e);
                    static QState Washing(AOWashingMachine * const me, QEvt const * const e);
                    static QState DrainingWash(AOWashingMachine * const me, QEvt const * const e);
                    static QState FillingRinse(AOWashingMachine * const me, QEvt const * const e);
                    static QState Rinsing(AOWashingMachine * const me, QEvt const * const e);
                    static QState DrainingRinse(AOWashingMachine * const me, QEvt const * const e);
                    static QState Spinning(AOWashingMachine * const me, QEvt const * const e);

    // Helper functions
    uint8_t CheckDoorSensor();
    void ErrorBeep();
    void PlayCompletionSong();
    void StartFilling();
    void StartWashing();
    void StartDraining();
    void StartRinsing();
    void StartSpinning();

    void StopFilling();
    void StopWashing();
    void StopDraining();
    void StopRinsing();
    void StopSpinning();

    void LockDoor();
    void UnlockDoor();
    void PrintCycleParams();

    enum {
        DOOR_OPEN,
        DOOR_CLOSED
    };

    typedef enum {
        NORMAL,
        DELICATE,
        BULKY,
        TOWELS
    } WashType;

    typedef struct {
        uint16_t washTime; // ms
        uint16_t rinseTime; // ms
        uint16_t washTemperature;
        uint16_t rinseTemperature;
        WashType washType;
    } cycleParameters_t;

    cycleParameters_t m_cycleParams;
    uint16_t m_doorState;

    // m_history is a function pointer used for saving the previous state.
    QState (*m_history) (AOWashingMachine * const me, QEvt const * const e);

    Timer m_washTimer;
    Timer m_rinseTimer;
    Timer m_spinTimer;

#define AO_WASHING_MACHINE_TIMER_EVT \
    ADD_EVT(WASH_TIMEOUT) \
    ADD_EVT(RINSE_TIMEOUT) \
    ADD_EVT(SPIN_TIMEOUT)

#define AO_WASHING_MACHINE_INTERNAL_EVT \
    ADD_EVT(WASH_CLOSE) \
    ADD_EVT(WASH_OPEN) \
    ADD_EVT(WASH_DONE)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        AO_WASHING_MACHINE_TIMER_EVT_START = TIMER_EVT_START(AO_WASHING_MACHINE),
        AO_WASHING_MACHINE_TIMER_EVT
    };

    enum {
        AO_WASHING_MACHINE_INTERNAL_EVT_START = INTERNAL_EVT_START(AO_WASHING_MACHINE),
        AO_WASHING_MACHINE_INTERNAL_EVT
    };
};

} // namespace APP

#endif // AO_WASHING_MACHINE_H

