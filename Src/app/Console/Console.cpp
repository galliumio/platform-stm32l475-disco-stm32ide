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

#include <stdarg.h>
#include <stdio.h>
#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "ConsoleInterface.h"
#include "UartActInterface.h"
#include "UartOutInterface.h"
#include "UartInInterface.h"
#include "UartAct.h"
#include "CmdInputInterface.h"
#include "CmdParserInterface.h"
#include "Console.h"

FW_DEFINE_THIS_FILE("Console.cpp")

namespace APP {

static char const * const timerEvtName[] = {
    "STATE_TIMER",
};

static char const * const internalEvtName[] = {
    "DONE",
    "FAILED",
    "CMD_RECV",
    "RAW_DISABLE",
};

static char const * const interfaceEvtName[] = {
    "CONSOLE_START_REQ",
    "CONSOLE_START_CFM",
    "CONSOLE_STOP_REQ",
    "CONSOLE_STOP_CFM",
    "CONSOLE_CMD_IND",
    "CONSOLE_RAW_ENABLE_REQ",
};

enum {
    OUT_FIFO_ORDER = 13,
    IN_FIFO_ORDER = 10,
};

// UART buffers can be allocated to TCM by uncommenting the section __attribute__ below. (Note TCM is limited to 128K.)
// Make sure the beginning and end of the buffers are aligned to cache line size (32 bytes).
// (Not strictly required for output buffer; Only required for input buffer that needs invalidation.)
uint8_t consoleOutFifoStor[CONSOLE_COUNT][ROUND_UP_32(1 << OUT_FIFO_ORDER)] __attribute__((aligned(32))); // __attribute__ ((section (".data.DTCM")));
uint8_t consoleInFifoStor[CONSOLE_COUNT][ROUND_UP_32(1 << IN_FIFO_ORDER)] __attribute__((aligned(32)));   // __attribute__ ((section (".data.DTCM")));

static uint8_t *GetOutFifoStor(Hsmn hsmn) {
    uint16_t inst = hsmn - CONSOLE;
    FW_ASSERT(inst < ARRAY_COUNT(consoleOutFifoStor));
    return consoleOutFifoStor[inst];
}

static uint8_t *GetInFifoStor(Hsmn hsmn) {
    uint16_t inst = hsmn - CONSOLE;
    FW_ASSERT(inst < ARRAY_COUNT(consoleInFifoStor));
    return consoleInFifoStor[inst];
}

uint32_t Console::PutChar(char c) {
    return Log::PutChar(m_outIfHsmn, c);
}

uint32_t Console::PutCharN(char c, uint32_t count) {
    return Log::PutCharN(m_outIfHsmn, c, count);
}

uint32_t Console::PutStr(char const *str) {
    return Log::PutStr(m_outIfHsmn, str);
}

void Console::PutStrOver(char const *str, uint32_t oldLen) {
    return Log::PutStrOver(m_outIfHsmn, str, oldLen);
}

uint32_t Console::Print(char const *format, ...) {
    va_list arg;
    va_start(arg, format);
    char buf[Log::BUF_LEN];
    uint32_t len = vsnprintf(buf, sizeof(buf), format, arg);
    va_end(arg);
    len = LESS(len, sizeof(buf) - 1);
    return Log::Write(m_outIfHsmn, buf, len);
}

uint32_t Console::PrintItem(uint32_t index, uint32_t minWidth, uint32_t itemPerLine, char const *format, ...) {
    va_list arg;
    va_start(arg, format);
    char buf[Log::BUF_LEN];
    vsnprintf(buf, sizeof(buf), format, arg);
    va_end(arg);
    uint32_t result = Log::PrintItem(m_outIfHsmn, index, minWidth, itemPerLine, "%s", buf);
    return result;
}

uint32_t Console::PrintEvt(QP::QEvt const &e) {
    if (IS_EVT_HSMN_VALID(e.sig) && !IS_TIMER_EVT(e.sig)) {
        Evt const &evt = static_cast<Evt const &>(e);
        Hsmn from = evt.GetFrom();
        return Log::Print(m_outIfHsmn, "%lu Received %s from %s(%d) seq=%d\n\r",
                          GetSystemMs(), Log::GetEvtName(e.sig), Log::GetHsmName(from), from, evt.GetSeq());
    } else {
        return Log::Print(m_outIfHsmn, "%lu Received %s\n\r", GetSystemMs(), Log::GetEvtName(e.sig));
    }
}

uint32_t Console::PrintErrorEvt(ErrorEvt const &e) {
    Hsmn from = e.GetFrom();
    Hsmn origin = e.GetOrigin();
    return Log::Print(m_outIfHsmn, "%lu Received %s from %s(%d) seq=%u error=%u origin=%s(%u) reason=%u\n\r",
                      GetSystemMs(), Log::GetEvtName(e.sig), Log::GetHsmName(from), from, e.GetSeq(),
                      e.GetError(), Log::GetHsmName(origin), origin, e.GetReason());
}

uint32_t Console::PrintMsg(Msg const &m) {
    return Log::Print(m_outIfHsmn, "%lu Received %s to %s from %s seq=%u\n\r",
                      GetSystemMs(), m.GetType(), m.GetTo(), m.GetFrom(), m.GetSeq());
}

uint32_t Console::PrintErrorMsg(ErrorMsg const &m) {
    return Log::Print(m_outIfHsmn, "%lu Received %s to %s from %s seq=%u error=%s origin=%s reason=%s\n\r",
                      GetSystemMs(), m.GetType(), m.GetTo(), m.GetFrom(), m.GetSeq(),
                      m.GetError(), m.GetOrigin(), m.GetReason());
}

uint32_t Console::PrintMsg(MsgEvt const &e) {
    return PrintEvt(e) && PrintMsg(e.GetMsgBase());
}

uint32_t Console::PrintErrorMsg(ErrorMsgEvt const &e) {
    return PrintEvt(e) && PrintErrorMsg(e.GetErrorMsg());
}

// Returns length of formatted string written, which can be > lineLen due to formatting.
uint32_t Console::PrintBufLine(uint8_t const *lineBuf, uint32_t lineLen, uint8_t unit, uint32_t lineLabel) {
    return Log::PrintBufLine(m_outIfHsmn, lineBuf, lineLen, unit, lineLabel);
}

// Returns number of raw data bytes written, which can be <= dataLen.
uint32_t Console::PrintBuf(uint8_t const *dataBuf, uint32_t dataLen, uint8_t align, uint32_t label) {
    return Log::PrintBuf(m_outIfHsmn, dataBuf, dataLen, align, label);
}

// Handles a command stored in an event e according to the command handler table passed in.
// @param e - Contains the command to lookup and associated arguments.
// @param cmd - Command handler table.
// @param cmdCount - Number of entries in cmd.
// @param isRoot - True when cmd is the root/first command handler table used for command lookup.
// @return CMD_DONE when command handling is done (including the case when nothing is to be done).
//         CMD_CONTINUE when further handling is pending (upon further events).
CmdStatus Console::HandleCmd(Evt const *e, CmdHandler const *cmd, uint32_t cmdCount, bool isRoot) {
    FW_ASSERT(e && cmd && cmdCount);
    CmdStatus status = CMD_DONE;
    switch (e->sig) {
        case CONSOLE_CMD: {
            ConsoleCmd const &ind = static_cast<ConsoleCmd const &>(*e);
            uint32_t skip = isRoot ? 0 : 1;
            if (ind.Argc() > skip) {
                status = RunCmd(ind.Argv() + skip, ind.Argc() - skip, cmd, cmdCount);
            }
            break;
        }
    }
    return status;
}

// List all entries in command handler table.
CmdStatus Console::ListCmd(Evt const *e, CmdHandler const *cmd, uint32_t cmdCount) {
    uint32_t &index = Var(0);
    switch (e->sig) {
        case CONSOLE_CMD: {
            PutStr("[Commands]\n\r");
            index = 0;
            return CMD_CONTINUE;
        }
        case UART_OUT_EMPTY_IND: {
            for (; index < cmdCount; index++) {
                bool result = PrintItem(0, 16, 2, "\r%s", cmd[index].key) &&
                              PrintItem(1, 1,  2, "%s", cmd[index].text);
                if (!result) {
                    return CMD_CONTINUE;
                }
            }
            PutStr("\n\r");
            return CMD_DONE;
        }
    }
    return CMD_CONTINUE;
}

uint32_t &Console::Var(uint32_t index) {
    FW_ASSERT(index < MAX_VAR);
    return m_var[index];
}

void Console::Banner() {
    PutStr("\n\r\n\r");
    PutStr("*******************************\n\r");
    PutStr("*   Console STM32L475 Disco  *\n\r");
    PutStr("*******************************\n\r");
}

void Console::Prompt() {
    Print("%d %s> ", GetSystemMs(), GetName());
}

CmdStatus Console::RunCmd(char const **argv, uint32_t argc, CmdHandler const *cmd, uint32_t cmdCount) {
    FW_ASSERT(argv && argc && cmd && cmdCount);
    for (uint32_t i = 0; i < cmdCount; i++) {
        if (STRING_EQUAL(argv[0], cmd[i].key)) {
            // @todo - Superuser check.
            ClearVar();
            m_msgSeq.Reset();
            m_evtSeq.Reset();
            ConsoleCmd evt(GetHsmn(), argv, argc);
            m_lastCmdFunc = cmd[i].func;
            FW_ASSERT(m_lastCmdFunc);
            // Ensures the console timer is stopped before running a new command, in case it is currently running
            // and the new command handler uses it.
            m_consoleTimer.Stop();
            return m_lastCmdFunc(*this, &evt);
        }
    }
    return CMD_DONE;
}

void Console::LastCmdDone() {
    m_lastCmdFunc = NULL;
    m_consoleTimer.Stop();
    Prompt();
}

void Console::RootCmdFunc(Evt const *e) {
    FW_ASSERT(m_rootCmdFunc);
    if (m_rootCmdFunc(*this, e) == CMD_DONE) {
        LastCmdDone();
    }
}

void Console::LastCmdFunc(Evt const *e) {
    if (m_lastCmdFunc) {
        if (m_lastCmdFunc(*this, e) == CMD_DONE) {
            LastCmdDone();
        }
    }
}

Console::Console(Hsmn hsmn, char const *name, char const *cmdInputName, char const *cmdParserName) :
    Active((QStateHandler)&Console::InitialPseudoState, hsmn, name),
    m_ifHsmn(HSM_UNDEF), m_outIfHsmn(HSM_UNDEF), m_isDefault(false), m_inEvt(QEvt::STATIC_EVT),
    m_cmdInput(GetCmdInputHsmn(), cmdInputName, *this),
    m_cmdParser(GetCmdParserHsmn(), cmdParserName),
    m_outFifo(GetOutFifoStor(hsmn), OUT_FIFO_ORDER),
    m_inFifo(GetInFifoStor(hsmn), IN_FIFO_ORDER),
    m_argc(0), m_rootCmdFunc(NULL), m_lastCmdFunc(NULL), m_msgSeq(""), m_evtSeq(HSM_UNDEF),
    m_stateTimer(GetHsmn(), STATE_TIMER),
    m_consoleTimer(GetHsmn(), CONSOLE_TIMER) {
    SET_EVT_NAME(CONSOLE);
    FW_ASSERT((hsmn >= CONSOLE) && (hsmn <= CONSOLE_LAST));
}

QState Console::InitialPseudoState(Console * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Console::Root);
}

