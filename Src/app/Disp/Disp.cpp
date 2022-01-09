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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "DispInterface.h"
#include "Disp.h"

FW_DEFINE_THIS_FILE("Disp.cpp")

// Includes Adafruit's standard 5x7 font.
#include "glcdfont.cpp"

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "DISP_TIMER_EVT_START",
    DISP_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "DISP_INTERNAL_EVT_START",
    DISP_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "DISP_INTERFACE_EVT_START",
    DISP_INTERFACE_EVT
};

Disp::Disp(QP::QStateHandler const initial, Hsmn hsmn, char const *name) :
    Region(initial, hsmn, name),
    m_cursorX(0), m_cursorY(0), m_textcolor(COLOR565_BLACK), m_textbgcolor(COLOR565_WHITE),
    m_textsize(1), m_wrap(true), m_gfxFont(NULL) {
    SET_EVT_NAME(DISP);
}

// Fills a memory buffer of dimension colxrow pixels with a rectangle of size wxh pixels at location (x, y).
// The fill color is specified by color. Each pixel has two bytes.
void Disp::FillMem(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color, uint16_t col, uint16_t row) {
    FW_ASSERT(((x+w) <= col) && ((y+h) <= row));
    FW_ASSERT((uint32_t)((y+h-1)*col*2 + (x+w-1)*2 + 1) < sizeof(m_memBuf));
    for (uint32_t i=0; i<h; i++) {
        for (uint32_t j=0; j<w; j++) {
            m_memBuf[(y+i)*col*2 + (x+j)*2] = BYTE_1(color);
            m_memBuf[(y+i)*col*2 + (x+j)*2 + 1] = BYTE_0(color);
        }
    }
}

void Disp::DrawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) {
    if(!m_gfxFont) { // 'Classic' built-in font
        uint16_t col = 6*size;
        uint16_t row = 8*size;
        if((x >= GetWidth())            || // Clip right
           (y >= GetHeight())           || // Clip bottom
           ((x + col - 1) < 0) || // Clip left
           ((y + row - 1) < 0))   // Clip top
            return;
        // Gallium - Optimization for opaque font using data buffer to write a character bitmap.
        //           (@todo - Replace hardcoded parameters)
        if (bg != color) {
            // Width is 6 to add a vertical line after the character.
            FillMem(0, 0, col, row, bg, col, row);
            for(int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
                uint8_t line = font[c * 5 + i];
                for(int8_t j=0; j<8; j++, line >>= 1) {
                    if(line & 1) {
                        FillMem(i*size, j*size, size, size, color, col, row);
                    }
                }
            }
            WriteBitmap(x, y, col, row, m_memBuf, col*row*2);
        } else {
            // Original library method. It is for both opaque and transparent, though here it is always transparent.
            // Original begins.
            for(int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
                uint8_t line = font[c * 5 + i];
                for(int8_t j=0; j<8; j++, line >>= 1) {
                    if(line & 1) {
                        if(size == 1)
                            WritePixel(x+i, y+j, color);
                        else
                            FillRect(x+i*size, y+j*size, size, size, color);
                    } else if(bg != color) {
                        if(size == 1)
                            WritePixel(x+i, y+j, bg);
                        else
                            FillRect(x+i*size, y+j*size, size, size, bg);
                    }
                }
            }
            if(bg != color) { // If opaque, draw vertical line for last column
                if(size == 1) WriteFastVLine(x+5, y, 8, bg);
                else          FillRect(x+5*size, y, size, 8*size, bg);
            }
            // Original ends.
        }
    } else { // Custom font

        // Character is assumed previously filtered by write() to eliminate
        // newlines, returns, non-printable characters, etc.  Calling
        // drawChar() directly with 'bad' characters of font may cause mayhem!

        c -= m_gfxFont->first;
        GFXglyph *glyph  = &(m_gfxFont->glyph[c]);
        uint8_t  *bitmap = m_gfxFont->bitmap;

        uint16_t bo = glyph->bitmapOffset;
        uint8_t  w  = glyph->width,
                 h  = glyph->height;
        int8_t   xo = glyph->xOffset,
                 yo = glyph->yOffset;
        uint8_t  xx, yy, bits = 0, bit = 0;
        int16_t  xo16 = 0, yo16 = 0;

        if(size > 1) {
            xo16 = xo;
            yo16 = yo;
        }

        // Todo: Add character clipping here
        // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS. See original source for details.
        for(yy=0; yy<h; yy++) {
            for(xx=0; xx<w; xx++) {
                if(!(bit++ & 7)) {
                    bits = bitmap[bo++];
                }
                if(bits & 0x80) {
                    if(size == 1) {
                        WritePixel(x+xo+xx, y+yo+yy, color);
                    } else {
                        FillRect(x+(xo16+xx)*size, y+(yo16+yy)*size,
                          size, size, color);
                    }
                }
                bits <<= 1;
            }
        }
    } // End classic vs custom font
}

