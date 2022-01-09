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

#ifndef FW_LOG_H
#define FW_LOG_H

#include "fw_def.h"
#include "fw_error.h"
#include "fw_map.h"
#include "fw_pipe.h"
#include "fw_bitset.h"
#include "fw_evtSet.h"
#include "fw_assert.h"

#define FW_LOG_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_log.h", (int_t)__LINE__))

namespace FW {

#define SET_EVT_NAME(evtHsmn_)   Log::SetEvtName(evtHsmn_, timerEvtName, ARRAY_COUNT(timerEvtName), \
                                                 internalEvtName, ARRAY_COUNT(internalEvtName), \
                                                 interfaceEvtName, ARRAY_COUNT(interfaceEvtName))
#define SET_TIMER_EVT_NAME(evtHsmn_)        Log::SetEvtName(evtHsmn_, EvtSet::TIMER_EVT, timerEvtName, ARRAY_COUNT(timerEvtName))
#define SET_INTERNAL_EVT_NAME(evtHsmn_)     Log::SetEvtName(evtHsmn_, EvtSet::INTERNAL_EVT, internalEvtName, ARRAY_COUNT(internalEvtName))
#define SET_INTERFACE_EVT_NAME(evtHsmn_)    Log::SetEvtName(evtHsmn_, EvtSet::INTERFACE_EVT, interfaceEvtName, ARRAY_COUNT(interfaceEvtName))

#define PRINT(format_, ...)      Log::Print(HSM_UNDEF, format_, ## __VA_ARGS__)
// The following macros can only be used within an HSM. Newline is automatically appended.
#define EVENT(e_)                Log::Event(Log::TYPE_LOG, me->GetHsmn(), e_, __FUNCTION__);
#define ERROR_EVENT(e_)          Log::ErrorEvent(Log::TYPE_LOG, me->GetHsmn(), e_, __FUNCTION__);
#define INFO(format_, ...)       Log::Debug(Log::TYPE_INFO, me->GetHsmn(), format_, ## __VA_ARGS__)
#define LOG(format_, ...)        Log::Debug(Log::TYPE_LOG, me->GetHsmn(), format_, ## __VA_ARGS__)
#define CRITICAL(format_, ...)   Log::Debug(Log::TYPE_CRITICAL, me->GetHsmn(), format_, ## __VA_ARGS__)
#define WARNING(format_, ...)    Log::Debug(Log::TYPE_WARNING, me->GetHsmn(), format_, ## __VA_ARGS__)
#define ERROR(format_, ...)      Log::Debug(Log::TYPE_ERROR, me->GetHsmn(), format_, ## __VA_ARGS__)

#define PRINT_BUF(buf_, len_, unit_, label_)    Log::PrintBuf(HSM_UNDEF, buf_, len_, unit_, label_)
// The following macros can only be used within an HSM. Newline is automatically appended.
#define INFO_BUF(buf_, len_, unit_, label_)     Log::DebugBuf(Log::TYPE_INFO, me->GetHsmn(), buf_, len_, unit_, label_)
#define LOG_BUF(buf_, len_, unit_, label_)      Log::DebugBuf(Log::TYPE_LOG, me->GetHsmn(), buf_, len_, unit_, label_)
#define CRITICAL_BUF(buf_, len_, unit_, label_) Log::DebugBuf(Log::TYPE_CRITICAL, me->GetHsmn(), buf_, len_, unit_, label_)
#define WARNING_BUF(buf_, len_, unit_, label_)  Log::DebugBuf(Log::TYPE_WARNING, me->GetHsmn(), buf_, len_, unit_, label_)
#define ERROR_BUF(buf_, len_, unit_, label_)    Log::DebugBuf(Log::TYPE_ERROR, me->GetHsmn(), buf_, len_, unit_, label_)

class Log {
public:
    // Types are listed in decreasing order of criticality.
    // Order must match type names defined in m_typeName.
    enum Type {
        TYPE_ERROR,
        TYPE_WARNING,
        TYPE_CRITICAL,
        TYPE_LOG,
        TYPE_INFO,
        NUM_TYPE
    };

    enum {
        MAX_VERBOSITY = 5,      // 0-5. The higher the value the more verbose it is.
                                // 0 - All debug out disabled.
                                // 1 - Shows ERROR (Type 0).
                                // 2 - Shows ERROR, WARNING (Type 0-1).
                                // 3 - Shows ERROR, WARNING, CRITICAL (Type 0-2).
                                // 4 - Shows ERROR, WARNING, CRITICAL, LOG (Type 0-3).
                                // 5 - Shows ERROR, WARNING, CRITICAL, LOG, INFO (Type 0-4).
        DEFAULT_VERBOSITY = 0
    };

