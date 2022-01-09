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

#ifndef ILI9341_H
#define ILI9341_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "gfxfont.h"
#include "Disp.h"

using namespace QP;
using namespace FW;

namespace APP {

class Ili9341 : public Disp {
public:
    static SPI_HandleTypeDef *GetHal() { return &m_hal; }
    static Hsmn GetHsmn() { return ILI9341; }
    static void SignalSpiSem() { m_spiSem.signal(); }

    Ili9341(XThread &container);

protected:
    static QState InitialPseudoState(Ili9341 * const me, QEvt const * const e);
    static QState Root(Ili9341 * const me, QEvt const * const e);
        static QState Stopped(Ili9341 * const me, QEvt const * const e);
        static QState Started(Ili9341 * const me, QEvt const * const e);
            static QState Idle(Ili9341 * const me, QEvt const * const e);
            static QState Busy(Ili9341 * const me, QEvt const * const e);

    void InitSpi();
    void DeInitSpi();
    bool InitHal();
    void DeInitHal();
    bool SpiWriteDma(uint8_t const *buf, uint16_t len);
    bool SpiReadDma(uint8_t *buf, uint16_t len);
    void WriteCmd(uint8_t cmd);
    void WriteDataBuf(uint8_t const *buf, uint16_t len);
    void WriteData1(uint8_t b0);
    void WriteData2(uint8_t b0, uint8_t b1);
    void WriteData3(uint8_t b0, uint8_t b1, uint8_t b2);
    void WriteData4(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);
    void InitDisp();
    void SetAddrWindow(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h);
    void PushColor(uint16_t color) { WriteData2(BYTE_1(color), BYTE_0(color)); }
    void PushColor(uint16_t color, uint32_t pixelCnt);

    uint16_t GetWidth() override { return m_width; }
    uint16_t GetHeight() override { return m_height; }
    void SetRotation(uint8_t rotation) override;
    void WritePixel(int16_t x, int16_t y, uint16_t color) override;
    void FillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color) override;
    void WriteBitmap(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t *buf, uint32_t len) override;


    Hsmn m_client;
    Timer m_stateTimer;

    class Config {
    public:
        // Key
        Hsmn hsmn;
        uint16_t width;
        uint16_t height;
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

        // ILI9341 D/CX pin parameters (Data/Command)
        GPIO_TypeDef *dcPort;
        uint32_t dcPin;

        // SPI Tx DMA parameters.
        DMA_Channel_TypeDef *txDmaCh;
        uint32_t txDmaReq;
        IRQn_Type txDmaIrq;
        uint32_t txDmaPrio;

        // SPI Rx DMA parameters.
        DMA_Channel_TypeDef *rxDmaCh;
        uint32_t rxDmaReq;
        IRQn_Type rxDmaIrq;
        uint32_t rxDmaPrio;
    };

    static Config const CONFIG[];
    Config const *m_config;
    static SPI_HandleTypeDef m_hal;
    DMA_HandleTypeDef m_txDmaHandle;
    DMA_HandleTypeDef m_rxDmaHandle;
    XThread &m_container;              // Its type needs to be XThread rather than the base class QActive in order to call XThread methods (delay).
    static QXSemaphore m_spiSem;       // Binary semaphore to signal SPI read/write completion.

    uint16_t m_width;                  // After rotation effect.
    uint16_t m_height;                 // After rotation effect.

    enum {
        BUFFER_SIZE = 10 * 240 * sizeof(uint16_t)
    };
    uint8_t m_buffer[BUFFER_SIZE];

};

} // namespace APP

#endif // ILI9341_H