QState Console::Root(Console * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_cmdInput.Init(me);
            me->m_cmdParser.Init(me);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Console::Stopped);
        }
        case CONSOLE_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new ConsoleStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case CONSOLE_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Console::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Console::Stopped(Console * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CONSOLE_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new ConsoleStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case CONSOLE_START_REQ: {
            EVENT(e);
            ConsoleStartReq const &req = static_cast<ConsoleStartReq const &>(*e);
            // Remember the assigned interface active object.
            me->m_ifHsmn = req.GetIfHsmn();
            me->m_isDefault = req.IsDefault();
            me->m_rootCmdFunc = req.GetCmdFunc();
            // Make sure the interface type is supported. Add further check here.
            FW_ASSERT(me->IsUart());
            FW_ASSERT(me->m_rootCmdFunc);
            me->m_inEvt = req;
            return Q_TRAN(&Console::Starting);
        }
    }
    return Q_SUPER(&Console::Root);
}

QState Console::Starting(Console * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = ConsoleStartReq::TIMEOUT_MS;
            if (me->IsUart()) {
                FW_ASSERT(timeout > UartActStartReq::TIMEOUT_MS);
                me->SendReq(new UartActStartReq(&me->m_outFifo, &me->m_inFifo), me->m_ifHsmn, true);
            }
            // Add other interface types here.
            me->m_stateTimer.Start(timeout);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        // Add other interface events here.
        case UART_ACT_START_CFM: {
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
                me->SendCfm(new ConsoleStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new ConsoleStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&Console::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new ConsoleStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Console::Started);
        }
    }
    return Q_SUPER(&Console::Root);
}

