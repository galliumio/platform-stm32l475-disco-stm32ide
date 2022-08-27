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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "UartInInterface.h"
#include "UartIn.h"

FW_DEFINE_THIS_FILE("UartIn.cpp")

namespace APP {

static char const * const timerEvtName[] = {
    "ACTIVE_TIMER",
};

static char const * const internalEvtName[] = {
    "DONE",
    "DATA_RDY",
    "DMA_RECV",
    "FIFO_OVERFLOW",
    "HW_FAIL",
};

static char const * const interfaceEvtName[] = {
    "UART_IN_START_REQ",
    "UART_IN_START_CFM",
    "UART_IN_STOP_REQ",
    "UART_IN_STOP_CFM",
    "UART_IN_FAIL_IND",
};

static uint16_t GetInst(Hsmn hsmn) {
    uint16_t inst = hsmn - UART_IN;
    FW_ASSERT(inst < UART_IN_COUNT);
    return inst;
}

void UartIn::DmaCompleteCallback(Hsmn hsmn) {
    static Sequence counter = 10000;
    Evt *evt = new Evt(UartIn::DMA_RECV, hsmn, HSM_UNDEF, counter++);
    Fw::Post(evt);
}

void UartIn::DmaHalfCompleteCallback(Hsmn hsmn) {
    static Sequence counter = 10000;
    Evt *evt = new Evt(UartIn::DMA_RECV, hsmn, HSM_UNDEF, counter++);
    Fw::Post(evt);
}

void UartIn::RxCallback(Hsmn hsmn, HwError error) {
    static Sequence counter = 10000;
    // It is safe to always treat it as "data ready", even if it only carries error info.
    Evt *evt = new Evt(UartIn::DATA_RDY, hsmn, HSM_UNDEF, counter++);
    Fw::PostNotInQ(evt);
    if (error == HW_ERROR_OVERRUN) {
        // For now, we are only interested in overrun errors (ignoring noise error).
        PRINT("UartIn::RxCallback - overrun error (%x)\n\r", error);
    }
}

void UartIn::EnableRxInt() {
    // Enable Data Register Not Empty Interrupt.
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    SET_BIT(m_hal.Instance->CR1, USART_CR1_RXNEIE);
    QF_CRIT_EXIT(crit);
}
    
void UartIn::DisableRxInt() {
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    CLEAR_BIT(m_hal.Instance->CR1, USART_CR1_RXNEIE);
    QF_CRIT_EXIT(crit);
}

void UartIn::CleanInvalidateCache(uint32_t addr, uint32_t len) {
    // Cache not available on this platform.
    //SCB_CleanInvalidateDCache_by_Addr(reinterpret_cast<uint32_t *>(ROUND_DOWN_32(addr)), ROUND_UP_32(addr + len) - ROUND_DOWN_32(addr));
    (void)addr;
    (void)len;
}

UartIn::UartIn(Hsmn hsmn, char const *name, UART_HandleTypeDef &hal) :
    Region((QStateHandler)&UartIn::InitialPseudoState, hsmn, name),
    m_hal(hal), m_manager(HSM_UNDEF), m_client(HSM_UNDEF), m_fifo(NULL), m_dataRecv(false), m_activeTimer(hsmn, ACTIVE_TIMER) {
    SET_EVT_NAME(UART_IN);
}

QState UartIn::InitialPseudoState(UartIn * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&UartIn::Root);
}

QState UartIn::Root(UartIn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&UartIn::Stopped);
            break;
        }
        case UART_IN_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new UartInStartCfm(ERROR_STATE, me->GetHsmn()), req);
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&QHsm::top);
            break;
        }
    }
    return status;
}

QState UartIn::Stopped(UartIn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case UART_IN_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new UartInStopCfm(ERROR_SUCCESS), req);
            status = Q_HANDLED();
            break;
        }
        case UART_IN_START_REQ: {
            EVENT(e);
            UartInStartReq const &req = static_cast<UartInStartReq const &>(*e);
            me->m_manager = req.GetFrom();
            me->m_fifo = req.GetFifo();
            FW_ASSERT(me->m_fifo);
            me->m_fifo->Reset();
            me->m_client = req.GetClient();
            me->SendCfm(new UartInStartCfm(ERROR_SUCCESS), req);
            status = Q_TRAN(&UartIn::Started);
            break;
        }
        default: {
            status = Q_SUPER(&UartIn::Root);
            break;
        }
    }
    return status;
}

