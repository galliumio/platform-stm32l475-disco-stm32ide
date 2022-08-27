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

#include <stdlib.h>
#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "fw_error.h"
#include "SystemInterface.h"
#include "WifiInterface.h"
#include "WifiThread.h"
#include "Wifi.h"
#include "es_wifi_io.h"         // In current folder.

FW_DEFINE_THIS_FILE("Wifi.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "WIFI_TIMER_EVT_START",
    WIFI_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "WIFI_INTERNAL_EVT_START",
    WIFI_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "WIFI_INTERFACE_EVT_START",
    WIFI_INTERFACE_EVT
};

// Define SPI and interrupt configurations.
Wifi::Config const Wifi::CONFIG[] = {
    {
      WIFI, SPI3, SPI3_IRQn, SPI3_PRIO,
      GPIOC, GPIO_PIN_10, GPIOC, GPIO_PIN_11, GPIOC, GPIO_PIN_12, GPIO_AF6_SPI3,    // SCK, MISO, MOSI spi pins.
      GPIOE, GPIO_PIN_0, GPIOB, GPIO_PIN_13, GPIOE, GPIO_PIN_8,                     // CS, Wakeup and Reset output pins.
      EXTI1_IRQn, EXTI1_PRIO, GPIOE, GPIO_PIN_1                                     // CmdDataReady input pin.
    }
};
// Only support single instance.
Wifi::Config const *Wifi::m_config;
SPI_HandleTypeDef Wifi::m_hal;
QXSemaphore Wifi::m_spiSem;
QXSemaphore Wifi::m_cmdDataRdySem;
Pin Wifi::m_spiSck;
Pin Wifi::m_spiMiso;
Pin Wifi::m_spiMosi;
Pin Wifi::m_spiCs;
Pin Wifi::m_wakeupPin;
Pin Wifi::m_resetPin;
Pin Wifi::m_cmdDataRdyPin;

// Credentials defined outside this file.
extern char const *wifiSsid;
extern char const *wifiPwd;

void Wifi::ResetModule() {
    m_resetPin.Clear();
    DelayMs(10);
    m_resetPin.Set();
    DelayMs(500);
}

void Wifi::EnableCs() {
    m_spiCs.Clear();
}

void Wifi::DisableCs() {
    m_spiCs.Set();
}


bool Wifi::IsCmdDataReady() {
    return m_cmdDataRdyPin.IsSet();
}

void Wifi::FlushSpiRxFifo() {
    HAL_SPIEx_FlushRxFifo(&m_hal);
}

// @param[in] buf - Pointer to the beginning of data buffer to write.
// @param[in] len - No. of 16-bit items to write.
// @param[in] waitMs - Max wait time in ms.
// @remark CS is controlled externally in es_wifi_io.cpp.
//         It is assumed SPI bus is only used by Wifi; otherwise needs mutex (in es_wifi_io.cpp).
bool Wifi::SpiWriteInt(uint8_t *buf, uint16_t len, uint32_t waitMs) {
    FW_ASSERT(buf);
    bool status = false;
    if (HAL_SPI_Transmit_IT(&m_hal, buf, len) == HAL_OK) {
        status = m_spiSem.wait(BSP_MSEC_TO_TICK(waitMs));
    }
    return status;
}

// @param[in] buf - Pointer to the beginning of buffer to store received data.
// @param[in] len - No. of 16-bit items to read.
// @param[in] waitMs - Max wait time in ms.
// @remark CS is controlled externally in es_wifi_io.cpp.
//         It is assumed SPI bus is only used by Wifi; otherwise needs mutex (in es_wifi_io.cpp).
bool Wifi::SpiReadInt(uint8_t *buf, uint16_t len, uint32_t waitMs) {
    FW_ASSERT(buf);
    bool status = false;
    if (HAL_SPI_Receive_IT(&m_hal, buf, len) == HAL_OK) {
        status = m_spiSem.wait(BSP_MSEC_TO_TICK(waitMs));
    }
    return status;
}

void Wifi::DelayMs(uint32_t ms) {
    XThread::DelayMs(ms);
}

bool Wifi::WaitCmdDataRdyHigh(uint32_t waitMs) {
    return m_cmdDataRdySem.wait(BSP_MSEC_TO_TICK(waitMs));
}

void Wifi::ClearCmdDataRdySem() {
    m_cmdDataRdySem.tryWait();
}