QState Console::Stopping(Console * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = ConsoleStopReq::TIMEOUT_MS;
            if (me->IsUart()) {
                FW_ASSERT(timeout > UartActStopReq::TIMEOUT_MS);
                me->SendReq(new UartActStopReq(), me->m_ifHsmn, true);
            }
            // Add other interface types here.
            me->m_stateTimer.Start(timeout);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case CONSOLE_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        // Add other interface events here.
        case UART_ACT_STOP_CFM: {
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
            return Q_TRAN(&Console::Stopped);
        }
    }
    return Q_SUPER(&Console::Root);
}

QState Console::Started(Console * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            QSignal writeReqSig = 0;
            if (me->IsUart()) {
                writeReqSig = UART_OUT_WRITE_REQ;
                me->m_outIfHsmn = UartAct::GetUartOutHsmn(me->m_ifHsmn);
            }
            // Add other interface types here.
            FW_ASSERT(writeReqSig);
            Log::AddInterface(me->m_outIfHsmn, &me->m_outFifo, writeReqSig, me->m_isDefault);
            // Start input region.
            CmdInputStartReq req(me->GetCmdInputHsmn(), me->GetHsmn(), QEvt::STATIC_EVT);
            me->m_cmdInput.Dispatch(&req);
            me->Banner();
            me->m_lastCmdFunc = NULL;
            me->Prompt();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            Log::RemoveInterface(me->m_outIfHsmn);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            // Gallium - Go to Interactive state for testing, bypassing Login state.
            return Q_TRAN(&Console::Interactive);
        }
        case UART_IN_DATA_IND: {
            char c;
            // Limits the max no. of characters to process in a loop. Each character is dispatched
            // to CmdInput region synchronously which may send UART_OUT_WRITE_REQ to UartOut.
            // UartOut in return may send UART_OUT_EMPTY_IND to Console. The worst case is there is a
            // UART_OUT_EMPTY_IND per character processed, which may flood its event queue or deplete
            // the small event pool triggering assert. The condition "count--" must come first.
            uint32_t count = CHAR_LOOP_COUNT;
            while(count-- && me->m_inFifo.Read(reinterpret_cast<uint8_t *>(&c), 1)) {
                // Static event to save new and gc. It must not be deferred.
                CmdInputCharReq req(me->GetCmdInputHsmn(), me->GetHsmn(), c, QEvt::STATIC_EVT);
                me->m_cmdInput.Dispatch(&req);
                if (me->m_cmdInput.IsCmdReady()) {
                    me->Raise(new Evt(CMD_RECV));
                    break;
                }
            }
            if (me->m_inFifo.GetUsedCount()) {
                me->Send(new UartInDataInd(), me->GetHsmn());
            }
            return Q_HANDLED();
        }
        default: {
            QSignal sig = e->sig;
            if ((sig == CONSOLE_TIMER) ||
                (IS_EVT_HSMN_VALID(sig) && IS_INTERFACE_EVT(sig) &&
                 (sig != CONSOLE_START_REQ) && (sig != CONSOLE_STOP_REQ) && (sig != UART_IN_DATA_IND))) {
                me->LastCmdFunc(static_cast<Evt const *>(e));
                return Q_HANDLED();
            }
            break;
        }
    }
    return Q_SUPER(&Console::Root);
}


