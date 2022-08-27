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

#ifndef CMD_INPUT_H
#define CMD_INPUT_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "fw_strbuf.h"
#include "fw_histbuf.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

class Console;

class CmdInput : public Region {
public:
    enum {
        MAX_LEN = 64
    };
    CmdInput(Hsmn hsmn, char const *name, Console &console);
    bool IsCmdReady() { return isIn(Q_STATE_CAST(&CmdInput::CmdReady)); }
    StrBuf<MAX_LEN>& GetCmd() { return m_cmdHistory.EditBuf(); }

protected:
    static QState InitialPseudoState(CmdInput * const me, QEvt const * const e);
    static QState Root(CmdInput * const me, QEvt const * const e);
        static QState Stopped(CmdInput * const me, QEvt const * const e);
        static QState Started(CmdInput * const me, QEvt const * const e);
            static QState Edit(CmdInput * const me, QEvt const * const e);
                static QState Empty(CmdInput * const me, QEvt const * const e);
                static QState Partial(CmdInput * const me, QEvt const * const e);
                static QState Full(CmdInput * const me, QEvt const * const e);
            static QState Escaped(CmdInput * const me, QEvt const * const e);
            static QState Browsing(CmdInput * const me, QEvt const * const e);
            static QState CmdReady(CmdInput * const me, QEvt const * const e);

    static char const BS_SP_BS[];
    static char const CR_LF[];
    static char const UP1[];
    static char const UP2[];
    static char const DOWN1[];
    static char const DOWN2[];
    bool IsUp();
    bool IsDown();

    Console &m_console;         // Reference to container Console object.
    StrBuf<3> m_escSeqBuf;
    HistBuf<MAX_LEN> m_cmdHistory;

    Timer m_stateTimer;

    enum {
        STATE_TIMER = TIMER_EVT_START(CMD_INPUT),
    };

    enum {
        UPDATE = INTERNAL_EVT_START(CMD_INPUT),
        UP_KEY,
        DOWN_KEY,
        ESC_DONE,
    };
};

} // namespace APP

#endif // CMD_INPUT_H
