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

#include <stdarg.h>
#include <stdio.h>
#include "bsp.h"
#include "qpcpp.h"
#include "fw_hsm.h"
#include "fw_pipe.h"
#include "fw_log.h"
#include "fw.h"
#include "fw_assert.h"

FW_DEFINE_THIS_FILE("fw_log.cpp")

using namespace QP;

namespace FW {

uint8_t Log::m_verbosity = DEFAULT_VERBOSITY;
uint32_t Log::m_onStor[ROUND_UP_DIV(MAX_HSM_COUNT, 32)];
Bitset Log::m_on(m_onStor, ARRAY_COUNT(m_onStor), MAX_HSM_COUNT);
Log::HsmnInf Log::m_hsmnInfStor[MAX_HSM_COUNT];
Log::HsmnInfMap Log::m_hsmnInfMap(m_hsmnInfStor, ARRAY_COUNT(m_hsmnInfStor), HsmnInf(HSM_UNDEF, Inf(NULL, 0, false)));

char const * const Log::m_typeName[NUM_TYPE] = {
    "<ERROR>",
    "<WARNING>",
    "<CRITICAL>",
    "",
    ""
};
char const Log::m_truncatedError[] = "<##TRUN##>";

QSignal const Log::m_entrySig = 1;
char const * const Log::m_builtinEvtName[] = {
    "NULL",
    "ENTRY",
    "EXIT",
    "INIT"
};
char const Log::m_undefName[] = "UNDEF";

Log::EvtSetStor Log::GetEvtSetStor() {
    // Event names for each HSM. Only the 1st HSM of an event interface is actually used.
    static EvtSet evtSetStor[MAX_HSM_COUNT];
    return &evtSetStor;
}

// Sets up event names for the specified event interface.
// @param evtHsmn - HSMN of the event interface for which event names are being set up.
//                  evtHsmn forms the upper bits of an event signal. It is retrieved from
//                  a signal via the macro GET_EVT_HSMN(signal_).
// This function is to be called in the constructors of Active or Region objects.
// By design there is no "Unset/Clear" function. All event names are to be set up
// to the framework during system initialization (before main()).
// As a result there is no need to implement critical sections in other API functions using
// evtSetStor.
void Log::SetEvtName(Hsmn evtHsmn, EvtName timerEvtName, EvtCount timerEvtCount,
                    EvtName internalEvtName, EvtCount internalEvtCount,
                    EvtName interfaceEvtName, EvtCount interfaceEvtCount) {
    EvtSetStor stor = GetEvtSetStor();
    FW_ASSERT(stor && (evtHsmn < ARRAY_COUNT(*stor)));
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    (*stor)[evtHsmn].Init(timerEvtName, timerEvtCount, internalEvtName, internalEvtCount, interfaceEvtName, interfaceEvtCount);
    QF_CRIT_EXIT(crit);
}

void Log::SetEvtName(Hsmn evtHsmn, EvtSet::Category cat, EvtName evtName, EvtCount evtCount) {
    EvtSetStor stor = GetEvtSetStor();
    FW_ASSERT(stor && (evtHsmn < ARRAY_COUNT(*stor)));
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    (*stor)[evtHsmn].Init(cat, evtName, evtCount);
    QF_CRIT_EXIT(crit);
}

char const *Log::GetEvtName(QSignal signal) {
    if (signal < Q_USER_SIG) {
        return GetBuiltinEvtName(signal);
    }
    EvtSetStor stor = GetEvtSetStor();
    Hsmn hsmn = GET_EVT_HSMN(signal);
    FW_ASSERT(stor && (hsmn < ARRAY_COUNT(*stor)));
    EvtSet const &evtSet = (*stor)[hsmn];
    return evtSet.Get(signal);
}

char const *Log::GetBuiltinEvtName(QP::QSignal signal) {
    if (signal < Q_USER_SIG) {
        return m_builtinEvtName[signal];
    }
    return GetUndefName();
}

void Log::AddInterface(Hsmn infHsmn, Fifo *fifo, QSignal sig, bool isDefault) {
    FW_LOG_ASSERT((infHsmn != HSM_UNDEF) && fifo && sig);
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_hsmnInfMap.Save(HsmnInf(infHsmn, Inf(fifo, sig, isDefault)));
    QF_CRIT_EXIT(crit);
}

void Log::RemoveInterface(Hsmn infHsmn) {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_hsmnInfMap.ClearByKey(infHsmn);
    QF_CRIT_EXIT(crit);
}

// @param infHsmn - HSMN of the interface object to write to.
//                  If it is HSM_UNDEF, it writes to all "default" interfaces (default mode).
//                  In default mode when a FIFO is full, the message is truncated.
// @param buf - Pointer to byte buffer.
// @param len - Length in bytes.
// @return Number of bytes written. In default mode, it is always equal to len, even when message is truncated.
//         Otherwise, it is the actual number of bytes written. When FIFO is full it returns 0.
uint32_t Log::Write(Hsmn infHsmn, char const *buf, uint32_t len) {
    if (infHsmn == HSM_UNDEF) {
        WriteDefault(buf, len);
        return len;
    }
    // Write to specific interface.
    uint32_t result = 0;
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    HsmnInf *kv = m_hsmnInfMap.GetByKey(infHsmn);
    if (kv) {
        // A matching interface is found.
        Inf const &inf = kv->GetValue();
        Fifo *fifo = inf.GetFifo();
        FW_ASSERT(fifo);
        bool status = false;
        result = fifo->WriteNoCrit(reinterpret_cast<uint8_t const *>(buf), len, &status);
        QF_CRIT_EXIT(crit);
        // Post MUST be outside critical section.
        if (status) {
            QSignal sig = inf.GetSig();
            FW_ASSERT(sig);
            Fw::Post(new Evt(sig, infHsmn));
        }
    } else {
        QF_CRIT_EXIT(crit);
    }
    return result;
}

// @description Writes to all "default" interfaces. If a FIFO is full, the message is truncated.
// @param buf - Pointer to byte buffer.
// @param len - Length in bytes.
void Log::WriteDefault(char const *buf, uint32_t len) {
    uint32_t writeCount = 0;
    uint32_t index = m_hsmnInfMap.GetTotalCount();
    while (index--) {
        HsmnInf *kv = m_hsmnInfMap.GetByIndex(index);
        // Maintain critical section within loop to reduce interrupt latency.
        // Okay to miss a new entry after passing its index.
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        Hsmn infHsmn = kv->GetKey();
        if ((infHsmn != HSM_UNDEF) && (kv->GetValue().IsDefault())) {
            Fifo *fifo = kv->GetValue().GetFifo();
            FW_ASSERT(fifo);
            bool status1 = false;
            bool status2 = false;
            if (fifo->IsTruncated()) {
                fifo->WriteNoCrit(reinterpret_cast<uint8_t const *>(m_truncatedError), CONST_STRING_LEN(m_truncatedError), &status1);
            }
            if (!fifo->IsTruncated()) {
                fifo->WriteNoCrit(reinterpret_cast<uint8_t const *>(buf), len, &status2);
            }
            QF_CRIT_EXIT(crit);
            // Post MUST be outside critical section.
            if (status1 || status2) {
                QSignal sig = kv->GetValue().GetSig();
                FW_ASSERT(sig);
                Fw::Post(new Evt(sig, infHsmn));
            }
            writeCount++;
        } else {
            QF_CRIT_EXIT(crit);
        }
    }
    if (writeCount == 0) {
        // Gallium - If no FIFO has been setup, write to BSP usart directly.
        BspWrite(buf, len);
    }
}

uint32_t Log::PutChar(Hsmn infHsmn, char c) {
    return Write(infHsmn, &c, 1);
}

uint32_t Log::PutCharN(Hsmn infHsmn, char c, uint32_t count) {
    char buf[BUF_LEN];
    FW_ASSERT(count <= BUF_LEN);
    memset(buf, c, count);
    return Write(infHsmn, buf, count);
}

uint32_t Log::PutStr(Hsmn infHsmn, char const *str) {
    FW_ASSERT(str);
    return Write(infHsmn, str, strlen(str));
}

void Log::PutStrOver(Hsmn infHsmn, char const *str, uint32_t oldLen) {
    FW_ASSERT(str);
    PutCharN(infHsmn, BS, oldLen);
    uint32_t newLen = PutStr(infHsmn, str);
    if (oldLen > newLen) {
        PutCharN(infHsmn, SP, oldLen - newLen);
        PutCharN(infHsmn, BS, oldLen - newLen);
    }
}

uint32_t Log::Print(Hsmn infHsmn, char const *format, ...) {
    va_list arg;
    va_start(arg, format);
    char buf[BUF_LEN];
    uint32_t len = vsnprintf(buf, sizeof(buf), format, arg);
    va_end(arg);
    len = LESS(len, sizeof(buf) - 1);
    return Write(infHsmn, buf, len);
}

uint32_t Log::PrintItem(Hsmn infHsmn, uint32_t index, uint32_t minWidth, uint32_t itemPerLine, char const *format, ...) {
    va_list arg;
    va_start(arg, format);
    char buf[BUF_LEN];
    // Reserve 2 bytes for newline.
    const uint32_t MAX_LEN = sizeof(buf) - 2;
    uint32_t len = vsnprintf(buf, MAX_LEN, format, arg);
    va_end(arg);
    len = LESS(len, (MAX_LEN - 1));
    if (len < (MAX_LEN - 1)) {
        uint32_t paddedLen = ROUND_UP_DIV(len, minWidth) * minWidth;
        paddedLen = LESS(paddedLen, MAX_LEN - 1);
        FW_ASSERT((paddedLen >= len) && (paddedLen <= (MAX_LEN - 1)));
        memset(&buf[len], ' ', paddedLen - len);
        len = paddedLen;
        buf[len] = 0;
    }
    if (((index + 1) % itemPerLine) == 0) {
        FW_ASSERT(len <= (sizeof(buf) - 3));
        buf[len++] = '\n';
        buf[len++] = '\r';
        buf[len] = 0;
    }
    return Write(infHsmn, buf, len);
}

// @param buf - Buffer to store the output string.
// @param bufSize - Size of buf in bytes.
// @param float - Floating-point number to convert to string. Its value must fit in a 32-bit integer.
// @param totalWidth - Total width (minimum) of output string including decimal point and decimal places (not including null-termination).
// @param decimalPlaces - No. of decimal places (limited to the range [1..6])
void Log::FloatToStr(char *buf, uint32_t bufSize, float v, uint32_t totalWidth, uint32_t decimalPlaces) {
    FW_ASSERT(buf && bufSize);
    const uint32_t FACTOR[] = {
        1, 10, 100, 1000, 10000, 100000, 1000000
    };
    decimalPlaces = GREATER(decimalPlaces, 1);
    decimalPlaces = LESS(decimalPlaces, 6);
    FW_ASSERT(decimalPlaces < ARRAY_COUNT(FACTOR));
    uint32_t factor = FACTOR[decimalPlaces];
    int64_t d = static_cast<int64_t>(v * factor + ((v >= 0) ? 0.5 : -0.5));
    d = (d >= 0) ? d % factor : (-d) % factor;
    // totalWidth includes the decimal point. The following ensures the integral width is >= 1.
    totalWidth = GREATER(totalWidth, decimalPlaces + 2);
    int intWidth = totalWidth - decimalPlaces - 1;
    if ((static_cast<int32_t>(v) == 0) && (v < 0)) {
        // The integral part is "-0".
        uint32_t padWidth = GREATER(intWidth - 2, 0);
        padWidth = LESS(padWidth, bufSize - 1);
        for (uint32_t i = 0; i < padWidth; i++) {
            buf[i] = ' ';
        }
        snprintf(buf + padWidth, bufSize - padWidth, "-0.%0*ld", static_cast<int>(decimalPlaces), static_cast<int32_t>(d));

    } else {
        snprintf(buf, bufSize, "%*ld.%0*ld", intWidth, static_cast<int32_t>(v),
                 static_cast<int>(decimalPlaces), static_cast<int32_t>(d));
    }
}

void Log::Event(Type type, Hsmn hsmn, QP::QEvt const *e, char const *func) {
    FW_ASSERT(e && func);
    Hsm *hsm = Fw::GetHsm(hsmn);
    FW_ASSERT(hsm);
    if (e->sig == m_entrySig) {
        hsm->SetState(func);
    }
    if (!IsOutput(type, hsmn)) {
        return;
    }
    if (IS_EVT_HSMN_VALID(e->sig) && !IS_TIMER_EVT(e->sig)) {
        Evt const *evt = static_cast<Evt const *>(e);
        Hsmn from = evt->GetFrom();
        Print(HSM_UNDEF, "%lu %s(%u): %s %s from %s(%d) seq=%d\n\r",
              GetSystemMs(), hsm->GetName(), hsmn, func, GetEvtName(e->sig), GetHsmName(from), from, evt->GetSeq());
    } else {
        Print(HSM_UNDEF, "%lu %s(%u): %s %s\n\r", GetSystemMs(), hsm->GetName(), hsmn, func, GetEvtName(e->sig));
    }
}

void Log::ErrorEvent(Type type, Hsmn hsmn, ErrorEvt const &e, char const *func) {
    FW_ASSERT(func);
    Hsm *hsm = Fw::GetHsm(hsmn);
    FW_ASSERT(hsm);
    if (!IsOutput(type, hsmn)) {
        return;
    }
    Hsmn from = e.GetFrom();
    Hsmn origin = e.GetOrigin();
    Print(HSM_UNDEF, "%lu %s(%u): %s %s from %s(%d) seq=%d error=%d origin=%s(%d) reason=%d\n\r",
          GetSystemMs(), hsm->GetName(), hsmn, func, GetEvtName(e.sig), GetHsmName(from), from, e.GetSeq(),
          e.GetError(), GetHsmName(origin), origin, e.GetReason());
}

void Log::Debug(Type type, Hsmn hsmn, char const *format, ...) {
    Hsm *hsm = Fw::GetHsm(hsmn);
    FW_ASSERT(hsm);
    if (!IsOutput(type, hsmn)) {
        return;
    }
    char buf[BUF_LEN];
    // Reserve 2 bytes for newline.
    const uint32_t MAX_LEN = sizeof(buf) - 2;
    // Note there is no space after type name.
    uint32_t len = snprintf(buf, MAX_LEN, "%lu %s(%u): %s", GetSystemMs(), hsm->GetName(), hsmn, GetTypeName(type));
    len = LESS(len, (MAX_LEN - 1));
    if (len < (MAX_LEN - 1)) {
        va_list arg;
        va_start(arg, format);
        len += vsnprintf(&buf[len], MAX_LEN - len, format, arg);
        va_end(arg);
        len = LESS(len, MAX_LEN - 1);
    }
    FW_ASSERT(len <= (sizeof(buf) - 3));
    buf[len++] = '\n';
    buf[len++] = '\r';
    buf[len] = 0;
    Write(HSM_UNDEF, buf, len);
}

uint32_t Log::PrintBufLine(Hsmn infHsmn, uint8_t const *lineBuf, uint32_t lineLen, uint8_t unit, uint32_t lineLabel) {
    char buf[BUF_LEN];
    // Reserve 2 bytes for newline.
    const uint32_t MAX_LEN = sizeof(buf) - 2;
    // Print label.
    uint32_t len = snprintf(buf, MAX_LEN, "[0x%.8lx] ", lineLabel);
    len = LESS(len, MAX_LEN - 1);
    // Print hex data.
    uint8_t i = 0;
    for (i = 0; i < lineLen; i += unit) {
        if (len < (MAX_LEN - 1)) {
            if (unit == 1) {
                len += snprintf(&buf[len], MAX_LEN - len, "%.2x ", lineBuf[i]);
            } else if (unit == 2) {
                len += snprintf(&buf[len], MAX_LEN - len, "%.4x ", *((uint16_t *)&lineBuf[i]));
            } else {
                len += snprintf(&buf[len], MAX_LEN - len, "%.8lx ", *((uint32_t *)&lineBuf[i]));
            }
            len = LESS(len, MAX_LEN - 1);
        }
    }
    // Print padding.
    if (len < (MAX_LEN - 1)) {
        // Each unit has 2 * unit byte size plus a space.
        uint32_t padding = ((BYTE_PER_LINE - lineLen) / unit) * (unit * 2 + 1);
        padding =  LESS(padding, MAX_LEN - 1 - len);
        FW_ASSERT((len + padding) <= (MAX_LEN - 1));
        memset(&buf[len], ' ', padding);
        len += padding;
        buf[len] = 0;
    }
    // Print ASCII.
    for (i = 0; i < lineLen; i++) {
        if (len < (MAX_LEN - 1)) {
            char ch = lineBuf[i];
            buf[len++] = ((ch >= 0x20) && (ch <= 0x7E)) ? ch : '_';
        }
    }
    buf[len] = 0;
    // Print newline.
    FW_ASSERT(len <= (sizeof(buf) - 3));
    buf[len++] = '\n';
    buf[len++] = '\r';
    buf[len] = 0;
    return Write(infHsmn, buf, len);
}

// @return Number of raw data bytes in dataBuf written. It is NOT the length of the formatted strings written.
uint32_t Log::PrintBuf(Hsmn infHsmn, uint8_t const *dataBuf, uint32_t dataLen, uint8_t unit, uint32_t label) {
    FW_ASSERT((unit == 1) || (unit == 2) || (unit == 4));
    FW_ASSERT(dataBuf && (((uint32_t)dataBuf % unit) == 0) && ((dataLen % unit) == 0) && ((BYTE_PER_LINE % unit) == 0));
    Print(infHsmn, "Buffer 0x%.8x len %lu:\n\r", dataBuf, dataLen);
    uint32_t dataIndex = 0;
    while (dataIndex < dataLen) {
        // Print at most 16 bytes of data in each line.
        uint8_t count = BYTE_PER_LINE;
        if (dataIndex + count > dataLen) {
            count = dataLen - dataIndex;
        }
        if (Log::PrintBufLine(infHsmn, &dataBuf[dataIndex], count, unit, label + dataIndex) == 0) {
            break;
        }
        dataIndex += count;
    }
    FW_ASSERT(dataIndex <= dataLen);
    return dataIndex;
}

void Log::DebugBuf(Type type, Hsmn hsmn, uint8_t const *dataBuf, uint32_t dataLen, uint8_t align, uint32_t label) {
    if (IsOutput(type, hsmn)) {
        PrintBuf(HSM_UNDEF, dataBuf, dataLen, align, label);
    }
}

void Log::On(Hsmn hsmn) {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_on.Set(hsmn);
    QF_CRIT_EXIT(crit);
}

void Log::Off(Hsmn hsmn) {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_on.Clear(hsmn);
    QF_CRIT_EXIT(crit);
}

void Log::OnAll() {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_on.SetAll();
    QF_CRIT_EXIT(crit);
}

void Log::OffAll() {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    m_on.ClearAll();
    QF_CRIT_EXIT(crit);
}

bool Log::IsOutput(Type type, Hsmn hsmn) {
    return (type < m_verbosity) && IsOn(hsmn);
}

// Must allow HSM_UNDEF since the "m_from" hsmn of an event is optional
// (e.g. an internal event or event sent from main).
char const *Log::GetHsmName(Hsmn hsmn) {
    Hsm *hsm = Fw::GetHsm(hsmn);
    if (!hsm) {
        return GetUndefName();
    }
    return hsm->GetName();
}

char const *Log::GetTypeName(Type type) {
    FW_ASSERT(type < NUM_TYPE);
    return m_typeName[type];
}

char const *Log::GetState(Hsmn hsmn) {
    Hsm *hsm = Fw::GetHsm(hsmn);
    if (!hsm) {
        return GetUndefName();
    }
    return hsm->GetState();
}

} // namespace FW
