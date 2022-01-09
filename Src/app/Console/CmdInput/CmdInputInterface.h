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

#ifndef CMD_INPUT_INTERFACE_H
#define CMD_INPUT_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

enum {
    CMD_INPUT_START_REQ = INTERFACE_EVT_START(CMD_INPUT),
    CMD_INPUT_STOP_REQ,
    CMD_INPUT_CHAR_REQ,
};

enum {
    CMD_INPUT_REASON_UNSPEC = 0,
};

// No confirmation required.
class CmdInputStartReq : public Evt {
public:
    // For direct dispatch() from Console.cpp.
    CmdInputStartReq(Hsmn to, Hsmn from, QEvt::StaticEvt /*dummy*/) :
        Evt(CMD_INPUT_START_REQ, to, from, 0, QEvt::STATIC_EVT) {}
};

// No confirmation required.
class CmdInputStopReq : public Evt {
public:
    CmdInputStopReq(Hsmn to, Hsmn from, QEvt::StaticEvt /*dummy*/) :
        Evt(CMD_INPUT_STOP_REQ, to, from, 0, QEvt::STATIC_EVT) {}
};

class CmdInputCharReq : public Evt {
public:
    // For direct dispatch() from Console.cpp.
    CmdInputCharReq(Hsmn to, Hsmn from, char ch, QEvt::StaticEvt /*dummy*/) :
        Evt(CMD_INPUT_CHAR_REQ, to, from, 0, QEvt::STATIC_EVT), m_ch(ch) {}
    // For Raise() within CmdInput.cpp.
    CmdInputCharReq(char ch) :
        Evt(CMD_INPUT_CHAR_REQ), m_ch(ch) {}
    char GetCh() const { return m_ch; }
private:
    char m_ch;       // Character to be processed.
};

} // namespace APP

#endif // CMD_INPUT_INTERFACE_H