QState Console::Login(Console * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Console::Started);
}

QState Console::Interactive(Console * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CMD_RECV: {
            EVENT(e);
            // Since parser my update cmdStr, we must make a copy to avoid modifying history.
            STRING_COPY(me->m_cmdStr, me->m_cmdInput.GetCmd().GetRawConst(), sizeof(me->m_cmdStr));
            //LOG("cmdstr = %s", me->m_cmdStr);
            // Start parser region.
            CmdParserStartReq req(me->GetCmdParserHsmn(), me->GetHsmn(),
                                  me->m_cmdStr, me->m_argv, &me->m_argc, ARRAY_COUNT(me->m_argv), QEvt::STATIC_EVT);
            me->m_cmdParser.Dispatch(&req);
            //LOG("argc = %d", me->m_argc);
            //for (uint32_t i = 0; i < me->m_argc; i++) {
            //    LOG("argv[%d] = %s", i, me->m_argv[i]);
            //}
            ConsoleCmd ind(me->GetHsmn(), me->m_argv, me->m_argc);
            me->RootCmdFunc(&ind);
            return Q_HANDLED();
        }
        case CONSOLE_RAW_ENABLE_REQ: {
            EVENT(e);
            return Q_TRAN(&Console::Raw);
        }
    }
    return Q_SUPER(&Console::Started);
}

QState Console::Raw(Console * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case RAW_DISABLE: {
            EVENT(e);
            return Q_TRAN(&Console::Interactive);
        }
        case UART_IN_DATA_IND: {
            me->LastCmdFunc(static_cast<Evt const *>(e));
            if (!me->m_lastCmdFunc) {
                me->Raise(new Evt(RAW_DISABLE));
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Console::Started);
}

/*
QState Console::MyState(Console * const me, QEvt const * const e) {
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
            return Q_TRAN(&Console::SubState);
        }
    }
    return Q_SUPER(&Console::SuperState);
}
*/

} // namespace APP
