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

#ifndef UART_OUT_H
#define UART_OUT_H

#include "bsp.h"
#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

class UartOut : public Region {
public:
    UartOut(Hsmn hsmn, char const *name, UART_HandleTypeDef &hal);
    static void DmaCompleteCallback(Hsmn hsmn);

protected:
    static QState InitialPseudoState(UartOut * const me, QEvt const * const e);
    static QState Root(UartOut * const me, QEvt const * const e);
        static QState Stopped(UartOut * const me, QEvt const * const e);
        static QState Started(UartOut * const me, QEvt const * const e);
            static QState Inactive(UartOut * const me, QEvt const * const e);
            static QState Active(UartOut * const me, QEvt const * const e);
                static QState Normal(UartOut * const me, QEvt const * const e);
                static QState StopWait(UartOut * const me, QEvt const * const e);
            static QState Failed(UartOut * const me, QEvt const * const e);

    static void CleanCache(uint32_t addr, uint32_t len);

    UART_HandleTypeDef &m_hal;
    Hsmn m_manager;     // Managing HSM
    Hsmn m_client;      // User HSM
    Fifo *m_fifo;
    uint32_t m_writeCount;
    Timer m_activeTimer;

    enum {
        ACTIVE_TIMEOUT_MS = 1000,
    };

    enum {
        ACTIVE_TIMER = TIMER_EVT_START(UART_OUT),
    };

    enum {
        DONE = INTERNAL_EVT_START(UART_OUT),
        DMA_DONE,
        CONTINUE,
        HW_FAIL,
    };
};

} // namespace APP

#endif // UART_OUT_H