void Wifi::InitSpi() {
    FW_ASSERT(m_config);
    // GPIO clocks enabled in periph.cpp
    // Enable SPI clock
    switch((uint32_t)(m_config->spi)) {
        case SPI1_BASE: __HAL_RCC_SPI1_CLK_ENABLE(); __HAL_RCC_SPI1_FORCE_RESET(); __HAL_RCC_SPI1_RELEASE_RESET(); break;
        case SPI2_BASE: __HAL_RCC_SPI2_CLK_ENABLE(); __HAL_RCC_SPI2_FORCE_RESET(); __HAL_RCC_SPI2_RELEASE_RESET(); break;
        case SPI3_BASE: __HAL_RCC_SPI3_CLK_ENABLE(); __HAL_RCC_SPI3_FORCE_RESET(); __HAL_RCC_SPI3_RELEASE_RESET(); break;
        // Add more cases here...
        default: FW_ASSERT(0); break;
    }

    // Note cmdDataRdy is set to interrupt on rising edge.
    // spiMiso is set to pull-up.
    m_wakeupPin.Init(m_config->wakeupPort, m_config->wakeupPin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 0, GPIO_SPEED_FREQ_LOW);
    m_wakeupPin.Clear();
    m_cmdDataRdyPin.Init(m_config->cmdDataRdyPort, m_config->cmdDataRdyPin, GPIO_MODE_IT_RISING, GPIO_NOPULL, 0, GPIO_SPEED_FREQ_LOW);
    m_resetPin.Init(m_config->resetPort, m_config->resetPin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 0, GPIO_SPEED_FREQ_LOW);

    m_spiCs.Init(m_config->csPort, m_config->csPin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 0, GPIO_SPEED_FREQ_MEDIUM);
    m_spiCs.Set();
    m_spiSck.Init(m_config->sckPort, m_config->sckPin, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF6_SPI3, GPIO_SPEED_FREQ_MEDIUM);
    m_spiMosi.Init(m_config->mosiPort, m_config->mosiPin, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF6_SPI3, GPIO_SPEED_FREQ_MEDIUM);
    m_spiMiso.Init(m_config->misoPort, m_config->misoPin, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_AF6_SPI3, GPIO_SPEED_FREQ_MEDIUM);

    // Configure NVIC.
    NVIC_SetPriority(m_config->spiIrq, m_config->spiPrio);
    NVIC_EnableIRQ(m_config->spiIrq);
    NVIC_SetPriority(m_config->cmdDataRdyIrq, m_config->cmdDataRdyPrio);
    NVIC_EnableIRQ(m_config->cmdDataRdyIrq);
}

void Wifi::DeInitSpi() {
    // Deactivates NSS in case it is still activated since it is NOT deactivated at the end of SPI_WIFI_SendData().
    m_spiCs.Set();
    switch((uint32_t)(m_config->spi)) {
        case SPI1_BASE: __HAL_RCC_SPI1_FORCE_RESET(); __HAL_RCC_SPI1_RELEASE_RESET(); __HAL_RCC_SPI1_CLK_DISABLE(); break;
        case SPI2_BASE: __HAL_RCC_SPI2_FORCE_RESET(); __HAL_RCC_SPI2_RELEASE_RESET(); __HAL_RCC_SPI2_CLK_DISABLE(); break;
        case SPI3_BASE: __HAL_RCC_SPI3_FORCE_RESET(); __HAL_RCC_SPI3_RELEASE_RESET(); __HAL_RCC_SPI3_CLK_DISABLE(); break;
        // Add more cases here...
        default: FW_ASSERT(0); break;
    }
    m_wakeupPin.DeInit();
    m_cmdDataRdyPin.DeInit();
    m_resetPin.DeInit();
    m_spiCs.DeInit();
    m_spiSck.DeInit();
    m_spiMosi.DeInit();
    m_spiMiso.DeInit();
    NVIC_DisableIRQ(m_config->spiIrq);
    NVIC_DisableIRQ(m_config->cmdDataRdyIrq);
}

