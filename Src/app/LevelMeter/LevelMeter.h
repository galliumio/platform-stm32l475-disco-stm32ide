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

#ifndef LEVEL_METER_H
#define LEVEL_METER_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "fw_seqrec.h"
#include "app_hsmn.h"
#include "SensorAccelGyroInterface.h"
#include "SensorHumidTempInterface.h"

using namespace QP;
using namespace FW;

namespace APP {

class LevelMeter : public Active {
public:
    LevelMeter();

protected:
    static QState InitialPseudoState(LevelMeter * const me, QEvt const * const e);
    static QState Root(LevelMeter * const me, QEvt const * const e);
        static QState Stopped(LevelMeter * const me, QEvt const * const e);
        static QState Starting(LevelMeter * const me, QEvt const * const e);
        static QState Stopping(LevelMeter * const me, QEvt const * const e);
        static QState Started(LevelMeter * const me, QEvt const * const e);
            static QState Normal(LevelMeter * const me, QEvt const * const e);
            static QState Redrawing(LevelMeter * const me, QEvt const * const e);

    enum {
        ACCEL_GYRO_PIPE_ORDER = 7,
        HUMID_TEMP_PIPE_ORDER = 2,
    };
    AccelGyroReport m_accelGyroStor[1 << ACCEL_GYRO_PIPE_ORDER];
    HumidTempReport m_humidTempStor[1 << HUMID_TEMP_PIPE_ORDER];
    AccelGyroPipe m_accelGyroPipe;
    HumidTempPipe m_humidTempPipe;
    AccelGyroReport m_avgReport;
    float m_pitch;              // Average pitch in degree.
    float m_roll;               // Average roll in degree.
    float m_pitchThres;         // Pitch alarm threshold in degree (applies to negative threshold).
    float m_rollThres;          // Roll alarm threshold in degree (applies to negative threshold).
    float m_humidity;           // Latest processor humidity measurement.
    float m_temperature;        // Latest processor temperature measurement.
    Evt m_inEvt;                // Static event copy of a generic incoming req to be confirmed. Added more if needed.
    MsgSeqRec m_msgSeq;         // Keeps track of sequence numbers of outgoing messages.

    enum {
        REPORT_TIMEOUT_MS = 100 //333
    };

    Timer m_stateTimer;
    Timer m_reportTimer;

#define LEVEL_METER_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(REPORT_TIMER)

#define LEVEL_METER_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED) \
    ADD_EVT(REDRAW)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        LEVEL_METER_TIMER_EVT_START = TIMER_EVT_START(LEVEL_METER),
        LEVEL_METER_TIMER_EVT
    };

    enum {
        LEVEL_METER_INTERNAL_EVT_START = INTERNAL_EVT_START(LEVEL_METER),
        LEVEL_METER_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };
};

} // namespace APP

#endif // LEVEL_METER_H
