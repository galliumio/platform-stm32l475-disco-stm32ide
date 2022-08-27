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
#include "GpioOutInterface.h"
#include "AOWashingMachineInterface.h"
#include "AOWashingMachine.h"

FW_DEFINE_THIS_FILE("AOWashingMachine.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "AO_WASHING_MACHINE_TIMER_EVT_START",
    AO_WASHING_MACHINE_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "AO_WASHING_MACHINE_INTERNAL_EVT_START",
    AO_WASHING_MACHINE_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "AO_WASHING_MACHINE_INTERFACE_EVT_START",
    AO_WASHING_MACHINE_INTERFACE_EVT
};

// Constants used within this file.
static const uint16_t NORMAL_WASH_TIME_MS = 3000;
static const uint16_t NORMAL_RINSE_TIME_MS = 3000;
static const uint16_t NORMAL_WASH_TEMP = 100;  // Degrees F
static const uint16_t NORMAL_RINSE_TEMP = 100; // Degrees F

static const uint16_t DELICATE_WASH_TIME_MS = 3000;
static const uint16_t DELICATE_RINSE_TIME_MS = 3000;
static const uint16_t DELICATE_WASH_TEMP = 70;  // Degrees F
static const uint16_t DELICATE_RINSE_TEMP = 70; // Degrees F

static const uint16_t BULKY_WASH_TIME_MS = 10000;
static const uint16_t BULKY_RINSE_TIME_MS = 10000;
static const uint16_t BULKY_WASH_TEMP = 100;  // Degrees F
static const uint16_t BULKY_RINSE_TEMP = 100; // Degrees F

static const uint16_t TOWELS_WASH_TIME_MS = 6000;
static const uint16_t TOWELS_RINSE_TIME_MS = 6000;
static const uint16_t TOWELS_WASH_TEMP = 150;  // Degrees F
static const uint16_t TOWELS_RINSE_TEMP = 150; // Degrees F

static const uint16_t SPIN_TIME_MS = 5000;

AOWashingMachine::AOWashingMachine() :
    Active((QStateHandler)&AOWashingMachine::InitialPseudoState, AO_WASHING_MACHINE, "AO_WASHING_MACHINE"),
       m_doorState(DOOR_OPEN),
       m_history(&FillingWash),
       m_washTimer(GetHsmn(), WASH_TIMEOUT),
       m_rinseTimer(GetHsmn(), RINSE_TIMEOUT),
       m_spinTimer(GetHsmn(), SPIN_TIMEOUT) {
    SET_EVT_NAME(AO_WASHING_MACHINE);
}

QState AOWashingMachine::InitialPseudoState(AOWashingMachine * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&AOWashingMachine::Root);
}

QState AOWashingMachine::Root(AOWashingMachine * const me, QEvt const * const e) {
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
            return Q_TRAN(&AOWashingMachine::Stopped);
        }
    }
    return Q_SUPER(&QHsm::top);
}

