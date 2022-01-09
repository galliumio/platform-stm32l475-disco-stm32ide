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

#ifndef UART_ACT_H
#define UART_ACT_H

#include "bsp.h"
#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "fw_pipe.h"
#include "app_hsmn.h"
#include "UartIn.h"
#include "UartOut.h"

using namespace QP;
using namespace FW;

namespace APP {

class UartAct : public Active {
public:
    using Active::GetHsmn;

    typedef KeyValue<Hsmn, UART_HandleTypeDef *> HsmnHal;
    typedef Map<Hsmn, UART_HandleTypeDef *> HsmnHalMap;
    static HsmnHalMap &GetHsmnHalMap();
    static void SaveHal(Hsmn hsmn, UART_HandleTypeDef *hal);
    static UART_HandleTypeDef *GetHal(Hsmn hsmn);
    static Hsmn GetHsmn(UART_HandleTypeDef *hal);
    static uint16_t GetInst(Hsmn hsmn);
    static Hsmn GetUartInHsmn(Hsmn hsmn) { return UART_IN + GetInst(hsmn); }
    static Hsmn GetUartOutHsmn(Hsmn hsmn) { return UART_OUT + GetInst(hsmn); }

    UartAct(Hsmn hsmn, char const *name, char const *inName, char const *outName);

protected:
    static QState InitialPseudoState(UartAct * const me, QEvt const * const e);
    static QState Root(UartAct * const me, QEvt const * const e);
        static QState Stopped(UartAct * const me, QEvt const * const e);
        static QState Starting(UartAct * const me, QEvt const * const e);
        static QState Stopping(UartAct * const me, QEvt const * const e);
        static QState Started(UartAct * const me, QEvt const * const e);
            static QState Normal(UartAct * const me, QEvt const * const e);
            static QState Failed(UartAct * const me, QEvt const * const e);

    void InitUart();
    void DeInitUart();

    typedef struct {
        // Key
        Hsmn hsmn;
        // Common parameters.
        USART_TypeDef *uart;
        IRQn_Type uartIrq;
        uint32_t uartPrio;

        // Tx parameters.
        GPIO_TypeDef *txPort;
        uint32_t txPin;
        uint32_t txAf;
        DMA_Channel_TypeDef *txDmaCh;
        uint32_t txDmaReq;
        IRQn_Type txDmaIrq;
        uint32_t txDmaPrio;

        // Rx parameters.
        GPIO_TypeDef *rxPort;
        uint32_t rxPin;
        uint32_t rxAf;
        DMA_Channel_TypeDef *rxDmaCh;
        uint32_t rxDmaReq;
        IRQn_Type rxDmaIrq;
        uint32_t rxDmaPrio;
    } Config;
    static Config const CONFIG[];

    Config const *m_config;
    UART_HandleTypeDef m_hal;
    DMA_HandleTypeDef m_txDmaHandle;
    DMA_HandleTypeDef m_rxDmaHandle;
    Hsmn m_uartInHsmn;
    Hsmn m_uartOutHsmn;
    UartIn m_uartIn;
    UartOut m_uartOut;

    Hsmn m_client;
    Fifo *m_outFifo;
    Fifo *m_inFifo;
    Evt m_inEvt;                // Static event copy of a generic incoming req to be confirmed. Added more if needed.

    Timer m_stateTimer;

    enum {
        STATE_TIMER = TIMER_EVT_START(UART_ACT),
    };

    enum {
        START = INTERNAL_EVT_START(UART_ACT),
        DONE,
        FAIL
    };

    class Fail : public ErrorEvt {
    public:
        Fail(Error error, Hsmn origin, Reason reason) :
             ErrorEvt(FAIL, error, origin, reason) {}
    };
};

} // namespace APP

#endif // UART_ACT_H
