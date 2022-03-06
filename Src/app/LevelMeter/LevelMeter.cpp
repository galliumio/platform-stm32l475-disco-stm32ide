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

#include <stdio.h>
#include <math.h>
#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "DispInterface.h"
#include "SensorInterface.h"
//#include "WifiInterface.h"
#include "LevelMeterInterface.h"
#include "SensorMsgInterface.h"
#include "LevelMeter.h"

FW_DEFINE_THIS_FILE("LevelMeter.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "LEVEL_METER_TIMER_EVT_START",
    LEVEL_METER_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "LEVEL_METER_INTERNAL_EVT_START",
    LEVEL_METER_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "LEVEL_METER_INTERFACE_EVT_START",
    LEVEL_METER_INTERFACE_EVT
};

LevelMeter::LevelMeter() :
    Active((QStateHandler)&LevelMeter::InitialPseudoState, LEVEL_METER, "LEVEL_METER"),
    m_accelGyroPipe(m_accelGyroStor, ACCEL_GYRO_PIPE_ORDER),
    m_humidTempPipe(m_humidTempStor, HUMID_TEMP_PIPE_ORDER),
    m_pitch(0), m_roll(0), m_pitchThres(90), m_rollThres(90), m_inEvt(QEvt::STATIC_EVT), m_msgSeq(""),
    m_stateTimer(GetHsmn(), STATE_TIMER),
    m_reportTimer(GetHsmn(), REPORT_TIMER) {
    SET_EVT_NAME(LEVEL_METER);
}

QState LevelMeter::InitialPseudoState(LevelMeter * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&LevelMeter::Root);
}

QState LevelMeter::Root(LevelMeter * const me, QEvt const * const e) {
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
            return Q_TRAN(&LevelMeter::Stopped);
        }
        case LEVEL_METER_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LevelMeterStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case LEVEL_METER_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&LevelMeter::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState LevelMeter::Stopped(LevelMeter * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case LEVEL_METER_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new LevelMeterStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case LEVEL_METER_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_inEvt = req;
            return Q_TRAN(&LevelMeter::Starting);
        }
    }
    return Q_SUPER(&LevelMeter::Root);
}

QState LevelMeter::Starting(LevelMeter * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = LevelMeterStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > DispStartReq::TIMEOUT_MS);
            FW_ASSERT(timeout > SensorAccelGyroOnReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new DispStartReq(), ILI9341, true);
            me->SendReq(new SensorAccelGyroOnReq(&me->m_accelGyroPipe), SENSOR_ACCEL_GYRO, false);
            me->SendReq(new SensorHumidTempOnReq(&me->m_humidTempPipe), SENSOR_HUMID_TEMP, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case DISP_START_CFM:
        case SENSOR_ACCEL_GYRO_ON_CFM:
        case SENSOR_HUMID_TEMP_ON_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new LevelMeterStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new LevelMeterStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&LevelMeter::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new LevelMeterStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&LevelMeter::Started);
        }
    }
    return Q_SUPER(&LevelMeter::Root);
}

QState LevelMeter::Stopping(LevelMeter * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = LevelMeterStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > DispStopReq::TIMEOUT_MS);
            FW_ASSERT(timeout > SensorAccelGyroOffReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new DispStopReq(), ILI9341, true);
            me->SendReq(new SensorAccelGyroOffReq(), SENSOR_ACCEL_GYRO, false);
            me->SendReq(new SensorHumidTempOffReq(), SENSOR_HUMID_TEMP, false);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case LEVEL_METER_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case DISP_STOP_CFM:
        case SENSOR_ACCEL_GYRO_OFF_CFM:
        case SENSOR_HUMID_TEMP_OFF_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&LevelMeter::Stopped);
        }
    }
    return Q_SUPER(&LevelMeter::Root);
}

