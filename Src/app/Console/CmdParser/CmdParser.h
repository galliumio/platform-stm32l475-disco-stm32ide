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

#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

class CmdParser : public Region {
public:
    CmdParser(Hsmn hsmn, char const *name);

protected:
    static QState InitialPseudoState(CmdParser * const me, QEvt const * const e);
    static QState Root(CmdParser * const me, QEvt const * const e);
        static QState Stopped(CmdParser * const me, QEvt const * const e);
        static QState Started(CmdParser * const me, QEvt const * const e);
            static QState Delimiter(CmdParser * const me, QEvt const * const e);
            static QState SimpleArg(CmdParser * const me, QEvt const * const e);
            static QState QuotedArg(CmdParser * const me, QEvt const * const e);
            static QState Done(CmdParser * const me, QEvt const * const e);

    char *m_cmdStr;     // Pointer to command string passed in via CMD_PARSER_START_REQ.
                        // It will be modified by this object.
    char const **m_argv;
    uint32_t *m_argc;
    uint32_t m_maxArgc;
    int32_t m_index;    // Current index to m_cmdStr. Must be signed since it is initialized to -1.

    Timer m_stateTimer;

    enum {
        STATE_TIMER = TIMER_EVT_START(CMD_PARSER),
    };

    enum {
        DONE = INTERNAL_EVT_START(CMD_PARSER),
        STEP,
        CH_DELIM,
        CH_QUOTE,
        CH_SLASH,
        CH_OTHER,
    };
};

} // namespace APP

#endif // CMD_PARSER_H
