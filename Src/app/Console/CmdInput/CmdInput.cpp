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

#include <ctype.h>
#include <string.h>
#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "fw_macro.h"
#include "Console.h"
#include "CmdInputInterface.h"
#include "CmdInput.h"

FW_DEFINE_THIS_FILE("CmdInput.cpp")

namespace APP {

static char const * const timerEvtName[] = {
    "STATE_TIMER",
};

static char const * const internalEvtName[] = {
    "UPDATE",
    "UP_KEY",
    "DOWN_KEY",
    "ESC_DONE",
};

static char const * const interfaceEvtName[] = {
    "CMD_INPUT_START_REQ",
    "CMD_INPUT_STOP_REQ",
    "CMD_INPUT_CHAR_REQ",
};

char const CmdInput::BS_SP_BS[]  = { BS, SP, BS, 0 };
char const CmdInput::CR_LF[]     = { CR, LF, 0 };
char const CmdInput::UP1[]   = { 0x41, 0x00 };
char const CmdInput::UP2[]   = { 0x5B, 0x41, 0x00 };
char const CmdInput::DOWN1[] = { 0x42, 0x00 };
char const CmdInput::DOWN2[] = { 0x5B, 0x42, 0x00 };

bool CmdInput::IsUp() {
    char const *str = m_escSeqBuf.GetRawConst();
    return (STRING_EQUAL(str, UP1) || STRING_EQUAL(str, UP2));
}
bool CmdInput::IsDown() {
    char const *str = m_escSeqBuf.GetRawConst();
    return (STRING_EQUAL(str, DOWN1) || STRING_EQUAL(str, DOWN2));
}

CmdInput::CmdInput(Hsmn hsmn, char const *name, Console &console) :
    Region((QStateHandler)&CmdInput::InitialPseudoState, hsmn, name),
    m_console(console), m_stateTimer(GetHsmn(), STATE_TIMER) {
    SET_EVT_NAME(CMD_INPUT);
}

QState CmdInput::InitialPseudoState(CmdInput * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&CmdInput::Root);
}

QState CmdInput::Root(CmdInput * const me, QEvt const * const e) {
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
            return Q_TRAN(&CmdInput::Stopped);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState CmdInput::Stopped(CmdInput * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CMD_INPUT_START_REQ: {
            EVENT(e);
            me->m_cmdHistory.Reset();
            return Q_TRAN(&CmdInput::Started);
        }
    }
    return Q_SUPER(&CmdInput::Root);
}

QState CmdInput::Started(CmdInput * const me, QEvt const * const e) {
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
            return Q_TRAN(&CmdInput::Edit);
        }
        case CMD_INPUT_STOP_REQ: {
            EVENT(e);
            return Q_TRAN(&CmdInput::Stopped);
        }
        case CMD_INPUT_CHAR_REQ: {
            CmdInputCharReq const &req = static_cast<CmdInputCharReq const &>(*e);
            if (req.GetCh() == ESC) {
                return Q_TRAN(&CmdInput::Escaped);
            }
            // If guard is false, passes event to super state.
            break;
        }
    }
    return Q_SUPER(&CmdInput::Root);
}

QState CmdInput::Edit(CmdInput * const me, QEvt const * const e) {
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
            if (me->m_cmdHistory.EditBuf().IsEmpty()) {
                return Q_TRAN(&CmdInput::Empty);
            } else if (me->m_cmdHistory.EditBuf().IsFull()) {
                return Q_TRAN(&CmdInput::Full);
            }
            return Q_TRAN(&CmdInput::Partial);
        }
        case UPDATE: {
            return Q_TRAN(&CmdInput::Edit);
        }
        case CMD_INPUT_CHAR_REQ: {
            CmdInputCharReq const &req = static_cast<CmdInputCharReq const &>(*e);
            char c = req.GetCh();
            LOG("c = %x", c);
            if (isprint(c)) {
                me->m_console.PutChar(c);
                me->m_cmdHistory.EditBuf().Append(c);
                me->Raise(new Evt(UPDATE));
                return Q_HANDLED();
            } else if ((c == BS) || (c == DEL)) {
                me->m_console.PutStr(BS_SP_BS);
                me->m_cmdHistory.EditBuf().DelLast();
                me->Raise(new Evt(UPDATE));
                return Q_HANDLED();
            } else if ((c == CR) || (c == LF)) {
                me->m_console.PutStr(CR_LF);
                return Q_TRAN(&CmdInput::CmdReady);
            }
            // If guard is false, passes event to super state.
            break;
        }
    }
    return Q_SUPER(&CmdInput::Started);
}

