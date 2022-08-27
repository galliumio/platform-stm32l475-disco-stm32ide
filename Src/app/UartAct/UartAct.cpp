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
#include "UartOutInterface.h"
#include "UartActInterface.h"
#include "UartAct.h"

FW_DEFINE_THIS_FILE("UartAct.cpp")

namespace APP {

UartAct::HsmnHalMap &UartAct::GetHsmnHalMap() {
    static HsmnHal hsmnHalStor[UART_ACT_COUNT];
    static HsmnHalMap hsmnHalMap(hsmnHalStor, ARRAY_COUNT(hsmnHalStor), HsmnHal(HSM_UNDEF, NULL));
    return hsmnHalMap;
}

// No need to have critical section since mapping is not changed after global/static object construction.
void UartAct::SaveHal(Hsmn hsmn, UART_HandleTypeDef *hal) {
    GetHsmnHalMap().Save(HsmnHal(hsmn, hal));
}

// No need to have critical section since mapping is not changed after global/static object construction.
UART_HandleTypeDef *UartAct::GetHal(Hsmn hsmn) {
    FW_ASSERT(hsmn != HSM_UNDEF);
    HsmnHal *kv = GetHsmnHalMap().GetByKey(hsmn);
    FW_ASSERT(kv);
    UART_HandleTypeDef *hal = kv->GetValue();
    FW_ASSERT(hal);
    return hal;
}

// No need to have critical section since mapping is not changed after global/static object construction.
Hsmn UartAct::GetHsmn(UART_HandleTypeDef *hal) {
    FW_ASSERT(hal);
    HsmnHal *kv = GetHsmnHalMap().GetFirstByValue(hal);
    FW_ASSERT(kv);
    Hsmn hsmn = kv->GetKey();
    FW_ASSERT(hsmn != HSM_UNDEF);
    return hsmn;
}

uint16_t UartAct::GetInst(Hsmn hsmn) {
    uint16_t inst = hsmn - UART_ACT;
    FW_ASSERT(inst < UART_ACT_COUNT);
    return inst;
}

static char const * const timerEvtName[] = {
    "STATE_TIMER",
};

static char const * const internalEvtName[] = {
    "START",
    "DONE",
    "FAIL",
};

static char const * const interfaceEvtName[] = {
    "UART_ACT_START_REQ",
    "UART_ACT_START_CFM",
    "UART_ACT_STOP_REQ",
    "UART_ACT_STOP_CFM",
    "UART_ACT_FAIL_IND",
};

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *hal) {
    Hsmn hsmn = UartAct::GetHsmn(hal);
    UartOut::DmaCompleteCallback(UART_OUT + UartAct::GetInst(hsmn));
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *hal) {
    Hsmn hsmn = UartAct::GetHsmn(hal);
    UartIn::DmaCompleteCallback(UART_IN + UartAct::GetInst(hsmn));
}

extern "C" void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *hal) {
    Hsmn hsmn = UartAct::GetHsmn(hal);
    UartIn::DmaHalfCompleteCallback(UART_IN + UartAct::GetInst(hsmn));
}

// Define UART configurations.
UartAct::Config const UartAct::CONFIG[] = {
    { UART1_ACT, USART1, USART1_IRQn, USART1_IRQ_PRIO,
      GPIOB, GPIO_PIN_6, GPIO_AF7_USART1, DMA2_Channel6, DMA_REQUEST_2, DMA2_Channel6_IRQn, DMA2_CHANNEL6_PRIO,
      GPIOB, GPIO_PIN_7, GPIO_AF7_USART1, DMA2_Channel7, DMA_REQUEST_2, DMA2_Channel7_IRQn, DMA2_CHANNEL7_PRIO },
    // Add more configurations here...
};

