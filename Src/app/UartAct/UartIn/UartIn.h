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

#ifndef UART_IN_H
#define UART_IN_H

#include "bsp.h"
#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "fw_pipe.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

class UartIn : public Region {
public:
    UartIn(Hsmn hsmn, char const *name, UART_HandleTypeDef &hal);
    static void DmaCompleteCallback(Hsmn hsmn);
    static void DmaHalfCompleteCallback(Hsmn hsmn);

    typedef uint8_t HwError;
    static const HwError HW_ERROR_NONE    = 0x00;   // No error.
    static const HwError HW_ERROR_OVERRUN = 0x01;   // HW rx buffer overrun.
    static const HwError HW_ERROR_NOISE   = 0x02;   // Noise detected on a received frame.
    static const HwError HW_ERROR_FRAME   = 0x04;   // Frame error (e.g. caused by incorrect baud rate).

    static void RxCallback(Hsmn hsmn, HwError error = HW_ERROR_NONE);

protected:
    static QState InitialPseudoState(UartIn * const me, QEvt const * const e);
    static QState Root(UartIn * const me, QEvt const * const e);
        static QState Stopped(UartIn * const me, QEvt const * const e);
        static QState Started(UartIn * const me, QEvt const * const e);
          static QState Failed(UartIn * const me, QEvt const * const e);
          static QState Normal(UartIn * const me, QEvt const * const e);
            static QState Inactive(UartIn * const me, QEvt const * const e);
            static QState Active(UartIn * const me, QEvt const * const e);

    void EnableRxInt();
    void DisableRxInt();
    static void CleanInvalidateCache(uint32_t addr, uint32_t len);

    UART_HandleTypeDef &m_hal;
    Hsmn m_manager;
    Hsmn m_client;
    Fifo *m_fifo;
    bool m_dataRecv;
    Timer m_activeTimer;

    enum{
        ACTIVE_TIMEOUT_MS = 10
    };

    enum {
        ACTIVE_TIMER = TIMER_EVT_START(UART_IN),
    };

    enum {
        DONE = INTERNAL_EVT_START(UART_IN),
        DATA_RDY,
        DMA_RECV,
        FIFO_OVERFLOW,      // Can't use OVERFLOW since it conflicts with math.h
        HW_FAIL,
    };
};

} // namespace APP

#endif // UART_IN_H