/**
 * This is the state where the state machine waits to be started by the system manager.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::Stopped(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            return Q_HANDLED();
        }
        case WASH_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WashStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case WASH_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WashStartCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&AOWashingMachine::Started);
        }
    }
    return Q_SUPER(&AOWashingMachine::Root);
}

/**
 * In the Started state, the washing machine is ready to be used.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::Started(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);

            // Initialize cycle parameters to NORMAL wash
            me->m_cycleParams.washTime = NORMAL_WASH_TIME_MS;
            me->m_cycleParams.rinseTime = NORMAL_RINSE_TIME_MS;
            me->m_cycleParams.washTemperature = NORMAL_WASH_TEMP;
            me->m_cycleParams.rinseTemperature = NORMAL_RINSE_TEMP;

            // Initialize the history to the first step in the wash cycle.
            me->m_history = &FillingWash;

            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG:
        {
            if (me->CheckDoorSensor() == DOOR_OPEN)
            {
                return Q_TRAN(&AOWashingMachine::DoorOpen);
            }
            else
            {
                return Q_TRAN(&AOWashingMachine::DoorClosed);
            }
        }
        case WASH_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            // @todo - Need to make sure system is properly shutdown.
            me->SendCfm(new WashStopCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&AOWashingMachine::Stopped);
        }
        case CYCLE_SELECTED_IND:
        {
            EVENT(e);

            UserSimCycleSelectedInd const &ind = static_cast<UserSimCycleSelectedInd const &>(*e);

            switch (ind.GetCycleType())
            {
                case UserSimCycleSelectedInd::DELICATE:
                {
                    me->m_cycleParams.washTime = DELICATE_WASH_TIME_MS;
                    me->m_cycleParams.rinseTime = DELICATE_RINSE_TIME_MS;
                    me->m_cycleParams.washTemperature = DELICATE_WASH_TEMP;
                    me->m_cycleParams.rinseTemperature = DELICATE_RINSE_TEMP;
                    break;
                }
                case UserSimCycleSelectedInd::TOWELS:
                {
                    me->m_cycleParams.washTime = TOWELS_WASH_TIME_MS;
                    me->m_cycleParams.rinseTime = TOWELS_RINSE_TIME_MS;
                    me->m_cycleParams.washTemperature = TOWELS_WASH_TEMP;
                    me->m_cycleParams.rinseTemperature = TOWELS_RINSE_TEMP;
                    break;
                }
                case UserSimCycleSelectedInd::BULKY:
                {
                    me->m_cycleParams.washTime = BULKY_WASH_TIME_MS;
                    me->m_cycleParams.rinseTime = BULKY_RINSE_TIME_MS;
                    me->m_cycleParams.washTemperature = BULKY_WASH_TEMP;
                    me->m_cycleParams.rinseTemperature = BULKY_RINSE_TEMP;
                    break;
                }
                case UserSimCycleSelectedInd::NORMAL:
                default:
                {
                    me->m_cycleParams.washTime = NORMAL_WASH_TIME_MS;
                    me->m_cycleParams.rinseTime = NORMAL_RINSE_TIME_MS;
                    me->m_cycleParams.washTemperature = NORMAL_WASH_TEMP;
                    me->m_cycleParams.rinseTemperature = NORMAL_RINSE_TEMP;
                    break;
                }
            }

            me->m_history = &FillingWash;
            me->PrintCycleParams();

            return Q_HANDLED();
        }
    }
    return Q_SUPER(&AOWashingMachine::Root);
}

/**
 * This state models the behavior when the door is open.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::DoorOpen(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->m_doorState = DOOR_OPEN;
            // Uncomment to hook up with LED.
            //me->Send(new GpioOutOffReq(), USER_LED);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            return Q_HANDLED();
        }
        case START_PAUSE_BUTTON_IND:
        {
            EVENT(e);
            me->ErrorBeep();
            return Q_HANDLED();
        }
        case CLOSE_DOOR_IND:
        {
            EVENT(e);
            me->Raise(new Evt(WASH_CLOSE));
            return Q_HANDLED();
        }
        case WASH_CLOSE:
        {
            return Q_TRAN(&AOWashingMachine::DoorClosed);
        }
    }
    return Q_SUPER(&AOWashingMachine::Started);
}

/**
 * This state models the behavior when the door is closed. This is where the washing happens.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */

QState AOWashingMachine::DoorClosed(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->m_doorState = DOOR_CLOSED;
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG:
        {
            return Q_TRAN(&AOWashingMachine::DoorUnlocked);
        }
        case WASH_OPEN:
        {
            return Q_TRAN(&AOWashingMachine::DoorOpen);
        }
    }
    return Q_SUPER(&AOWashingMachine::Started);
}

