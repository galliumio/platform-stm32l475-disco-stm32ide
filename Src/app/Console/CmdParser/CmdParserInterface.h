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

#ifndef CMD_PARSER_INTERFACE_H
#define CMD_PARSER_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

enum {
    CMD_PARSER_START_REQ = INTERFACE_EVT_START(CMD_PARSER),
    CMD_PARSER_STOP_REQ,
};

enum {
    CMD_PARSER_REASON_UNSPEC = 0,
};

class CmdParserStartReq : public Evt {
public:
    CmdParserStartReq(Hsmn to, Hsmn from, char *cmdStr, char const **argv, uint32_t *argc, uint32_t maxArgc, QEvt::StaticEvt /*dummy*/) :
        Evt(CMD_PARSER_START_REQ, to, from, 0, QEvt::STATIC_EVT),
        m_cmdStr(cmdStr), m_argv(argv), m_argc(argc), m_maxArgc(maxArgc) {}

    char *GetCmdStr() const { return m_cmdStr; }
    char const **GetArgv() const { return m_argv; }
    uint32_t *GetArgc() const { return m_argc; }
    uint32_t GetMaxArgc() const { return m_maxArgc; }
private:
    char *m_cmdStr;     // Pointer to the command string buffer.
                        // The memory of this buffer must be valid during the processing of this event.
    char const **m_argv;
    uint32_t *m_argc;
    uint32_t m_maxArgc;
};

class CmdParserStopReq : public Evt {
public:
    CmdParserStopReq(Hsmn to, Hsmn from, QEvt::StaticEvt /*dummy*/) :
        Evt(CMD_PARSER_STOP_REQ, to, from, 0, QEvt::STATIC_EVT) {}
};

} // namespace APP

#endif // CMD_PARSER_INTERFACE_H