bool Wifi::InitHal() {
    FW_ASSERT(m_config);
    m_hal.Instance               = m_config->spi;
    m_hal.Init.Mode              = SPI_MODE_MASTER;
    m_hal.Init.Direction         = SPI_DIRECTION_2LINES;
    m_hal.Init.DataSize          = SPI_DATASIZE_16BIT;
    m_hal.Init.CLKPolarity       = SPI_POLARITY_LOW;
    m_hal.Init.CLKPhase          = SPI_PHASE_1EDGE;
    m_hal.Init.NSS               = SPI_NSS_SOFT;
    m_hal.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;  /* 80/8= 10MHz (Inventek WIFI module supportes up to 20MHz)*/
    m_hal.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    m_hal.Init.TIMode            = SPI_TIMODE_DISABLE;
    m_hal.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    m_hal.Init.CRCPolynomial     = 0;
    return HAL_SPI_Init(&m_hal) == HAL_OK;
}

void Wifi::DeInitHal() {
    HAL_StatusTypeDef status = HAL_SPI_DeInit(&m_hal);
    FW_ASSERT(status == HAL_OK);
}

// Converts an IP addr string to uint32_t.
bool Wifi::Iptoul(char const *ipAddr, uint32_t &result) {
    FW_ASSERT(ipAddr);
    char addr[20];
    STRBUF_COPY(addr, ipAddr);

    char *start = addr;
    char *end;
    result = 0;
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t octet = strtoul(start, &end, 10);
        if ((start == end) || (octet > 0xff) || !((i == 3 && *end == 0) || (i < 3 && *end == '.'))) {
            return false;
        }
        result = (result << 8) | octet;
        if (end) {
            start = end + 1;
        }
    }
    return true;
}


Wifi::Wifi(XThread &container) :
    Region((QStateHandler)&Wifi::InitialPseudoState, WIFI, "WIFI"),
    m_client(HSM_UNDEF), m_stateTimer(GetHsmn(), STATE_TIMER),
    m_retryTimer(GetHsmn(), RETRY_TIMER), m_dataPollTimer(GetHsmn(), DATA_POLL_TIMER),
    m_container(container), m_port(0),
    m_dataOutFifo(nullptr), m_dataInFifo(nullptr), m_retryCnt(0), m_inEvt(QEvt::STATIC_EVT) {
    FW_ASSERT(CONFIG[0].hsmn == GetHsmn());
    m_config = &CONFIG[0];
    SET_EVT_NAME(WIFI);
    m_spiSem.init(0,1);
    m_cmdDataRdySem.init(0,1);
    memset(&m_hal, 0, sizeof(m_hal));
    memset(&m_domain, 0, sizeof(m_domain));
    memset(&m_macAddr, 0, sizeof(m_macAddr));
}

QState Wifi::InitialPseudoState(Wifi * const me, QEvt const * const e) {
    (void)e;
    // Initializes any regions here (currently none).
    // We need to use m_container rather than GetHsm().GetContainer() since we need the derived type XThread rather than QActive.
    return Q_TRAN(&Wifi::Root);
}

QState Wifi::Root(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi::Stopped);
        }
        case WIFI_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WifiStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case WIFI_CONNECT_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WifiConnectCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case WIFI_DISCONNECT_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WifiDisconnectCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case WIFI_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Wifi::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Wifi::Stopped(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case WIFI_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new WifiStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case WIFI_START_REQ: {
            EVENT(e);
            WifiStartReq const &req = static_cast<WifiStartReq const &>(*e);
            me->m_client = req.GetFrom();
            me->m_inEvt = req;
            return Q_TRAN(&Wifi::Starting);
        }
    }
    return Q_SUPER(&Wifi::Root);
}

QState Wifi::Starting(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = WifiStartReq::TIMEOUT_MS;
            me->m_stateTimer.Start(timeout);
            // @todo Currently placeholder. Sends DONE immediately. Do not use Raise() in entry action.
            me->Send(new Evt(DONE), me->GetHsmn());
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new WifiStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new WifiStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&Wifi::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new WifiStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Wifi::Started);
        }
    }
    return Q_SUPER(&Wifi::Root);
}

QState Wifi::Stopping(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = WifiStopReq::TIMEOUT_MS;
            me->m_stateTimer.Start(timeout);
            // @todo Currently placeholder. Sends DONE immediately. Do not use Raise() in entry action.
            me->Send(new Evt(DONE), me->GetHsmn());
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case WIFI_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi::Stopped);
        }
    }
    return Q_SUPER(&Wifi::Root);
}

QState Wifi::Started(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->DeInitHal();
            me->DeInitSpi();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi::Idle);
        }
        case FAULT_EVT: {
            EVENT(e);
            return Q_TRAN(&Wifi::Fault);
        }
    }
    return Q_SUPER(&Wifi::Root);
}

