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

#ifndef WIFI_H
#define WIFI_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_xthread.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "fw_pipe.h"
#include "app_hsmn.h"
#include "Pin.h"
#include "es_wifi.h"            // In system/BSP...
#include "WifiInterface.h"

using namespace QP;
using namespace FW;

namespace APP {

class Wifi : public Region {
public:
    Wifi(XThread &container);
    // Only supports single instance.
    static SPI_HandleTypeDef *GetHal() { return &m_hal; }
    static void SignalSpiSem() { m_spiSem.signal(); }
    static void SignalCmdDataRdySem() { m_cmdDataRdySem.signal(); }
    // Called from BSP hooks.
    static void ResetModule();
    static void EnableCs();
    static void DisableCs();
    static bool IsCmdDataReady();
    static void FlushSpiRxFifo();
    static bool SpiWriteInt(uint8_t *buf, uint16_t len, uint32_t waitMs = 1000);
    static bool SpiReadInt(uint8_t *buf, uint16_t len, uint32_t waitMs = 1000);
    static void DelayMs(uint32_t ms);
    static bool WaitCmdDataRdyHigh(uint32_t waitMs);
    static void ClearCmdDataRdySem();

protected:
    static QState InitialPseudoState(Wifi * const me, QEvt const * const e);
    static QState Root(Wifi * const me, QEvt const * const e);
        static QState Stopped(Wifi * const me, QEvt const * const e);
        static QState Starting(Wifi * const me, QEvt const * const e);
        static QState Stopping(Wifi * const me, QEvt const * const e);
        static QState Started(Wifi * const me, QEvt const * const e);
            static QState Idle(Wifi * const me, QEvt const * const e);
                static QState IdleNormal(Wifi * const me, QEvt const * const e);
                static QState IdleRetry(Wifi * const me, QEvt const * const e);
            static QState Fault(Wifi * const me, QEvt const * const e);
            static QState Running(Wifi * const me, QEvt const * const e);
                static QState ConnectWait(Wifi * const me, QEvt const * const e);
                    static QState Joining(Wifi * const me, QEvt const * const e);
                    static QState Connecting(Wifi * const me, QEvt const * const e);
                static QState DisconnectWait(Wifi * const me, QEvt const * const e);
                    static QState Disconnecting(Wifi * const me, QEvt const * const e);
                    static QState Unjoining(Wifi * const me, QEvt const * const e);
                static QState Connected(Wifi * const me, QEvt const * const e);


    void InitSpi();
    void DeInitSpi();
    bool InitHal();
    void DeInitHal();
    bool Iptoul(char const *ipAddr, uint32_t &result);

    class Config {
    public:
        // Key
        Hsmn hsmn;
        SPI_TypeDef *spi;

        // SPI interrupt parameters.
        IRQn_Type spiIrq;
        uint32_t spiPrio;

        // SPI SCK/MISO/MOSI parameters.
        GPIO_TypeDef *sckPort;
        uint32_t sckPin;
        GPIO_TypeDef *misoPort;
        uint32_t misoPin;
        GPIO_TypeDef *mosiPort;
        uint32_t mosiPin;
        uint32_t spiAf;

        // SPI CS parameters.
        GPIO_TypeDef *csPort;
        uint32_t csPin;

        // Wakeup and Reset output pins
        GPIO_TypeDef *wakeupPort;
        uint32_t wakeupPin;
        GPIO_TypeDef *resetPort;
        uint32_t resetPin;

        // CmdDataReady input pin.
        IRQn_Type cmdDataRdyIrq;
        uint32_t cmdDataRdyPrio;
        GPIO_TypeDef *cmdDataRdyPort;
        uint32_t cmdDataRdyPin;
    };

    // The following static members only support single instance.
    static Config const CONFIG[];
    static Config const *m_config;
    static SPI_HandleTypeDef m_hal;
    static QXSemaphore m_spiSem;        // Binary semaphore to siganl SPI read/write completion.
    static QXSemaphore m_cmdDataRdySem; // Binary semaphore to siganl CmdDataReady going high.

    static Pin m_spiSck;
    static Pin m_spiMiso;
    static Pin m_spiMosi;
    static Pin m_spiCs;
    static Pin m_wakeupPin;
    static Pin m_resetPin;
    static Pin m_cmdDataRdyPin;

    enum {
        CONNECT_WAIT_TIMEOUT_MS = 25000,
        RETRY_INIT_TIMEOUT_MS = 500,
        RETRY_SCAN_TIMEOUT_MS = 500,
        DISCONNECTING_TIMEOUT_MS = 2000,
        UNJOINING_TIMEOUT_MS = 2000,
        DATA_POLL_TIMEOUT_MS = 50,
    };

    enum {
        SOCKET_NUM = 0,
        LOCAL_PORT = 60000,
        MAX_SEND_LEN = ES_WIFI_PAYLOAD_SIZE
    };

    Hsmn m_client;
    Timer m_stateTimer;
    Timer m_retryTimer;
    Timer m_dataPollTimer;

    XThread &m_container;               // Its type needs to be XThread rather than the base class QActive in order to initialize
                                        // its composed regions (below).
    ES_WIFIObject_t m_esWifiObj;        // Device object for the es_wifi driver provided by ST BSP.
    // Add orthogonal regions here.
    char m_domain[WifiConnectReq::DOMAIN_LEN];  // Domain or IP address string of remote server.
    uint16_t m_port;                            // Port of remote server.
    Fifo *m_dataOutFifo;
    Fifo *m_dataInFifo;
    uint8_t m_macAddr[6];
    uint32_t m_retryCnt;
    Evt m_inEvt;                        // Static event copy of a generic incoming req to be confirmed. Added more if needed.

protected:

#define WIFI_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(RETRY_TIMER) \
    ADD_EVT(DATA_POLL_TIMER)

#define WIFI_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED) \
    ADD_EVT(INIT_FAILED) \
    ADD_EVT(FAULT_EVT) \
    ADD_EVT(DISCONNECTED)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        WIFI_TIMER_EVT_START = TIMER_EVT_START(WIFI),
        WIFI_TIMER_EVT
    };

    enum {
        WIFI_INTERNAL_EVT_START = INTERNAL_EVT_START(WIFI),
        WIFI_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };

    class FaultEvt : public ErrorEvt {
    public:
        FaultEvt(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
            ErrorEvt(FAULT_EVT, error, origin, reason) {}
    };
};

} // namespace APP

#endif // WIFI_H