/**
 * When the door is unlocked, the user is allowed to open the door, select the cycle,
 * and start a wash cycle.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::DoorUnlocked(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->UnlockDoor();
            // Uncomment to hook up with LED.
            //me->Send(new GpioOutPatternReq(0, true), USER_LED);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            return Q_HANDLED();
        }
        case OPEN_DOOR_IND:
        {
            EVENT(e);
            me->Raise(new Evt(WASH_OPEN));
            return Q_HANDLED();
        }
        case START_PAUSE_BUTTON_IND:
        {
            EVENT(e);
            // Go to the state pointed to by m_history.
            return Q_TRAN(me->m_history);
        }
    }
    return Q_SUPER(&AOWashingMachine::DoorClosed);
}

/**
 * In the Locked state, the washing machine is actively doing its cycle.
 * The user is not allowed to open the door because the machine is full
 * of water. Since a cycle is running, the user cannot select a new cycle type.
 * The user can pause the cycle.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::DoorLocked(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->LockDoor();
            // Uncomment to hook up with LED.
            //me->Send(new GpioOutPatternReq(1, true), USER_LED);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            return Q_HANDLED();
        }
        case START_PAUSE_BUTTON_IND:
        {
            EVENT(e);
            return Q_TRAN(&AOWashingMachine::DoorUnlocked);
        }
        case OPEN_DOOR_IND:
        {
            EVENT(e);
            PRINT("Can't open door while locked!!\r\n");
            return Q_HANDLED();
        }
        case WASH_DONE:
        {
            return Q_TRAN(&AOWashingMachine::DoorUnlocked);
        }
    }
    return Q_SUPER(&AOWashingMachine::DoorClosed);
}

/**
 * In the FillingWash state, the washing machine is being filled with water in
 * preparation for washing.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::FillingWash(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->m_history = &FillingWash;
            me->StartFilling();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            me->StopFilling();
            return Q_HANDLED();
        }
        case WASH_FILLED_IND:
        {
            EVENT(e);
            return Q_TRAN(&AOWashingMachine::Washing);
        }
    }
    return Q_SUPER(&AOWashingMachine::DoorLocked);
}

/**
 * In this state the wash cycle is run for the time specified in the cycle parameters.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::Washing(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->m_history = &Washing;
            me->m_washTimer.Start(me->m_cycleParams.washTime);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            me->m_washTimer.Stop();
            me->StopWashing();
            me->StartDraining();
            return Q_HANDLED();
        }
        case WASH_TIMEOUT:
        {
            EVENT(e);
            return Q_TRAN(&AOWashingMachine::DrainingWash);
        }
    }
    return Q_SUPER(&AOWashingMachine::DoorLocked);
}

/**
 * In this state the wash cycle is complete and the water is drained from
 * the machine.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::DrainingWash(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->m_history = &DrainingWash;
            me->StartDraining();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            return Q_HANDLED();
        }
        case WASH_DRAINED_IND:
        {
            EVENT(e);
            return Q_TRAN(&AOWashingMachine::FillingRinse);
        }
    }
    return Q_SUPER(&AOWashingMachine::DoorLocked);
}

/**
 * In this state the washing machine is filled with water in preparation for rinsing.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::FillingRinse(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->m_history = &FillingRinse;
            me->StartFilling();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            me->StopFilling();
            return Q_HANDLED();
        }
        case WASH_FILLED_IND:
        {
            EVENT(e);
            return Q_TRAN(&AOWashingMachine::Rinsing);
        }
    }
    return Q_SUPER(&AOWashingMachine::DoorLocked);
}

/**
 * In this state the rinse cycle runs for the time specified in the cycle parameters.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::Rinsing(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->m_history = &Rinsing;
            me->StartRinsing();
            me->m_rinseTimer.Start(me->m_cycleParams.rinseTime);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            me->m_rinseTimer.Stop();
            me->StopRinsing();
            me->StartDraining();
            return Q_HANDLED();
        }
        case RINSE_TIMEOUT:
        {
            EVENT(e);
            return Q_TRAN(&AOWashingMachine::DrainingRinse);
        }
    }
    return Q_SUPER(&AOWashingMachine::DoorLocked);
}

/**
 * In this state the rinse water is drained from the machine.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::DrainingRinse(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->m_history = &DrainingRinse;
            me->StartDraining();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            return Q_HANDLED();
        }
        case WASH_DRAINED_IND:
        {
            EVENT(e);
            return Q_TRAN(&AOWashingMachine::Spinning);
        }
    }
    return Q_SUPER(&AOWashingMachine::DoorLocked);
}

/**
 * In this state the spinning begins. After spinning, the wash cycle is complete, and we
 * go back to the DoorUnlocked state.
 *
 * @param me - Pointer to class
 * @param pEvent - The event to process
 *
 * @return Q_HANDLED if the event was processed, the containing state if not
 * processed.
 */
