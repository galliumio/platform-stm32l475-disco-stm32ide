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

/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DISP_H
#define DISP_H

#include "qpcpp.h"
#include "fw_region.h"
#include "app_hsmn.h"
#include "gfxfont.h"

using namespace QP;
using namespace FW;

namespace APP {

// Color definitions
#define COLOR565_BLACK       0x0000      ///<   0,   0,   0
#define COLOR565_NAVY        0x000F      ///<   0,   0, 128
#define COLOR565_DARKGREEN   0x03E0      ///<   0, 128,   0
#define COLOR565_DARKCYAN    0x03EF      ///<   0, 128, 128
#define COLOR565_MAROON      0x7800      ///< 128,   0,   0
#define COLOR565_PURPLE      0x780F      ///< 128,   0, 128
#define COLOR565_OLIVE       0x7BE0      ///< 128, 128,   0
#define COLOR565_LIGHTGREY   0xC618      ///< 192, 192, 192
#define COLOR565_DARKGREY    0x7BEF      ///< 128, 128, 128
#define COLOR565_BLUE        0x001F      ///<   0,   0, 255
#define COLOR565_GREEN       0x07E0      ///<   0, 255,   0
#define COLOR565_CYAN        0x07FF      ///<   0, 255, 255
#define COLOR565_RED         0xF800      ///< 255,   0,   0
#define COLOR565_MAGENTA     0xF81F      ///< 255,   0, 255
#define COLOR565_YELLOW      0xFFE0      ///< 255, 255,   0
#define COLOR565_WHITE       0xFFFF      ///< 255, 255, 255
#define COLOR565_ORANGE      0xFD20      ///< 255, 165,   0
#define COLOR565_GREENYELLOW 0xAFE5      ///< 173, 255,  47
#define COLOR565_PINK        0xFC18      ///< 255, 128, 192

class Disp : public Region {
public:
    Disp(QP::QStateHandler const initial, Hsmn hsmn, char const *name);
    virtual ~Disp() {}

protected:
    // Virtual functions used by high-level graphical functions.
    virtual uint16_t GetWidth() { return 0; }
    virtual uint16_t GetHeight() { return 0; }
    virtual void SetRotation(uint8_t rotation) { (void)rotation; };
    virtual void WritePixel(int16_t x, int16_t y, uint16_t color) { (void)x; (void)y; (void)color; };
    virtual void FillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color) { (void)x; (void)y; (void)w; (void)h; (void)color; };
    virtual void WriteBitmap(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t *buf, uint32_t len) { (void)x; (void)y; (void)w; (void)h; (void)buf; (void)len; };

    // High-level graphical functions for use by state-machines of derived classes.
    void WriteFastVLine(int16_t x, int16_t y, int16_t len, uint16_t color) { FillRect(x, y, 1, len, color); }
    void WriteFastHLine(int16_t x, int16_t y, int16_t len, uint16_t color) { FillRect(x, y, len, 1, color); }
    void FillScreen(uint16_t color) { FillRect(0, 0, GetWidth(), GetHeight(), color); }
    static uint16_t Color565(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
    static uint16_t Color565(uint32_t rgb) { return Color565(BYTE_2(rgb), BYTE_1(rgb), BYTE_0(rgb)); }
    void DrawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
    void Write(uint8_t c);
    void SetCursor(int16_t x, int16_t y) { m_cursorX = x; m_cursorY = y; }
    void SetTextSize(uint8_t s) { m_textsize = (s > 0) ? s : 1; }
    // For 'transparent' background, we'll set the bg to the same as fg
    void SetTextColor(uint16_t c) { m_textcolor = m_textbgcolor = c; }
    void SetTextColor(uint16_t c, uint16_t b) { m_textcolor = c; m_textbgcolor = b; }
    void SetTextWrap(bool w) { m_wrap = w; }
    void SetFont(const GFXfont *f);
    void CharBounds(char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy);
    void GetTextBounds(char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);

    int16_t m_cursorX;
    int16_t m_cursorY;
    uint16_t m_textcolor;
    uint16_t m_textbgcolor;
    uint8_t m_textsize;
    bool m_wrap;        // If set, 'wrap' text at right edge of display
    GFXfont *m_gfxFont;
    // Gallium - Optimization.
    // Fast method using data buffer for a character bitmap (@todo - Replace hardcoded parameters.)
    // It is hardcoded for 5x7 font, effective 6x8 with space lines.
    // It supports max multiplication (size factor) up to 4 in each dimension. Each pixel has 2 bytes.
    enum {
        MAX_FONT_COL = 6,       // Effective max no. of columns of a font char, including space line.
        MAX_FONT_ROW = 8,       // Effective max no. of rows of a font char, including space line.
        MAX_FONT_SIZE = 4,      // Max multiplication factor.
    };
    uint8_t m_memBuf[(MAX_FONT_COL*MAX_FONT_SIZE)*(MAX_FONT_ROW*MAX_FONT_SIZE)*2];
    void FillMem(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color, uint16_t col, uint16_t row);

#define DISP_TIMER_EVT \
    ADD_EVT(STATE_TIMER)

#define DISP_INTERNAL_EVT \
    ADD_EVT(START) \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED)


#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        DISP_TIMER_EVT_START = TIMER_EVT_START(DISP),
        DISP_TIMER_EVT
    };

    enum {
        DISP_INTERNAL_EVT_START = INTERNAL_EVT_START(DISP),
        DISP_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Hsmn hsmn, Error error, Hsmn origin, Reason reason) :
            ErrorEvt(FAILED, hsmn, hsmn, 0, error, origin, reason) {}
    };
};

} // namespace APP

#endif // DISP_H