// Corresponds to what was done in _msp.cpp file.
void UartAct::InitUart() {
    switch((uint32_t)(m_config->uart)) {
        case USART2_BASE: __HAL_RCC_USART2_CLK_ENABLE(); break;
        case USART1_BASE: __HAL_RCC_USART1_CLK_ENABLE(); break;
        // Add more cases here...
        default: FW_ASSERT(0); break;
    }

    // Configure peripheral GPIO
    // UART TX GPIO pin configuration.
    GPIO_InitTypeDef  gpioInit;
    gpioInit.Pin       = m_config->txPin;
    gpioInit.Mode      = GPIO_MODE_AF_PP;
    gpioInit.Pull      = GPIO_PULLUP;
    gpioInit.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpioInit.Alternate = m_config->txAf;
    HAL_GPIO_Init(m_config->txPort, &gpioInit);
    // UART RX GPIO pin configuration
    gpioInit.Pin       = m_config->rxPin;
    gpioInit.Alternate = m_config->rxAf;
    HAL_GPIO_Init(m_config->rxPort, &gpioInit);

    // Configure the DMA
    // Configure the DMA handler for Transmission process
    m_txDmaHandle.Instance                 = m_config->txDmaCh;
    m_txDmaHandle.Init.Request             = m_config->txDmaReq;
    m_txDmaHandle.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    m_txDmaHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
    m_txDmaHandle.Init.MemInc              = DMA_MINC_ENABLE;
    m_txDmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    m_txDmaHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    m_txDmaHandle.Init.Mode                = DMA_NORMAL;
    m_txDmaHandle.Init.Priority            = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&m_txDmaHandle);
    // Associate the initialized DMA handle to the UART handle
    __HAL_LINKDMA(&m_hal, hdmatx, m_txDmaHandle);
    // Configure the DMA handler forGPIO_InitStruct reception process
    m_rxDmaHandle.Instance                 = m_config->rxDmaCh;
    m_rxDmaHandle.Init.Request             = m_config->rxDmaReq;
    m_rxDmaHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    m_rxDmaHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
    m_rxDmaHandle.Init.MemInc              = DMA_MINC_ENABLE;
    m_rxDmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    m_rxDmaHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    m_rxDmaHandle.Init.Mode                = DMA_CIRCULAR;
    m_rxDmaHandle.Init.Priority            = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&m_rxDmaHandle);
    // Associate the initialized DMA handle to the the UART handle
    __HAL_LINKDMA(&m_hal, hdmarx, m_rxDmaHandle);

    // Configure the NVIC for DMA
    // NVIC for DMA TX
    NVIC_SetPriority(m_config->txDmaIrq, m_config->txDmaPrio);
    NVIC_EnableIRQ(m_config->txDmaIrq);
    // NVIC for DMA RX
    NVIC_SetPriority(m_config->rxDmaIrq, m_config->rxDmaPrio);
    NVIC_EnableIRQ(m_config->rxDmaIrq);
    // NVIC for USART
    NVIC_SetPriority(m_config->uartIrq, m_config->uartPrio);
    NVIC_EnableIRQ(m_config->uartIrq);
}

void UartAct::DeInitUart() {
    switch((uint32_t)(m_hal.Instance)) {
        case USART2_BASE: __HAL_RCC_USART2_FORCE_RESET(); __HAL_RCC_USART2_RELEASE_RESET(); __HAL_RCC_USART2_CLK_DISABLE(); break;
        case USART1_BASE: __HAL_RCC_USART1_FORCE_RESET(); __HAL_RCC_USART1_RELEASE_RESET(); __HAL_RCC_USART1_CLK_DISABLE(); break;
        // Add more cases here...
        default: FW_ASSERT(0); break;
    }

    // 2- Disable peripherals and GPIO Clocks
    HAL_GPIO_DeInit(m_config->txPort, m_config->txPin);
    HAL_GPIO_DeInit(m_config->rxPort, m_config->rxPin);
}

UartAct::UartAct(Hsmn hsmn, char const *name, char const *inName, char const *outName) :
    Active((QStateHandler)&UartAct::InitialPseudoState, hsmn, name),
    m_config(NULL),
    m_uartInHsmn(GetUartInHsmn(hsmn)),
    m_uartOutHsmn(GetUartOutHsmn(hsmn)),
    m_uartIn(m_uartInHsmn, inName, m_hal),
    m_uartOut(m_uartOutHsmn, outName, m_hal),
    m_client(HSM_UNDEF), m_outFifo(NULL), m_inFifo(NULL), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(hsmn, STATE_TIMER) {
    SET_EVT_NAME(UART_ACT);
    FW_ASSERT((hsmn >= UART_ACT) && (hsmn <= UART_ACT_LAST));
    memset(&m_hal, 0, sizeof(m_hal));
    memset(&m_txDmaHandle, 0, sizeof(m_txDmaHandle));
    memset(&m_rxDmaHandle, 0, sizeof(m_rxDmaHandle));
    uint32_t i;
    for (i = 0; i < ARRAY_COUNT(CONFIG); i++) {
        if (CONFIG[i].hsmn == hsmn) {
            m_config = &CONFIG[i];
            break;
        }
    }
    FW_ASSERT(i < ARRAY_COUNT(CONFIG));
    // Save hsmn to HAL mapping.
    SaveHal(hsmn, &m_hal);
}

QState UartAct::InitialPseudoState(UartAct * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&UartAct::Root);
}

QState UartAct::Root(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_uartIn.Init(me);
            me->m_uartOut.Init(me);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&UartAct::Stopped);
            break;
        }
        case UART_ACT_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new UartActStartCfm(ERROR_STATE, me->GetHsmn()), req);
            status = Q_HANDLED();
            break;
        }
        case UART_ACT_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            status = Q_TRAN(&UartAct::Stopping);
            break;
        }
        default: {
            status = Q_SUPER(&QHsm::top);
            break;
        }
    }
    return status;
}