QState AOWashingMachine::Spinning(AOWashingMachine * const me, QEvt const * const e)
{
    switch (e->sig)
    {
        case Q_ENTRY_SIG:
        {
            EVENT(e);
            me->m_history = &Spinning;
            me->m_spinTimer.Start(SPIN_TIME_MS);
            me->StartSpinning();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG:
        {
            EVENT(e);
            me->m_spinTimer.Stop();
            me->StopSpinning();
            if (me->m_history == &AOWashingMachine::FillingWash)
            {
                me->PlayCompletionSong();
            }
            return Q_HANDLED();
        }
        case SPIN_TIMEOUT:
        {
            EVENT(e);
            me->m_history = &FillingWash;
            me->Raise(new Evt(WASH_DONE));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&AOWashingMachine::DoorLocked);
}

// Helper functions
uint8_t AOWashingMachine::CheckDoorSensor()
{
    return m_doorState;
}

void AOWashingMachine::ErrorBeep()
{
    PRINT("    ** Error Beep **\r\n");
}

void AOWashingMachine::PlayCompletionSong()
{
    PRINT("    ** Wash all done!! **\r\n");
}

void AOWashingMachine::StartFilling()
{
    PRINT("    ** Filling started. **\r\n");

    // Send the wash filled event as if a washing machine sensor
    // had been triggered.
    Send(new Evt(WASH_FILLED_IND), GetHsmn());
}

void AOWashingMachine::StartWashing()
{
    PRINT("    ** Washing started. **\r\n");
}

void AOWashingMachine::StartDraining()
{
    PRINT("    ** Draining started. **\r\n");
    // Send the wash drained event as if a washing machine sensor
    // had been triggered.
    Send(new Evt(WASH_DRAINED_IND), GetHsmn());
}

void AOWashingMachine::StartRinsing()
{
    PRINT("    ** Rinsing started. **\r\n");
}

void AOWashingMachine::StartSpinning()
{
    PRINT("    ** Spinning started. **\r\n");
}

void AOWashingMachine::StopFilling()
{
    PRINT("    ** Filling stopped. **\r\n");
}

void AOWashingMachine::StopWashing()
{
    PRINT("    ** Washing stopped. **\r\n");
}

void AOWashingMachine::StopDraining()
{
    PRINT("    ** Draining stopped. **\r\n");
}

void AOWashingMachine::StopRinsing()
{
    PRINT("    ** Rinsing stopped. **\r\n");
}

void AOWashingMachine::StopSpinning()
{
    PRINT("    ** Spinning stopped. **\r\n");
}

void AOWashingMachine::LockDoor()
{
    PRINT("    ** Door locked **\r\n");
}

void AOWashingMachine::UnlockDoor()
{
    PRINT("    ** Door unlocked. **\r\n");
}

void AOWashingMachine::PrintCycleParams()
{
    PRINT("\r\n");
    PRINT("    Wash time = %d s\r\n", m_cycleParams.washTime/1000);
    PRINT("    Rinse time = %d s\r\n", m_cycleParams.rinseTime/1000);
    PRINT("    Wash temperature = %d F\r\n", m_cycleParams.washTemperature);
    PRINT("    Rinse temperature = %d F\r\n", m_cycleParams.rinseTemperature);
    PRINT("\r\n");
}

} // namespace APP
