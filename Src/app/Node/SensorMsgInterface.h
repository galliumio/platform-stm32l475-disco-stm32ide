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

#ifndef SENSOR_MSG_INTERFACE_H
#define SENSOR_MSG_INTERFACE_H

#include "fw_def.h"
#include "fw_msg.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

// This file defines messages for the "Sensor" role.

class SensorControlReqMsg final: public Msg {
public:
    SensorControlReqMsg(float pitchThres = 0.0, float rollThres = 0.0) :
        Msg("SensorControlReqMsg"), m_pitchThres(pitchThres), m_rollThres(rollThres) {
        m_len = sizeof(*this);
    }
    float GetPitchThres() const { return m_pitchThres; }
    float GetRollThres() const { return m_rollThres; }
protected:
    float m_pitchThres;  // Pitch alarm threshold in degree.
    float m_rollThres;   // Roll alarm threshold in degree.
} __attribute__((packed));

class SensorControlCfmMsg final: public ErrorMsg {
public:
    SensorControlCfmMsg(char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        ErrorMsg("SensorControlCfmMsg", error, origin, reason) {
        m_len = sizeof(*this);
    }
} __attribute__((packed));

class SensorDataIndMsg final: public Msg {
public:
    SensorDataIndMsg(float pitch = 0.0, float roll = 0.0) :
        Msg("SensorDataIndMsg"), m_pitch(pitch), m_roll(roll) {
        m_len = sizeof(*this);
    }
    uint32_t GetPitch() const { return m_pitch; }
    uint32_t GetRoll() const { return m_roll; }
protected:
    float m_pitch;  // Pitch in degree.
    float m_roll;   // Roll in degree.
} __attribute__((packed));

class SensorDataRspMsg final: public ErrorMsg {
public:
    SensorDataRspMsg(char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        ErrorMsg("SensorDataRspMsg", error, origin, reason) {
        m_len = sizeof(*this);
    }
} __attribute__((packed));

} // namespace APP

#endif // SENSOR_MSG_INTERFACE_H