QState Wifi::Idle(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_retryCnt = 0;
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi::IdleNormal);
        }
        case WIFI_CONNECT_REQ: {
            EVENT(e);
            WifiConnectReq const &req = static_cast<WifiConnectReq const &>(*e);
            me->m_inEvt = req;
            STRBUF_COPY(me->m_domain, req.GetDomain());
            me->m_port = req.GetPort();
            me->m_dataOutFifo = req.GetDataOutFifo();
            me->m_dataInFifo = req.GetDataInFifo();
            FW_ASSERT(me->m_dataOutFifo && me->m_dataInFifo);
            return Q_TRAN(&Wifi::ConnectWait);
        }
    }
    return Q_SUPER(&Wifi::Started);
}

QState Wifi::IdleNormal(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->InitSpi();
            if (!me->InitHal()) {
                ERROR("InitHal() failed");
                FW_ASSERT(0);
            }
            memset(&me->m_esWifiObj, 0, sizeof(me->m_esWifiObj));
            ES_WIFI_Status_t status = ES_WIFI_RegisterBusIO(&me->m_esWifiObj, SPI_WIFI_Init, SPI_WIFI_DeInit,
                                                            SPI_WIFI_Delay, SPI_WIFI_SendData, SPI_WIFI_ReceiveData);
            FW_ASSERT(status == ES_WIFI_STATUS_OK);
            if ((ES_WIFI_Init(&me->m_esWifiObj) != ES_WIFI_STATUS_OK) ||
                (ES_WIFI_GetMACAddress(&me->m_esWifiObj, me->m_macAddr) != ES_WIFI_STATUS_OK)) {
                ERROR("ES_WIFI_Init or ES_WIFI_GetMACAddress failed");
                me->Raise(new Evt(INIT_FAILED));
            } else {
                LOG("ES_WIFI_Init() success");
                LOG("WIFI MAC Address : %X:%X:%X:%X:%X:%X",
                    me->m_macAddr[0], me->m_macAddr[1], me->m_macAddr[2],
                    me->m_macAddr[3], me->m_macAddr[4], me->m_macAddr[5]);
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case INIT_FAILED: {
            EVENT(e);
            return Q_TRAN(&Wifi::IdleRetry);
        }
    }
    return Q_SUPER(&Wifi::Idle);
}

QState Wifi::IdleRetry(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->DeInitHal();
            me->DeInitSpi();
            me->m_retryTimer.Start(RETRY_INIT_TIMEOUT_MS);
            if (++me->m_retryCnt > 2) {
                me->Raise(new FaultEvt(ERROR_HARDWARE, me->GetHsmn()));
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_retryTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case WIFI_CONNECT_REQ:
        case WIFI_DISCONNECT_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case RETRY_TIMER: {
            EVENT(e);
            return Q_TRAN(&Wifi::IdleNormal);
        }
    }
    return Q_SUPER(&Wifi::Idle);
}

QState Wifi::Fault(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi::Started);
}

QState Wifi::Running(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_dataOutFifo = nullptr;
            me->m_dataInFifo = nullptr;
            me->DeInitHal();
            me->DeInitSpi();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi::ConnectWait);
        }
        case WIFI_DISCONNECT_REQ:
        case WIFI_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Wifi::DisconnectWait);
        }
        case DISCONNECTED: {
            EVENT(e);
            me->Send(new WifiDisconnectInd(), me->m_client);
            return Q_TRAN(&Wifi::Idle);
        }
    }
    return Q_SUPER(&Wifi::Started);
}

QState Wifi::ConnectWait(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_stateTimer.Start(CONNECT_WAIT_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi::Joining);
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new WifiConnectCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new WifiConnectCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }
            return Q_TRAN(&Wifi::DisconnectWait);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new WifiConnectCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Wifi::Connected);
        }
    }
    return Q_SUPER(&Wifi::Running);
}