void Disp::Write(uint8_t c) {
    if(!m_gfxFont) { // 'Classic' built-in font
        if(c == '\n') {                        // Newline?
            m_cursorX  = 0;                     // Reset x to zero,
            m_cursorY += m_textsize * 8;          // advance y one line
        } else if(c != '\r') {                 // Ignore carriage returns
            if(m_wrap && ((m_cursorX + m_textsize * 6) > GetWidth())) { // Off right?
                m_cursorX  = 0;                 // Reset x to zero,
                m_cursorY += m_textsize * 8;      // advance y one line
            }
            DrawChar(m_cursorX, m_cursorY, c, m_textcolor, m_textbgcolor, m_textsize);
            m_cursorX += m_textsize * 6;          // Advance x one char
        }
    } else { // Custom font
        if(c == '\n') {
            m_cursorX  = 0;
            m_cursorY += (int16_t)m_textsize * m_gfxFont->yAdvance;
        } else if(c != '\r') {
            uint8_t first = m_gfxFont->first;
            if((c >= first) && (c <= m_gfxFont->last)) {
                GFXglyph *glyph = &((m_gfxFont->glyph)[c - first]);
                uint8_t w = glyph->width;
                uint8_t h = glyph->height;
                if((w > 0) && (h > 0)) { // Is there an associated bitmap?
                    int16_t xo = glyph->xOffset; // sic
                    if(m_wrap && ((m_cursorX + m_textsize * (xo + w)) > GetWidth())) {
                        m_cursorX = 0;
                        m_cursorY += (int16_t)m_textsize * m_gfxFont->yAdvance;
                    }
                    DrawChar(m_cursorX, m_cursorY, c, m_textcolor, m_textbgcolor, m_textsize);
                }
                m_cursorX +=glyph->xAdvance * (int16_t)m_textsize;
            }
        }
    }
}

void Disp::SetFont(const GFXfont *f) {
    if(f) {            // Font struct pointer passed in?
        if(!m_gfxFont) { // And no current font struct?
            // Switching from classic to new font behavior.
            // Move cursor pos down 6 pixels so it's on baseline.
            m_cursorY += 6;
        }
    } else if(m_gfxFont) { // NULL passed.  Current font struct defined?
        // Switching from new to classic font behavior.
        // Move cursor pos up 6 pixels so it's at top-left of char.
        m_cursorY -= 6;
    }
    m_gfxFont = (GFXfont *)f;
}

// Broke this out as it's used by both the PROGMEM- and RAM-resident
// getTextBounds() functions.
void Disp::CharBounds(char c, int16_t *x, int16_t *y, int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy) {
    if(m_gfxFont) {
        if(c == '\n') { // Newline?
            *x  = 0;    // Reset x to zero, advance y by one line
            *y += m_textsize * m_gfxFont->yAdvance;
        } else if(c != '\r') { // Not a carriage return; is normal char
            uint8_t first = m_gfxFont->first;
            uint8_t last  = m_gfxFont->last;
            if((c >= first) && (c <= last)) { // Char present in this font?
                GFXglyph *glyph = &(m_gfxFont->glyph[c - first]);
                uint8_t gw = glyph->width,
                        gh = glyph->height,
                        xa = glyph->xAdvance;
                int8_t  xo = glyph->xOffset,
                        yo = glyph->yOffset;
                if(m_wrap && ((*x+(((int16_t)xo+gw)*m_textsize)) > GetWidth())) {
                    *x  = 0; // Reset x to zero, advance y by one line
                    *y += m_textsize * m_gfxFont->yAdvance;
                }
                int16_t ts = (int16_t)m_textsize,
                        x1 = *x + xo * ts,
                        y1 = *y + yo * ts,
                        x2 = x1 + gw * ts - 1,
                        y2 = y1 + gh * ts - 1;
                if(x1 < *minx) *minx = x1;
                if(y1 < *miny) *miny = y1;
                if(x2 > *maxx) *maxx = x2;
                if(y2 > *maxy) *maxy = y2;
                *x += xa * ts;
            }
        }
    } else { // Default font
        if(c == '\n') {                     // Newline?
            *x  = 0;                        // Reset x to zero,
            *y += m_textsize * 8;           // advance y one line
            // min/max x/y unchaged -- that waits for next 'normal' character
        } else if(c != '\r') {  // Normal char; ignore carriage returns
            if(m_wrap && ((*x + m_textsize * 6) > GetWidth())) { // Off right?
                *x  = 0;                    // Reset x to zero,
                *y += m_textsize * 8;         // advance y one line
            }
            int x2 = *x + m_textsize * 6 - 1, // Lower-right pixel of char
                y2 = *y + m_textsize * 8 - 1;
            if(x2 > *maxx) *maxx = x2;      // Track max x, y
            if(y2 > *maxy) *maxy = y2;
            if(*x < *minx) *minx = *x;      // Track min x, y
            if(*y < *miny) *miny = *y;
            *x += m_textsize * 6;             // Advance x one char
        }
    }
}

// Pass string and a cursor position, returns UL corner and W,H.
void Disp::GetTextBounds(char *str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    uint8_t c; // Current character

    *x1 = x;
    *y1 = y;
    *w  = *h = 0;

    int16_t minx = GetWidth(), miny = GetHeight(), maxx = -1, maxy = -1;

    while((c = *str++))
        CharBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);

    if(maxx >= minx) {
        *x1 = minx;
        *w  = maxx - minx + 1;
    }
    if(maxy >= miny) {
        *y1 = miny;
        *h  = maxy - miny + 1;
    }
}

} // namespace APP