QState LevelMeter::Started(LevelMeter * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_pitch = 0.0;
            me->m_roll = 0.0;
            me->m_pitchThres = 90.0;
            me->m_rollThres = 90.0;
            me->m_humidity = 0.0;
            me->m_temperature = 0.0;
            me->m_reportTimer.Start(REPORT_TIMEOUT_MS, Timer::PERIODIC);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_reportTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&LevelMeter::Normal);
        }
        case REPORT_TIMER: {
            EVENT(e);
            // Reads acceleration and gyroscope data.
            // Default to zero.
            AccelGyroReport report;
            me->m_avgReport = report;
            int32_t count = 0;
            while (me->m_accelGyroPipe.GetUsedCount()) {
                me->m_accelGyroPipe.Read(&report, 1);
                me->m_avgReport.m_aX += report.m_aX;
                me->m_avgReport.m_aY += report.m_aY;
                me->m_avgReport.m_aZ += report.m_aZ;
                count++;
            }
            if (count) {
                me->m_avgReport.m_aX /= count;
                me->m_avgReport.m_aY /= count;
                me->m_avgReport.m_aZ /= count;
            }
            LOG("(count=%d) %d, %d, %d", count, me->m_avgReport.m_aX, me->m_avgReport.m_aY, me->m_avgReport.m_aZ);

            const float PI = 3.14159265;
            float x = me->m_avgReport.m_aX;
            float y = me->m_avgReport.m_aY;
            float z = me->m_avgReport.m_aZ;
            me->m_pitch = atan(x/sqrt((y*y) + (z*z))) * 180/PI;
            me->m_roll  = atan(y/sqrt((x*x) + (z*z))) * 180/PI;
            // Alternative methods.
            /*
            const float G = 1000;
            x = (x > 0) ? LESS(x, G) : GREATER(x, -G);
            y = (y > 0) ? LESS(y, G) : GREATER(y, -G);
            me->m_pitch = asin(x/G) * 180/PI;
            me->m_roll = asin(y/G) * 180/PI;
            */
            LOG("pitch=%06.2f, roll=%06.2f", me->m_pitch, me->m_roll);

            // Reads humidity and temperature data.
            // Since they are slow changing, it's sufficient to save the last values.
            while (me->m_humidTempPipe.GetUsedCount()) {
                HumidTempReport report;
                me->m_humidTempPipe.Read(&report, 1);
                me->m_humidity = report.m_humidity;
                me->m_temperature = report.m_temperature;
            }
            LOG("humid=%f, temp=%f", me->m_humidity, me->m_temperature);

            me->Raise(new Evt(REDRAW));
            // @todo Currently when the destination (to) of a msg is undefined, the server sends to all nodes.
            //       This will be changed to pub-sub in the future.
            me->SendIndMsg(new LevelMeterDataInd(SensorDataIndMsg(me->m_pitch, me->m_roll)), NODE, MSG_UNDEF, true, me->m_msgSeq);
            return Q_HANDLED();
        }
        case LEVEL_METER_CONTROL_REQ: {
            auto const &req = static_cast<LevelMeterControlReq const &>(*e);
            me->m_pitchThres = req.GetPitchThres();
            me->m_rollThres = req.GetRollThres();
            me->Raise(new Evt(REDRAW));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&LevelMeter::Root);
}

QState LevelMeter::Normal(LevelMeter * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case REDRAW: {
            EVENT(e);
            return Q_TRAN(&LevelMeter::Redrawing);
        }
    }
    return Q_SUPER(&LevelMeter::Started);
}

QState LevelMeter::Redrawing(LevelMeter * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->Send(new DispDrawBeginReq(), ILI9341);
            char buf[30];
            snprintf(buf, sizeof(buf), "P= %06.2f", me->m_pitch);
            me->Send(new DispDrawTextReq(buf, 10, 30, COLOR24_BLUE, COLOR24_GREEN, 4), ILI9341);
            snprintf(buf, sizeof(buf), "R= %06.2f", me->m_roll);
            me->Send(new DispDrawTextReq(buf, 10, 90, COLOR24_BLUE, COLOR24_GREEN, 4), ILI9341);

            snprintf(buf, sizeof(buf), "PT= %05.2f", me->m_pitchThres);
            me->Send(new DispDrawTextReq(buf, 10, 150, COLOR24_BLACK, COLOR24_WHITE,4), ILI9341);
            snprintf(buf, sizeof(buf), "RT= %05.2f", me->m_rollThres);
            me->Send(new DispDrawTextReq(buf, 10, 210, COLOR24_BLACK, COLOR24_WHITE,4), ILI9341);

            snprintf(buf, sizeof(buf), "H= %05.2f", me->m_humidity);
            me->Send(new DispDrawTextReq(buf, 10, 280, COLOR24_DARK_GRAY, COLOR24_WHITE,2), ILI9341);
            snprintf(buf, sizeof(buf), "T= %05.2f", me->m_temperature);
            me->Send(new DispDrawTextReq(buf, 120, 280, COLOR24_DARK_GRAY, COLOR24_WHITE,2), ILI9341);
            me->Send(new DispDrawEndReq(), ILI9341);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DISP_DRAW_END_CFM: {
            EVENT(e);
            return Q_TRAN(&LevelMeter::Normal);
        }
        case REDRAW: {
            EVENT(e);
            LOG("Discarded");
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&LevelMeter::Started);
}

/*
QState LevelMeter::MyState(LevelMeter * const me, QEvt const * const e) {
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
            return Q_TRAN(&LevelMeter::SubState);
        }
    }
    return Q_SUPER(&LevelMeter::SuperState);
}
*/

} // namespace APP
