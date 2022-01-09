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

#ifndef LEVEL_METER_INTERFACE_H
#define LEVEL_METER_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "fw_msg.h"
#include "app_hsmn.h"
#include "SensorMsgInterface.h"

#define LEVEL_METER_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("LevelMeterInterface.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

#define LEVEL_METER_INTERFACE_EVT \
    ADD_EVT(LEVEL_METER_START_REQ) \
    ADD_EVT(LEVEL_METER_START_CFM) \
    ADD_EVT(LEVEL_METER_STOP_REQ) \
    ADD_EVT(LEVEL_METER_STOP_CFM) \
    ADD_EVT(LEVEL_METER_CONTROL_REQ) \
    ADD_EVT(LEVEL_METER_CONTROL_CFM) \
    ADD_EVT(LEVEL_METER_DATA_IND) \
    ADD_EVT(LEVEL_METER_DATA_RSP)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    LEVEL_METER_INTERFACE_EVT_START = INTERFACE_EVT_START(LEVEL_METER),
    LEVEL_METER_INTERFACE_EVT
};

enum {
    LEVEL_METER_REASON_UNSPEC = 0,
};

class LevelMeterStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 400
    };
    LevelMeterStartReq() :
        Evt(LEVEL_METER_START_REQ) {}
};

class LevelMeterStartCfm : public ErrorEvt {
public:
    LevelMeterStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LEVEL_METER_START_CFM, error, origin, reason) {}
};

class LevelMeterStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 400
    };
    LevelMeterStopReq() :
        Evt(LEVEL_METER_STOP_REQ) {}
};

class LevelMeterStopCfm : public ErrorEvt {
public:
    LevelMeterStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(LEVEL_METER_STOP_CFM, error, origin, reason) {}
};

class LevelMeterControlReq : public MsgEvt {
public:
    enum {
        TIMEOUT_MS = 300
    };
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'msg'.
    LevelMeterControlReq(SensorControlReqMsg const &r) :
        MsgEvt(LEVEL_METER_CONTROL_REQ, m_msg), m_msg(r) {
        LEVEL_METER_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    float GetPitchThres() const { return m_msg.GetPitchThres(); }
    float GetRollThres() const { return m_msg.GetRollThres(); }
protected:
    SensorControlReqMsg m_msg;
};

class LevelMeterControlCfm : public ErrorMsgEvt {
public:
    LevelMeterControlCfm(SensorControlCfmMsg const &r) :
        ErrorMsgEvt(LEVEL_METER_CONTROL_CFM, m_msg), m_msg(r) {
        LEVEL_METER_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
protected:
    SensorControlCfmMsg m_msg;
};

class LevelMeterDataInd : public MsgEvt {
public:
    enum {
        TIMEOUT_MS = 300
    };
    // Must pass 'm_msg' as reference to member object and NOT the parameter 'msg'.
    LevelMeterDataInd(SensorDataIndMsg const &r) :
        MsgEvt(LEVEL_METER_DATA_IND, m_msg), m_msg(r) {
        LEVEL_METER_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
    uint32_t GetPitch() const { return m_msg.GetPitch(); }
    uint32_t GetRoll() const { return m_msg.GetRoll(); }
protected:
    SensorDataIndMsg m_msg;
};

class LevelMeterDataRsp : public ErrorMsgEvt {
public:
    LevelMeterDataRsp(SensorDataRspMsg const &r) :
        ErrorMsgEvt(LEVEL_METER_DATA_RSP, m_msg), m_msg(r) {
        LEVEL_METER_INTERFACE_ASSERT(&GetMsgBase() == &m_msg);
    }
protected:
    SensorDataRspMsg m_msg;
};

} // namespace APP

#endif // LEVEL_METER_INTERFACE_H