QState UartAct::Stopped(UartAct * const me, QEvt const * const e) {
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
        case UART_ACT_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new UartActStopCfm(ERROR_SUCCESS), req);
            status = Q_HANDLED();
            break;
        }
        case UART_ACT_START_REQ: {
            EVENT(e);
            UartActStartReq const &req = static_cast<UartActStartReq const &>(*e);
            me->InitUart();
            // For now serial configuration is fixed. In the future, it can be passed in with START_REQ.
            me->m_hal.Instance = me->m_config->uart;
            me->m_hal.Init.BaudRate = 115200;
            me->m_hal.Init.WordLength = UART_WORDLENGTH_8B;
            me->m_hal.Init.StopBits = UART_STOPBITS_1;
            me->m_hal.Init.Parity = UART_PARITY_NONE;
            me->m_hal.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
            me->m_hal.Init.Mode = UART_MODE_TX_RX;
            HAL_StatusTypeDef halStatus = HAL_UART_Init(&me->m_hal);
            if(halStatus == HAL_OK) {
                me->m_client = req.GetFrom();
                me->m_outFifo = req.GetOutFifo();
                me->m_inFifo = req.GetInFifo();
                me->m_inEvt = req;
                me->Raise(new Evt(START));
            } else {
                ERROR("HAL_UART_Init failed(%d", halStatus);
                me->SendCfm(new UartActStartCfm(ERROR_HAL, me->GetHsmn()), req);
            }
            status = Q_HANDLED();
            break;
        }
        case START: {
            EVENT(e);
            status = Q_TRAN(&UartAct::Starting);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Root);
            break;
        }
    }
    return status;
}

QState UartAct::Starting(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = UartActStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > UartInStartReq::TIMEOUT_MS);
            FW_ASSERT(timeout > UartOutStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            FW_ASSERT(me->m_outFifo && me->m_inFifo);
            me->SendReq(new UartOutStartReq(me->m_outFifo, me->m_client), me->m_uartOutHsmn, true);
            me->SendReq(new UartInStartReq(me->m_inFifo, me->m_client), me->m_uartInHsmn, false);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            status = Q_HANDLED();
            break;
        }
        case UART_OUT_START_CFM:
        case UART_IN_START_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Fail(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            status = Q_HANDLED();
            break;
        }
        case FAIL:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAIL) {
                ErrorEvt const &fail = ERROR_EVT_CAST(*e);
                me->SendCfm(new UartActStartCfm(fail.GetError(), fail.GetOrigin(), fail.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new UartActStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            status = Q_TRAN(&UartAct::Stopping);
            break;
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new UartActStartCfm(ERROR_SUCCESS), me->m_inEvt);
            status = Q_TRAN(&UartAct::Started);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Root);
            break;
        }
    }
    return status;
}

QState UartAct::Stopping(UartAct * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = UartActStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > UartInStopReq::TIMEOUT_MS);
            FW_ASSERT(timeout > UartOutStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new UartInStopReq(), me->m_uartInHsmn, true);
            me->SendReq(new UartOutStopReq(), me->m_uartOutHsmn, false);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            status = Q_HANDLED();
            break;
        }
        case UART_ACT_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            status = Q_HANDLED();
            break;
        }
        case UART_IN_STOP_CFM:
        case UART_OUT_STOP_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Fail(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            status = Q_HANDLED();
            break;
        }
        case FAIL:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            status = Q_HANDLED();
            break;
        }
        case DONE: {
            EVENT(e);
            HAL_UART_DeInit(&me->m_hal);
            me->DeInitUart();
            status = Q_TRAN(&UartAct::Stopped);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Root);
            break;
        }
    }
    return status;
}

QState UartAct::Started(UartAct * const me, QEvt const * const e) {
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
            status = Q_TRAN(&UartAct::Normal);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Root);
            break;
        }
    }
    return status;
}

QState UartAct::Normal(UartAct * const me, QEvt const * const e) {
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
        case UART_IN_FAIL_IND:
        case UART_OUT_FAIL_IND: {
            EVENT(e);
            ErrorEvt const &ind = ERROR_EVT_CAST(*e);
            Evt *evt = new UartActFailInd(ind.GetError(), ind.GetOrigin(), ind.GetReason());
            me->Send(evt, me->m_client);
            status = Q_TRAN(&UartAct::Failed);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::Started);
            break;
        }
    }
    return status;
}

QState UartAct::Failed(UartAct * const me, QEvt const * const e) {
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
            status = Q_SUPER(&UartAct::Started);
            break;
        }
    }
    return status;
}

/*
QState UartAct::MyState(UartAct * const me, QEvt const * const e) {
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
            status = Q_TRAN(&UartAct::SubState);
            break;
        }
        default: {
            status = Q_SUPER(&UartAct::SuperState);
            break;
        }
    }
    return status;
}
*/

} // namespace APP