QState UartIn::Started(UartIn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            LOG("Instance = %d", GetInst(me->GetHsmn()));
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&UartIn::Normal);
            break;
        }
        case UART_IN_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new UartInStopCfm(ERROR_SUCCESS), req);
            status = Q_TRAN(&UartIn::Stopped);
            break;
        }
        default: {
            status = Q_SUPER(&UartIn::Root);
            break;
        }
    }
    return status;
}

QState UartIn::Failed(UartIn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&UartIn::Started);
            break;
        }
    }
    return status;
}

QState UartIn::Normal(UartIn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            HAL_StatusTypeDef result;
            result = HAL_UART_Receive_DMA(&me->m_hal, (uint8_t *)me->m_fifo->GetAddr(0), me->m_fifo->GetBufSize());
            FW_ASSERT(result == HAL_OK);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            HAL_UART_DMAStop(&me->m_hal);
            me->DisableRxInt();
            status = Q_HANDLED();
            break;
        }
        case DMA_RECV: {
            EVENT(e);
            // Sample DMA remaining count first. It may keep decrementing as data are being received.
            // Those that arrive after this point will be processed on the next DMA_RECV event.
            // The FIFO write index is only updated in this region, so there is no need to enforce
            // critical section.
            uint32_t dmaRemainCount = __HAL_DMA_GET_COUNTER(me->m_hal.hdmarx);
            uint32_t dmaCurrIndex = me->m_fifo->GetBufSize() - dmaRemainCount;
            uint32_t dmaRxCount = me->m_fifo->GetDiff(dmaCurrIndex, me->m_fifo->GetWriteIndex());
            if (dmaRxCount > 0) {
                if (dmaRxCount > me->m_fifo->GetAvailCount()) {
                    me->Raise(new Evt(FIFO_OVERFLOW));
                } else {
                    me->m_fifo->CacheOp(UartIn::CleanInvalidateCache, dmaRxCount);
                    me->m_fifo->IncWriteIndex(dmaRxCount);
                    me->Send(new UartInDataInd(), me->m_client);
                }
            }
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&UartIn::Inactive);
            break;
        }
        // TODO - FIFO_OVERFLOW handling
        default: {
            status = Q_SUPER(&UartIn::Started);
            break;
        }
    }
    return status;
}

QState UartIn::Inactive(UartIn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->EnableRxInt();
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case DATA_RDY: {
            EVENT(e);
            // Receive buffer not empty interrupt from UART. It indicates UART RX activity.
            // Rx interrupt automatically disabled in ISR.
            status = Q_TRAN(&UartIn::Active);
            break;
        }
        default: {
            status = Q_SUPER(&UartIn::Normal);
            break;
        }
    }
    return status;
}

QState UartIn::Active(UartIn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_activeTimer.Start(ACTIVE_TIMEOUT_MS);
            me->EnableRxInt();
            me->m_dataRecv = false;
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_activeTimer.Stop();
            status = Q_HANDLED();
            break;
        }
        case ACTIVE_TIMER: {
            EVENT(e);
            if (me->m_dataRecv) {
                status = Q_TRAN(&UartIn::Active);
            } else {
                me->Send(new Evt(DMA_RECV), me->GetHsmn());
                status = Q_TRAN(&UartIn::Inactive);
            }
            break;
        }
        case DATA_RDY: {
            EVENT(e);
            // Receive buffer not empty interrupt from UART. It indicates UART RX activity.
            me->m_dataRecv = true;
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&UartIn::Normal);
            break;
        }
    }
    return status;
}

/*
QState UartIn::MyState(UartIn * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&UartIn::SubState);
            break;
        }
        default: {
            status = Q_SUPER(&UartIn::SuperState);
            break;
        }
    }
    return status;
}
*/

} // namespace APP
