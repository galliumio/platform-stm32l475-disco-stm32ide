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

#ifndef DISP_MSG_INTERFACE_H
#define DISP_MSG_INTERFACE_H

#include "fw_def.h"
#include "fw_msg.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

// This file defines messages for the "Disp (Display)" role.

class DispTickerReqMsg final: public Msg {
public:
    DispTickerReqMsg(char const *to, uint16_t seq = 0,
                     char const *text = MSG_UNDEF, uint32_t fgColor = 0, uint32_t bgColor = 0, uint16_t index = 0) :
        Msg("DispTickerReqMsg", to, MSG_UNDEF, seq), m_fgColor(fgColor), m_bgColor(bgColor), m_index(index) {
        m_len = sizeof(*this);
        STRBUF_COPY(m_text, text);
    }
    char const *GetText() const { return m_text; }
    uint32_t GetFgColor() const { return m_fgColor; }
    uint32_t GetBgColor() const { return m_bgColor; }
    uint16_t GetIndex() const { return m_index; }

protected:
    char m_text[200];
    uint32_t m_fgColor;   // Foreground color (888 xBGR - little-endian RGB)
    uint32_t m_bgColor;   // Background color (888 xBGR - little-endian RGB)
    uint16_t m_index;
} __attribute__((packed));

class DispTickerCfmMsg final : public ErrorMsg {
public:
    DispTickerCfmMsg(char const *to, uint16_t seq = 0,
                  char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        ErrorMsg("DispTickerCfmMsg", to, MSG_UNDEF, seq, error, origin, reason) {
        m_len = sizeof(*this);
    }
} __attribute__((packed));


} // namespace APP

#endif // DISP_MSG_INTERFACE_H