QState CmdInput::Empty(CmdInput * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CMD_INPUT_CHAR_REQ: {
            CmdInputCharReq const &req = static_cast<CmdInputCharReq const &>(*e);
            char c = req.GetCh();
            if ((c == BS) || (c == DEL)) {
                // Discard character.
                return Q_HANDLED();
            }
            // If guard is false, passes event to super state.
            break;
        }
    }
    return Q_SUPER(&CmdInput::Edit);
}

QState CmdInput::Partial(CmdInput * const me, QEvt const * const e) {
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
    return Q_SUPER(&CmdInput::Edit);
}

QState CmdInput::Full(CmdInput * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CMD_INPUT_CHAR_REQ: {
            CmdInputCharReq const &req = static_cast<CmdInputCharReq const &>(*e);
            char c = req.GetCh();
            if (isprint(c)) {
                // Discard character.
                return Q_HANDLED();
            }
            // If guard is false, passes event to super state.
            break;
        }
    }
    return Q_SUPER(&CmdInput::Edit);
}

QState CmdInput::Escaped(CmdInput * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_escSeqBuf.Clear();
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CMD_INPUT_CHAR_REQ: {
            CmdInputCharReq const &req = static_cast<CmdInputCharReq const &>(*e);
            char c = req.GetCh();
            if (isprint(c)) {
                me->m_escSeqBuf.Append(c);
                if (me->IsUp()) {
                    me->Raise(new Evt(UP_KEY));
                } else if (me->IsDown()) {
                    me->Raise(new Evt(DOWN_KEY));
                } else if (me->m_escSeqBuf.IsFull()) {
                    me->Raise(new Evt(ESC_DONE));
                }
                return Q_HANDLED();
            } else if ((c == CR) || (c == LF)) {
                me->Raise(new Evt(ESC_DONE));
                return Q_HANDLED();
            }
            // If guard is false, passes event to super state.
            break;
        }
        case ESC_DONE: {
            me->m_console.PutStrOver(me->m_cmdHistory.EditBuf().GetRaw(), me->m_cmdHistory.BrowseBuf().GetLen());
            me->m_cmdHistory.ResetBrowseIndex();
            return Q_TRAN(&CmdInput::Edit);
        }
        case UP_KEY: {
            uint32_t oldLen = 0;
            if (me->m_cmdHistory.Up(oldLen)) {
                me->m_console.PutStrOver(me->m_cmdHistory.BrowseBuf().GetRaw(), oldLen);
            }
            return Q_TRAN(&CmdInput::Browsing);
        }
        case DOWN_KEY: {
            uint32_t oldLen = 0;
            if (me->m_cmdHistory.Down(oldLen)) {
                me->m_console.PutStrOver(me->m_cmdHistory.BrowseBuf().GetRaw(), oldLen);
            }
            return Q_TRAN(&CmdInput::Browsing);
        }
    }
    return Q_SUPER(&CmdInput::Started);
}

QState CmdInput::Browsing(CmdInput * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CMD_INPUT_CHAR_REQ: {
            CmdInputCharReq const &req = static_cast<CmdInputCharReq const &>(*e);
            char c = req.GetCh();
            if (c != ESC) {
                me->m_cmdHistory.CopyBrowseToEdit();
                me->Raise(new CmdInputCharReq(c));
                return Q_TRAN(&CmdInput::Edit);
            }
            // If guard is false, passes event to super state.
            break;
        }
    }
    return Q_SUPER(&CmdInput::Started);
}

QState CmdInput::CmdReady(CmdInput * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CMD_INPUT_CHAR_REQ: {
            CmdInputCharReq const &req = static_cast<CmdInputCharReq const &>(*e);
            me->m_cmdHistory.CommitEdit();
            me->Raise(new CmdInputCharReq(req.GetCh()));
            return Q_TRAN(&CmdInput::Edit);
        }
    }
    return Q_SUPER(&CmdInput::Started);
}

/*
QState CmdInput::MyState(CmdInput * const me, QEvt const * const e) {
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
            return Q_TRAN(&CmdInput::SubState);
        }
    }
    return Q_SUPER(&CmdInput::SuperState);
}
*/

} // namespace APP