QState Wifi::Joining(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            ES_WIFI_APs_t esWifiAPs;
            uint32_t apIdx = (uint32_t)(-1);
            if(ES_WIFI_ListAccessPoints(&me->m_esWifiObj, &esWifiAPs) == ES_WIFI_STATUS_OK) {
                LOG("WIFI No. of AP = %d", esWifiAPs.nbr);
                for(uint32_t i = 0; i < esWifiAPs.nbr; i++) {
                    LOG("%s (rssi=%d, ch=%d)", (char *)esWifiAPs.AP[i].SSID, esWifiAPs.AP[i].RSSI, esWifiAPs.AP[i].Channel);
                    if (STRING_EQUAL((char *)esWifiAPs.AP[i].SSID, wifiSsid)) {
                        apIdx = i;
                    }
                }
                if (apIdx < ARRAY_COUNT(esWifiAPs.AP)) {
                    LOG("Joining SSID %s...", (char *)esWifiAPs.AP[apIdx].SSID);
                    if (ES_WIFI_Connect(&me->m_esWifiObj, wifiSsid , wifiPwd, ES_WIFI_SEC_WPA2) == ES_WIFI_STATUS_OK)
                    {
                        LOG("WIFI joined");
                        // It does not use "raise()" to allow WIFI_STOP_REQ or WIFI_DISCONNECT_REQ to be processed first.
                        // Otherwise the two synchronous calls ES_WIFI_Connect() and ES_WIFI_StartClientConnection() will be
                        // called back to back without any events to be processed in between.
                        // The worst case is when it takes a long time to join an access point but the server is down which
                        // causes ES_WIFI_StartClientConnection() to return after a long time.
                        me->Send(new Evt(DONE), me->GetHsmn());
                    } else {
                        ERROR("ES_WIFI_Connect failed");
                        me->Raise(new Failed(ERROR_NETWORK, HSM_UNDEF, 0));
                    }
                } else {
                    WARNING("SSID %s not found. Retry...", wifiSsid);
                    me->m_retryTimer.Start(RETRY_SCAN_TIMEOUT_MS);
                }
            } else {
                ERROR("ES_WIFI_ListAccessPoints failed");
                me->Raise(new Failed(ERROR_NETWORK, HSM_UNDEF, 0));
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_retryTimer.Stop();
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi::Connecting);
        }
        case RETRY_TIMER: {
            EVENT(e);
            return Q_TRAN(&Wifi::Joining);
        }
    }
    return Q_SUPER(&Wifi::ConnectWait);
}

QState Wifi::Connecting(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t ipAddr;
            bool status = me->Iptoul(me->m_domain, ipAddr);
            FW_ASSERT(status);
            ES_WIFI_Conn_t conn;
            memset(&conn, 0, sizeof(conn));
            conn.Number = SOCKET_NUM;                // Use socket 0 (0-3).
            conn.RemotePort = me->m_port;
            conn.LocalPort = LOCAL_PORT;
            conn.Type = ES_WIFI_TCP_CONNECTION;
            conn.RemoteIP[0] = BYTE_3(ipAddr);
            conn.RemoteIP[1] = BYTE_2(ipAddr);
            conn.RemoteIP[2] = BYTE_1(ipAddr);
            conn.RemoteIP[3] = BYTE_0(ipAddr);
            if(ES_WIFI_StartClientConnection(&me->m_esWifiObj, &conn)== ES_WIFI_STATUS_OK)
            {
                LOG("ES_WIFI_StartClientConnection WIFI_STATUS_OK");
                me->Raise(new Evt(DONE));
            } else {
                ERROR("ES_WIFI_StartClientConnection failed");
                me->Raise(new Failed(ERROR_NETWORK, HSM_UNDEF, 0));
            }
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi::ConnectWait);
}

QState Wifi::DisconnectWait(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->Recall();
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi::Disconnecting);
        }
        case WIFI_DISCONNECT_REQ:
        case WIFI_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi::Idle);
        }
    }
    return Q_SUPER(&Wifi::Running);
}

QState Wifi::Disconnecting(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // The following call is commented as it doesn't work when Disconnecting is entered because
            // of a previous timeout error in ConnectWait state. It is not needed anyway since upon DONE
            // it will enter Idle state which will reset the WiFi module. It is retained as a placeholder.
            /*
            ES_WIFI_Conn_t conn;
            memset(&conn, 0, sizeof(conn));
            conn.Number = SOCKET_NUM;
            ES_WIFI_Status_t status = ES_WIFI_StopClientConnection(&me->m_esWifiObj, &conn);
            FW_ASSERT(status == ES_WIFI_STATUS_OK);
            */
            me->m_stateTimer.Start(DISCONNECTING_TIMEOUT_MS);
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case STATE_TIMER: {
            EVENT(e);
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Wifi::Unjoining);
        }
    }
    return Q_SUPER(&Wifi::DisconnectWait);
}