    enum {
        BUF_LEN = 512, //160,
        BYTE_PER_LINE = 16
    };

    // Set event names for an HSM.
    static void SetEvtName(Hsmn evtHsmn, EvtName timerEvtName, EvtCount timerEvtCount,
                           EvtName internalEvtName, EvtCount internalEvtCount,
                           EvtName interfaceEvtName, EvtCount interfaceEvtCount);
    static void SetEvtName(Hsmn evtHsmn, EvtSet::Category cat, EvtName evtName, EvtCount evtCount);
    static char const *GetEvtName(QP::QSignal signal);
    static char const *GetBuiltinEvtName(QP::QSignal signal);
    static char const *GetUndefName() { return m_undefName; }

    // Add and remove output device interface.
    static void AddInterface(Hsmn infHsmn, Fifo *fifo, QP::QSignal sig, bool isDefault);
    static void RemoveInterface(Hsmn infHsmn);

    static uint32_t Write(Hsmn infHsmn, char const *buf, uint32_t len);
    static void WriteDefault(char const *buf, uint32_t len);
    static uint32_t PutChar(Hsmn infHsmn, char c);
    static uint32_t PutCharN(Hsmn infHsmn, char c, uint32_t count);
    static uint32_t PutStr(Hsmn infHsmn, char const *str);
    static void PutStrOver(Hsmn infHsmn, char const *str, uint32_t oldLen);
    static uint32_t Print(Hsmn infHsmn, char const *format, ...);
    static uint32_t PrintItem(Hsmn infHsmn, uint32_t index, uint32_t minWidth, uint32_t itemPerLine, char const *format, ...);
    static void Event(Type type, Hsmn hsmn, QP::QEvt const *e, char const *func);
    static void ErrorEvent(Type type, Hsmn hsmn, ErrorEvt const &e, char const *func);
    static void Debug(Type type, Hsmn hsmn, char const *format, ...);
    static uint32_t PrintBufLine(Hsmn infHsmn, uint8_t const *lineBuf, uint32_t lineLen, uint8_t unit, uint32_t lineLabel);
    static uint32_t PrintBuf(Hsmn infHsmn, uint8_t const *dataBuf, uint32_t dataLen, uint8_t align = 1, uint32_t label = 0);
    static void DebugBuf(Type type, Hsmn hsmn, uint8_t const *dataBuf, uint32_t dataLen, uint8_t align = 1, uint32_t label = 0);

    static uint8_t GetVerbosity() { return m_verbosity; }
    static void SetVerbosity(uint8_t v) {
        FW_LOG_ASSERT(v <= MAX_VERBOSITY);
        m_verbosity = v;
    }
    static void On(Hsmn hsmn);
    static void Off(Hsmn hsmn);
    static void OnAll();
    static void OffAll();
    static bool IsOn(Hsmn hsmn) { return m_on.IsSet(hsmn); }

    static char const *GetHsmName(Hsmn hsmn);
    static char const *GetTypeName(Type type);
    static char const *GetState(Hsmn hsmn);
private:
    // EvtSetStor is a pointer to an array of EvtSet with MAX_HSM_COUNT elements.
    typedef EvtSet (*EvtSetStor)[MAX_HSM_COUNT];
    static EvtSetStor GetEvtSetStor();
    static bool IsOutput(Type type, Hsmn hsmn);

    class Inf {
    public:
        Inf(Fifo *fifo = NULL, QP::QSignal sig = 0, bool isDefault = false) :
            m_fifo(fifo), m_sig(sig), m_isDefault(isDefault) {
        }
        Fifo *GetFifo() const { return m_fifo; }
        QP::QSignal GetSig() const { return m_sig; }
        bool IsDefault() const { return m_isDefault; }
    private:
        Fifo *m_fifo;
        QP::QSignal m_sig;
        bool m_isDefault;   // True if this is a default interface for log output when interface is not specified.
        // Use built-in memberwise copy constructor and assignment operator.
    };

    typedef KeyValue<Hsmn, Inf> HsmnInf;
    typedef Map<Hsmn, Inf> HsmnInfMap;

    static uint8_t m_verbosity;
    static uint32_t m_onStor[ROUND_UP_DIV(MAX_HSM_COUNT, 32)];
    static Bitset m_on;
    static HsmnInf m_hsmnInfStor[MAX_HSM_COUNT];
    static HsmnInfMap m_hsmnInfMap;
    static char const * const m_typeName[NUM_TYPE];
    static char const m_truncatedError[];
    static QP::QSignal const m_entrySig;
    static char const * const m_builtinEvtName[];
    static char const m_undefName[];
};

} // namespace FW

#endif // FW_LOG_H
