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

#ifndef FW_DEF_H
#define FW_DEF_H

#include <stdint.h>
#include "qpcpp.h"
#include "fw_macro.h"

namespace FW {

#define HSMN_BIT_SIZE                       8
#define HSMN_BIT_MASK                       BIT_MASK_OF_SIZE(HSMN_BIT_SIZE)

// Storage size must >= HSMN_BIT_SIZE.
typedef uint8_t Hsmn;
Q_ASSERT_COMPILE((HSMN_BIT_SIZE) <= (8 * sizeof(Hsmn)));

enum {
    HSM_UNDEF = 0,          // HSM number 0 is reserved. Must not be used.
    MAX_HSM_COUNT = 64,     // Maximum number of HSM's including HSM_UNDEF.
                            // That is valid HSM number is from 1 to MAX_HSM_COUNT - 1.
    INSTANCE_UNDEF = (uint32_t)-1
};

#define EVT_TYPE_BIT_SIZE                   8
#define EVT_TYPE_BIT_MASK                   BIT_MASK_OF_SIZE(EVT_TYPE_BIT_SIZE)

#define TIMER_EVT_START(hsmId_)             BIT_DEF((hsmId_), HSMN_BIT_MASK, EVT_TYPE_BIT_SIZE)
#define INTERNAL_EVT_START(hsmId_)          BIT_SET(TIMER_EVT_START(hsmId_), 0x1, (EVT_TYPE_BIT_SIZE - 2))
#define INTERFACE_EVT_START(hsmId_)         BIT_SET(TIMER_EVT_START(hsmId_), 0x1, (EVT_TYPE_BIT_SIZE - 1))

#define GET_EVT_HSMN(signal_)               BIT_READ((signal_), HSMN_BIT_MASK, EVT_TYPE_BIT_SIZE)
#define GET_EVT_TYPE(signal_)               BIT_READ((signal_), EVT_TYPE_BIT_MASK, 0)

#define GET_TIMER_EVT_INDEX(signal_)        GET_EVT_TYPE(signal_)
#define GET_INTERNAL_EVT_INDEX(signal_)     BIT_CLR(GET_EVT_TYPE(signal_), 0x1, (EVT_TYPE_BIT_SIZE - 2))
#define GET_INTERFACE_EVT_INDEX(signal_)    BIT_CLR(GET_EVT_TYPE(signal_), 0x1, (EVT_TYPE_BIT_SIZE - 1))

#define IS_EVT_HSMN_VALID(signal_)          (GET_EVT_HSMN(signal_) != HSM_UNDEF)
#define IS_TIMER_EVT(signal_)               (BIT_READ((signal_), 0x3, (EVT_TYPE_BIT_SIZE - 2)) == 0x0)
#define IS_INTERNAL_EVT(signal_)            (BIT_READ((signal_), 0x3, (EVT_TYPE_BIT_SIZE - 2)) == 0x1)
#define IS_INTERFACE_EVT(signal_)           (BIT_READ((signal_), 0x1, (EVT_TYPE_BIT_SIZE - 1)) == 0x1)

#if ((EVT_TYPE_BIT_SIZE + HSMN_BIT_SIZE) > (8 * Q_SIGNAL_SIZE))
#error QSignal storage too small. Customize Q_SIGNAL_SIZE in qep_port.h
#endif

typedef QP::QSignal EvtCount;
typedef char const * const *EvtName;
typedef uint16_t Sequence;

enum {
    CTRLC = 0x03,
    BS  = 0x08,
    LF  = 0x0A,
    CR  = 0x0D,
    ESC = 0x1B,
    SP  = 0x20,
    DEL = 0x7F,
    TAB = 0x09,
    QUOTE = 0x22,
    SLASH = 0x5C,
};

} // namespace FW

#endif // FW_DEF_H

