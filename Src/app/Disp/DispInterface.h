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

#ifndef DISP_INTERFACE_H
#define DISP_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "fw_assert.h"

#define DISP_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("DispInterface.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

#define DISP_INTERFACE_EVT \
    ADD_EVT(DISP_START_REQ) \
    ADD_EVT(DISP_START_CFM) \
    ADD_EVT(DISP_STOP_REQ) \
    ADD_EVT(DISP_STOP_CFM) \
    ADD_EVT(DISP_DRAW_BEGIN_REQ) \
    ADD_EVT(DISP_DRAW_BEGIN_CFM) \
    ADD_EVT(DISP_DRAW_END_REQ) \
    ADD_EVT(DISP_DRAW_END_CFM) \
    ADD_EVT(DISP_DRAW_TEXT_REQ) \
    ADD_EVT(DISP_DRAW_RECT_REQ)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    DISP_INTERFACE_EVT_START = INTERFACE_EVT_START(DISP),
    DISP_INTERFACE_EVT
};

enum {
    DISP_REASON_UNSPEC = 0,
};

// Color definitions
#define COLOR24_BLACK        0x000000      ///<   0,   0,   0
#define COLOR24_WHITE        0xFFFFFF      ///< 255, 255, 255
#define COLOR24_GRAY         0x808080
#define COLOR24_DARK_GRAY    0x202020
#define COLOR24_RED          0xFF0000
#define COLOR24_GREEN        0x00FF00
#define COLOR24_BLUE         0x0000FF
#define COLOR24_YELLOW       0xFFFF00

class DispStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    DispStartReq() :
        Evt(DISP_START_REQ) {}
};

class DispStartCfm : public ErrorEvt {
public:
    DispStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(DISP_START_CFM, error, origin, reason) {}
};

class DispStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    DispStopReq() :
        Evt(DISP_STOP_REQ) {}
};

class DispStopCfm : public ErrorEvt {
public:
    DispStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(DISP_STOP_CFM, error, origin, reason) {}
};

class DispDrawBeginReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    DispDrawBeginReq() :
        Evt(DISP_DRAW_BEGIN_REQ) {}
};

class DispDrawBeginCfm : public ErrorEvt {
public:
    DispDrawBeginCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(DISP_DRAW_BEGIN_CFM, error, origin, reason) {}
};

class DispDrawEndReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    DispDrawEndReq() :
        Evt(DISP_DRAW_END_REQ) {}
};

class DispDrawEndCfm : public ErrorEvt {
public:
    DispDrawEndCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(DISP_DRAW_END_CFM, error, origin, reason) {}
};

// There is no confirmation. Sequence number is always 0.
class DispDrawTextReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100,
        MAX_TEXT_LEN = 32
    };
    DispDrawTextReq(char const *text, int16_t x, int16_t y,
                    uint32_t textColor = COLOR24_BLACK, uint32_t bgColor = COLOR24_WHITE, uint8_t multiplier = 1) :
        Evt(DISP_DRAW_TEXT_REQ),
        m_x(x), m_y(y), m_textColor(textColor), m_bgColor(bgColor), m_multiplier(multiplier) {
        DISP_INTERFACE_ASSERT(text && (multiplier > 0));
        STRING_COPY(m_text, text, sizeof(m_text));

    }
    char const *GetText() const { return m_text; }
    int16_t GetX() const { return m_x; }
    int16_t GetY() const { return m_y; }
    uint32_t GetTextColor() const { return m_textColor; }
    uint32_t GetBgColor() const { return m_bgColor; }
    uint8_t GetMultiplier() const { return m_multiplier; }
private:
    char m_text[MAX_TEXT_LEN];    // Null-terminated string to draw.
    int16_t m_x;
    int16_t m_y;
    uint32_t m_textColor;         // 24-bit RGB
    uint32_t m_bgColor;           // 24-bit RGB. Background color. If same as textColor, background is transparent.
    uint8_t m_multiplier;         // Font size multiplier. Must >= 1. If 1, same as original font size.
};

// There is no confirmation. Sequence number is always 0.
class DispDrawRectReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    DispDrawRectReq(int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t color) :
        Evt(DISP_DRAW_RECT_REQ),
        m_x(x), m_y(y), m_w(w), m_h(h), m_color(color) {
        DISP_INTERFACE_ASSERT(w && h);
    }
    DispDrawRectReq(Hsmn to, Hsmn from, Sequence seq, int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t color) :
        Evt(DISP_DRAW_RECT_REQ, to, from, seq),
        m_x(x), m_y(y), m_w(w), m_h(h), m_color(color) {
        DISP_INTERFACE_ASSERT(w && h);
    }
    int16_t GetX() const { return m_x; }
    int16_t GetY() const { return m_y; }
    uint16_t GetW() const { return m_w; }
    uint16_t GetH() const { return m_h; }
    uint32_t GetColor() const { return m_color; }
private:
    int16_t m_x;
    int16_t m_y;
    uint16_t m_w;
    uint16_t m_h;
    uint32_t m_color;         // 24-bit RGB
};

} // namespace APP

#endif // DISP_INTERFACE_H
