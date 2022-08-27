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

#ifndef AO_WASHING_MACHINE_INTERFACE_H
#define AO_WASHING_MACHINE_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

#define AO_WASHING_MACHINE_INTERFACE_EVT \
    ADD_EVT(USER_SIM_START_REQ) \
    ADD_EVT(USER_SIM_START_CFM) \
    ADD_EVT(USER_SIM_STOP_REQ) \
    ADD_EVT(USER_SIM_STOP_CFM) \
    ADD_EVT(OPEN_DOOR_IND) \
    ADD_EVT(CLOSE_DOOR_IND) \
    ADD_EVT(START_PAUSE_BUTTON_IND) \
    ADD_EVT(CYCLE_SELECTED_IND) \
    ADD_EVT(WASH_START_REQ) \
    ADD_EVT(WASH_START_CFM) \
    ADD_EVT(WASH_STOP_REQ) \
    ADD_EVT(WASH_STOP_CFM) \
    ADD_EVT(WASH_FILLED_IND) \
    ADD_EVT(WASH_DRAINED_IND)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    AO_WASHING_MACHINE_INTERFACE_EVT_START = INTERFACE_EVT_START(AO_WASHING_MACHINE),
    AO_WASHING_MACHINE_INTERFACE_EVT
};

enum {
    AO_WASHING_MACHINE_REASON_UNSPEC = 0,
};

class UserSimOpenDoorInd : public Evt {
public:
    UserSimOpenDoorInd() :
        Evt(OPEN_DOOR_IND) {}
};

class UserSimCloseDoorInd : public Evt {
public:
    UserSimCloseDoorInd() :
        Evt(CLOSE_DOOR_IND) {}
};

class UserSimStartPauseInd : public Evt {
public:
    UserSimStartPauseInd() :
        Evt(START_PAUSE_BUTTON_IND) {}
};

class UserSimCycleSelectedInd : public Evt {
public:
    typedef enum {
        NORMAL,
        DELICATE,
        BULKY,
        TOWELS
    } CycleType;
    UserSimCycleSelectedInd(CycleType cycle) :
        Evt(CYCLE_SELECTED_IND), m_cycleType(cycle) {}
    CycleType GetCycleType() const { return m_cycleType; }
private:
    CycleType m_cycleType;
};

class WashStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    WashStartReq() :
        Evt(WASH_START_REQ) {}
};

class WashStartCfm : public ErrorEvt {
public:
    WashStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WASH_START_CFM, error, origin, reason) {}
};

class WashStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    WashStopReq() :
        Evt(WASH_STOP_REQ) {}
};

class WashStopCfm : public ErrorEvt {
public:
    WashStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(WASH_STOP_CFM, error, origin, reason) {}
};

} // namespace APP

#endif // AO_WASHING_MACHINE_INTERFACE_H
