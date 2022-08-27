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

#ifndef SENSOR_ACCEL_GYRO_INTERFACE_H
#define SENSOR_ACCEL_GYRO_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "fw_pipe.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

#define SENSOR_ACCEL_GYRO_INTERFACE_EVT \
    ADD_EVT(SENSOR_ACCEL_GYRO_START_REQ) \
    ADD_EVT(SENSOR_ACCEL_GYRO_START_CFM) \
    ADD_EVT(SENSOR_ACCEL_GYRO_STOP_REQ) \
    ADD_EVT(SENSOR_ACCEL_GYRO_STOP_CFM) \
    ADD_EVT(SENSOR_ACCEL_GYRO_ON_REQ) \
    ADD_EVT(SENSOR_ACCEL_GYRO_ON_CFM) \
    ADD_EVT(SENSOR_ACCEL_GYRO_OFF_REQ) \
    ADD_EVT(SENSOR_ACCEL_GYRO_OFF_CFM)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    SENSOR_ACCEL_GYRO_INTERFACE_EVT_START = INTERFACE_EVT_START(SENSOR_ACCEL_GYRO),
    SENSOR_ACCEL_GYRO_INTERFACE_EVT
};

enum {
    SENSOR_ACCEL_GYRO_REASON_UNSPEC = 0,
};

// Data types used in sensor events.
class AccelGyroReport
{
public:
  AccelGyroReport(int32_t aX = 0, int32_t aY = 0, int32_t aZ = 0, int32_t gX = 0, int32_t gY = 0, int32_t gZ = 0) :
      m_aX(aX), m_aY(aY), m_aZ(aZ), m_gX(gX), m_gY(gY), m_gZ(gZ) {}
  int32_t m_aX;
  int32_t m_aY;
  int32_t m_aZ;
  int32_t m_gX;
  int32_t m_gY;
  int32_t m_gZ;
};

typedef Pipe<AccelGyroReport> AccelGyroPipe;


class SensorAccelGyroStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 200
    };
    SensorAccelGyroStartReq() :
        Evt(SENSOR_ACCEL_GYRO_START_REQ) {}
};

class SensorAccelGyroStartCfm : public ErrorEvt {
public:
    SensorAccelGyroStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(SENSOR_ACCEL_GYRO_START_CFM, error, origin, reason) {}
};

class SensorAccelGyroStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 200
    };
    SensorAccelGyroStopReq() :
        Evt(SENSOR_ACCEL_GYRO_STOP_REQ) {}
};

class SensorAccelGyroStopCfm : public ErrorEvt {
public:
    SensorAccelGyroStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(SENSOR_ACCEL_GYRO_STOP_CFM, error, origin, reason) {}
};

class SensorAccelGyroOnReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    // @todo - Add configuration parameters, e.g. ODR.
    SensorAccelGyroOnReq(AccelGyroPipe *pipe) :
        Evt(SENSOR_ACCEL_GYRO_ON_REQ), m_pipe(pipe) {}
    AccelGyroPipe *GetPipe() const { return m_pipe; }
private:
    AccelGyroPipe *m_pipe;

};

class SensorAccelGyroOnCfm : public ErrorEvt {
public:
    SensorAccelGyroOnCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(SENSOR_ACCEL_GYRO_ON_CFM, error, origin, reason) {}
};

class SensorAccelGyroOffReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    // @todo - Add configuration parameters, e.g. ODR.
    SensorAccelGyroOffReq() :
        Evt(SENSOR_ACCEL_GYRO_OFF_REQ) {}
};

class SensorAccelGyroOffCfm : public ErrorEvt {
public:
    SensorAccelGyroOffCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(SENSOR_ACCEL_GYRO_OFF_CFM, error, origin, reason) {}
};

} // namespace APP

#endif // SENSOR_ACCEL_GYRO_INTERFACE_H
