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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "CmdParserInterface.h"
#include "CmdParser.h"

FW_DEFINE_THIS_FILE("CmdParser.cpp")

namespace APP {

static char const * const timerEvtName[] = {
    "STATE_TIMER",
};

static char const * const internalEvtName[] = {
    "DONE",
    "STEP",
    "CH_DELIM",
    "CH_QUOTE",
    "CH_SLASH",
    "CH_OTHER"
};

static char const * const interfaceEvtName[] = {
    "CMD_PARSER_START_REQ",
    "CMD_PARSER_STOP_REQ",
};

CmdParser::CmdParser(Hsmn hsmn, char const *name) :
    Region((QStateHandler)&CmdParser::InitialPseudoState, hsmn, name),
    m_cmdStr(NULL), m_argv(NULL), m_argc(0), m_maxArgc(0), m_index(-1),
    m_stateTimer(GetHsmn(), STATE_TIMER) {
    SET_EVT_NAME(CMD_PARSER);
}

QState CmdParser::InitialPseudoState(CmdParser * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&CmdParser::Root);
}

QState CmdParser::Root(CmdParser * const me, QEvt const * const e) {
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
            return Q_TRAN(&CmdParser::Stopped);
        }
        case CMD_PARSER_START_REQ: {
            EVENT(e);
            CmdParserStartReq const &req = static_cast<CmdParserStartReq const &>(*e);
            me->m_cmdStr = req.GetCmdStr();
            me->m_argv = req.GetArgv();
            me->m_argc = req.GetArgc();
            me->m_maxArgc = req.GetMaxArgc();
            me->m_index = -1;
            *me->m_argc = 0;
            me->Raise(new Evt(STEP));
            return Q_TRAN(&CmdParser::Started);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState CmdParser::Stopped(CmdParser * const me, QEvt const * const e) {
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
    return Q_SUPER(&CmdParser::Root);
}

QState CmdParser::Started(CmdParser * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CMD_PARSER_STOP_REQ: {
            EVENT(e);
            return Q_TRAN(&CmdParser::Stopped);
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&CmdParser::Delimiter);
        }
        case STEP: {
            char ch = me->m_cmdStr[++me->m_index];
            if ((*me->m_argc >= me->m_maxArgc) or (ch == 0)) {
                me->Raise(new Evt(DONE));
            } else {
                if (ch == SP || ch == TAB) {
                    me->Raise(new Evt(CH_DELIM));
                } else if (ch == QUOTE) {
                    me->Raise(new Evt(CH_QUOTE));
                } else if (ch == SLASH) {
                    me->Raise(new Evt(CH_SLASH));
                } else {
                    me->Raise(new Evt(CH_OTHER));
                }
                me->Raise(new Evt(STEP));
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&CmdParser::Root);
}

QState CmdParser::Delimiter(CmdParser * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CH_SLASH:
        case CH_OTHER: {
            return Q_TRAN(&CmdParser::SimpleArg);
        }
        case CH_QUOTE: {
            return Q_TRAN(&CmdParser::QuotedArg);
        }
    }
    return Q_SUPER(&CmdParser::Started);
}

QState CmdParser::SimpleArg(CmdParser * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_argv[(*me->m_argc)++] = &me->m_cmdStr[me->m_index];
            return Q_HANDLED();
        }
        case CH_DELIM: {
            me->m_cmdStr[me->m_index] = 0;
            return Q_TRAN(&CmdParser::Delimiter);
        }
        case CH_QUOTE: {
            me->m_cmdStr[me->m_index] = 0;
            return Q_TRAN(&CmdParser::QuotedArg);
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&CmdParser::Started);
}

QState CmdParser::QuotedArg(CmdParser * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_argv[(*me->m_argc)++] = &me->m_cmdStr[me->m_index + 1];
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case CH_QUOTE: {
            me->m_cmdStr[me->m_index] = 0;
            return Q_TRAN(&CmdParser::Delimiter);
        }
        case CH_SLASH: {
            if (me->m_cmdStr[me->m_index + 1]) {
                me->m_index++;
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&CmdParser::Started);
}

QState CmdParser::Done(CmdParser * const me, QEvt const * const e) {
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
    return Q_SUPER(&CmdParser::Started);
}

/*
QState CmdParser::MyState(CmdParser * const me, QEvt const * const e) {
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
            return Q_TRAN(&CmdParser::SubState);
        }
    }
    return Q_SUPER(&CmdParser::SuperState);
}
*/

} // namespace APP