QState Wifi::Unjoining(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            // The following call is commented as it doesn't work when Disconnecting is entered because
            // of a previous timeout error in ConnectWait state. It is not needed anyway since upon DONE
            // it will enter Idle state which will reset the WiFi module. It is retained as a placeholder.
            /*
            ES_WIFI_Status_t status = ES_WIFI_Disconnect(&me->m_esWifiObj);
            FW_ASSERT(status == ES_WIFI_STATUS_OK);
            */
            me->m_stateTimer.Start(UNJOINING_TIMEOUT_MS);
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case STATE_TIMER: {
            EVENT(e);
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Wifi::DisconnectWait);
}

QState Wifi::Connected(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_dataPollTimer.Start(DATA_POLL_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_dataPollTimer.Stop();
            return Q_HANDLED();
        }
        case WIFI_WRITE_REQ: {
            EVENT(e);
            FW_ASSERT(me->m_dataOutFifo);
            bool failed = false;
            while(me->m_dataOutFifo->GetUsedCount()) {
                uint16_t sentLen = 0;
                uint32_t writeLen = me->m_dataOutFifo->GetUsedBlockCount();
                writeLen = MIN(static_cast<uint32_t>(MAX_SEND_LEN), writeLen);
                LOG("Calling ES_WIFI_SendData writeLen=%d", writeLen);
                ES_WIFI_Status_t status = ES_WIFI_SendData(&me->m_esWifiObj, SOCKET_NUM,
                                                           reinterpret_cast<uint8_t *>(me->m_dataOutFifo->GetReadAddr()),
                                                           writeLen, &sentLen, 0);
                LOG("ES_WIFI_SendData return sentLen=%d", sentLen);
                if ((status != ES_WIFI_STATUS_OK) || (writeLen != sentLen)) {
                    failed = true;
                    break;
                }
                me->m_dataOutFifo->IncReadIndex(writeLen);
            }
            if (failed) {
                me->Raise(new Evt(DISCONNECTED));
            } else {
                me->Send(new WifiEmptyInd(), me->m_client);
            }
            return Q_HANDLED();
        }
        case DATA_POLL_TIMER: {
            bool failed = false;
            uint32_t totalLen = 0;
            uint32_t readLen;
            uint16_t recvLen;
            do {
                readLen = me->m_dataInFifo->GetAvailBlockCount();
                if (readLen) {
                    //LOG("Calling ES_WIFI_ReceiveData readLen=%d", readLen);
                    ES_WIFI_Status_t status = ES_WIFI_ReceiveData(&me->m_esWifiObj, SOCKET_NUM,
                                                                  reinterpret_cast<uint8_t *>(me->m_dataInFifo->GetWriteAddr()),
                                                                  readLen, &recvLen, 0);
                    //LOG("ES_WIFI_ReceiveData return recvLen=%d", recvLen);
                    if (status != ES_WIFI_STATUS_OK) {
                        failed = true;
                        break;
                    }
                    me->m_dataInFifo->IncWriteIndex(recvLen);
                    totalLen += recvLen;
                }
            } while(readLen && (recvLen == readLen));
            if (failed) {
                me->Raise(new Evt(DISCONNECTED));
            } else if (totalLen) {
                me->Send(new WifiDataInd(), me->m_client);
            }
            me->m_dataPollTimer.Start(DATA_POLL_TIMEOUT_MS);

            // Test only
            /*
            uint8_t buf[512];
            uint16_t recvLen;
            while(1) {
                ES_WIFI_Status_t status = ES_WIFI_ReceiveData(&me->m_esWifiObj, SOCKET_NUM, buf, sizeof(buf), &recvLen, 0);
                if (status == ES_WIFI_STATUS_OK) {
                    if (recvLen) {
                        Log::DebugBuf(Log::TYPE_LOG, WIFI, buf, recvLen, 1, 0);
                        continue;
                    }
                } else {
                    ERROR("ES_WIFI_ReceiveData failed");
                }
                break;
            }
            */
        }
    }
    return Q_SUPER(&Wifi::Running);
}

/*
QState Wifi::MyState(Wifi * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Wifi::SubState);
        }
    }
    return Q_SUPER(&Wifi::SuperState);
}
*/


} // namespace APP
