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

#ifndef CONSOLE_INTERFACE_H
#define CONSOLE_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "fw_assert.h"

using namespace QP;
using namespace FW;

namespace APP {
enum {
    CONSOLE_START_REQ = INTERFACE_EVT_START(CONSOLE),
    CONSOLE_START_CFM,
    CONSOLE_STOP_REQ,
    CONSOLE_STOP_CFM,
    CONSOLE_RAW_ENABLE_REQ,     // type = Evt
};

enum {
    CONSOLE_REASON_UNSPEC = 0,
};

class Console;
enum CmdStatus {
    CMD_DONE,           // Command processing is complete.
    CMD_CONTINUE,       // Further processing is pending.
};
typedef CmdStatus (*CmdFunc)(Console& console, Evt const *e);
struct CmdHandler {
    char const *key;            // Command to match.
    CmdFunc func;               // Handler function.
    char const *text;           // Text description.
    bool isSuper;               // True if only superuser can run it.
};

class ConsoleStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 300
    };
    ConsoleStartReq(Hsmn to, Hsmn from, Sequence seq, CmdFunc cmdFunc, Hsmn ifHsmn, bool isDefault = true) :
        Evt(CONSOLE_START_REQ, to, from, seq), m_cmdFunc(cmdFunc),
        m_ifHsmn(ifHsmn), m_isDefault(isDefault) {}
    CmdFunc GetCmdFunc() const { return m_cmdFunc; }
    Hsmn GetIfHsmn() const { return m_ifHsmn; }
    bool IsDefault() const { return m_isDefault; }
private:
    CmdFunc m_cmdFunc;
    Hsmn m_ifHsmn;      // HSMN of the interface active object.
    bool m_isDefault;   // True if this is a default interface for log output when interface is not specified.
};

class ConsoleStartCfm : public ErrorEvt {
public:
    ConsoleStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(CONSOLE_START_CFM, error, origin, reason) {}
};

class ConsoleStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 300
    };
    ConsoleStopReq(Hsmn to, Hsmn from, Sequence seq) :
        Evt(CONSOLE_STOP_REQ, to, from, seq) {}
};

class ConsoleStopCfm : public ErrorEvt {
public:
    ConsoleStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(CONSOLE_STOP_CFM, error, origin, reason) {}
};

} // namespace APP

#endif // CONSOLE_INTERFACE_H
