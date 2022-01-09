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

/***************************************************
  This is our library for the Adafruit ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "fw_xthread.h"
#include "DispInterface.h"
#include "Ili9341.h"
#include "Ili9341Def.h"

#define PROGMEM
#include "FreeSans24pt7b.h"

FW_DEFINE_THIS_FILE("Ili9341.cpp")

namespace APP {

// Define SPI and interrupt configurations.
Ili9341::Config const Ili9341::CONFIG[] = {
    { ILI9341, 240, 320, SPI1, SPI1_IRQn, SPI1_PRIO,                            // SPI IRQ
      GPIOA, GPIO_PIN_5, GPIOA, GPIO_PIN_6, GPIOA, GPIO_PIN_7, GPIO_AF5_SPI1,   // SPI pins (SCK, MISO, MOSI
      GPIOA, GPIO_PIN_2, GPIOA, GPIO_PIN_15,                                    // CS and D/CX
      DMA2_Channel4, DMA_REQUEST_4, DMA2_Channel4_IRQn, DMA2_CHANNEL4_PRIO,     // TX DMA
      DMA2_Channel3, DMA_REQUEST_4, DMA2_Channel3_IRQn, DMA2_CHANNEL3_PRIO,     // RX DMA
    },
};

SPI_HandleTypeDef Ili9341::m_hal;   // Only support single instance.
QXSemaphore Ili9341::m_spiSem;      // Only support single instance.


void Ili9341::InitSpi() {
    GPIO_InitTypeDef  GPIO_InitStruct;

    // GPIO and DMA clocks enabled in periph.cpp
    // Enable SPI clock
    switch((uint32_t)(m_config->spi)) {
        case SPI1_BASE: __HAL_RCC_SPI1_CLK_ENABLE(); __HAL_RCC_SPI1_FORCE_RESET(); __HAL_RCC_SPI1_RELEASE_RESET(); break;
        case SPI2_BASE: __HAL_RCC_SPI2_CLK_ENABLE(); __HAL_RCC_SPI2_FORCE_RESET(); __HAL_RCC_SPI2_RELEASE_RESET(); break;
        // Add more cases here...
        default: FW_ASSERT(0); break;
    }

    // SPI SCK pin config.
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = m_config->spiAf;
    GPIO_InitStruct.Pin       = m_config->sckPin;
    HAL_GPIO_Init(m_config->sckPort, &GPIO_InitStruct);
    // SPI MISO pin config.
    GPIO_InitStruct.Pin = m_config->misoPin;
    HAL_GPIO_Init(m_config->misoPort, &GPIO_InitStruct);
    // SPI MOSI pin config.
    GPIO_InitStruct.Pin = m_config->mosiPin;
    HAL_GPIO_Init(m_config->mosiPort, &GPIO_InitStruct);

    // CS pin config (GPIO_InitStruct.Alternate don't care.)
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pin = m_config->csPin;
    HAL_GPIO_Init(m_config->csPort, &GPIO_InitStruct);
    // D/CX config
    GPIO_InitStruct.Pin = m_config->dcPin;
    HAL_GPIO_Init(m_config->dcPort, &GPIO_InitStruct);

    // Configurate DMA.
    m_txDmaHandle.Instance                 = m_config->txDmaCh;
    m_txDmaHandle.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    m_txDmaHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
    m_txDmaHandle.Init.MemInc              = DMA_MINC_ENABLE;
    m_txDmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    m_txDmaHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    m_txDmaHandle.Init.Mode                = DMA_NORMAL;
    m_txDmaHandle.Init.Priority            = DMA_PRIORITY_LOW;
    m_txDmaHandle.Init.Request             = m_config->txDmaReq;
    HAL_DMA_Init(&m_txDmaHandle);
    __HAL_LINKDMA(&m_hal, hdmatx, m_txDmaHandle);

    m_rxDmaHandle.Instance                 = m_config->rxDmaCh;
    m_rxDmaHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    m_rxDmaHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
    m_rxDmaHandle.Init.MemInc              = DMA_MINC_ENABLE;
    m_rxDmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    m_rxDmaHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    m_rxDmaHandle.Init.Mode                = DMA_NORMAL;
    m_rxDmaHandle.Init.Priority            = DMA_PRIORITY_HIGH;
    m_rxDmaHandle.Init.Request             = m_config->rxDmaReq;
    HAL_DMA_Init(&m_rxDmaHandle);
    __HAL_LINKDMA(&m_hal, hdmarx, m_rxDmaHandle);

    // Configure NVIC.
    NVIC_SetPriority(m_config->txDmaIrq, m_config->txDmaPrio);
    NVIC_EnableIRQ(m_config->txDmaIrq);
    NVIC_SetPriority(m_config->rxDmaIrq, m_config->rxDmaPrio);
    NVIC_EnableIRQ(m_config->rxDmaIrq);
    NVIC_SetPriority(m_config->spiIrq, m_config->spiPrio);
    NVIC_EnableIRQ(m_config->spiIrq);
}

void Ili9341::DeInitSpi() {
    switch((uint32_t)(m_config->spi)) {
        case SPI1_BASE: __HAL_RCC_SPI1_FORCE_RESET(); __HAL_RCC_SPI1_RELEASE_RESET(); __HAL_RCC_SPI1_CLK_DISABLE(); break;
        case SPI2_BASE: __HAL_RCC_SPI2_FORCE_RESET(); __HAL_RCC_SPI2_RELEASE_RESET(); __HAL_RCC_SPI2_CLK_DISABLE(); break;
        // Add more cases here...
        default: FW_ASSERT(0); break;
    }
    HAL_GPIO_DeInit(m_config->sckPort, m_config->sckPin);
    HAL_GPIO_DeInit(m_config->misoPort, m_config->misoPin);
    HAL_GPIO_DeInit(m_config->mosiPort, m_config->mosiPin);
    HAL_GPIO_DeInit(m_config->csPort, m_config->csPin);
    HAL_GPIO_DeInit(m_config->dcPort, m_config->dcPin);

    HAL_DMA_DeInit(&m_txDmaHandle);
    HAL_DMA_DeInit(&m_rxDmaHandle);

    NVIC_DisableIRQ(m_config->txDmaIrq);
    NVIC_DisableIRQ(m_config->rxDmaIrq);
    NVIC_DisableIRQ(m_config->spiIrq);
}

bool Ili9341::InitHal() {
    m_hal.Instance               = m_config->spi;
    m_hal.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    m_hal.Init.Direction         = SPI_DIRECTION_2LINES;
    m_hal.Init.CLKPhase          = SPI_PHASE_1EDGE;
    m_hal.Init.CLKPolarity       = SPI_POLARITY_LOW;
    m_hal.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    m_hal.Init.CRCPolynomial     = 7;
    m_hal.Init.DataSize          = SPI_DATASIZE_8BIT;
    m_hal.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    m_hal.Init.NSS               = SPI_NSS_SOFT;
    m_hal.Init.TIMode            = SPI_TIMODE_DISABLE;
    m_hal.Init.Mode              = SPI_MODE_MASTER;
    return (HAL_SPI_Init(&m_hal) == HAL_OK);
}

void Ili9341::DeInitHal() {
    HAL_StatusTypeDef status = HAL_SPI_DeInit(&m_hal);
    FW_ASSERT(status == HAL_OK);
}

bool Ili9341::SpiWriteDma(uint8_t const *buf, uint16_t len) {
    bool status = false;
    HAL_GPIO_WritePin(m_config->csPort, m_config->csPin, GPIO_PIN_RESET);
    // Needs to cast away const-ness. It has been verified that HAL_SPI_Transmit_DMA does not write to buf.
    if (HAL_SPI_Transmit_DMA(&m_hal, const_cast<uint8_t *>(buf), len) == HAL_OK) {
        status = m_spiSem.wait(BSP_MSEC_TO_TICK(100000));
        // @todo - There may be a bug in QXK that after a semaphore wait times out, the "waitSet" is not cleared but no task
        //         is waiting on the semaphore. It triggers an assert at qxk_sema.cpp line 220 when the semaphore is signaled again
        //         in HAL_SPI_TxCpltCallback() in stm32f4xx_it.cpp (before it is waited on again here.)
        //
    }
    HAL_GPIO_WritePin(m_config->csPort, m_config->csPin, GPIO_PIN_SET);
    return status;
}

bool Ili9341::SpiReadDma(uint8_t *buf, uint16_t len) {
    bool status = false;
    HAL_GPIO_WritePin(m_config->csPort, m_config->csPin, GPIO_PIN_RESET);
    if (HAL_SPI_Receive_DMA(&m_hal, buf, len) == HAL_OK) {
        status = m_spiSem.wait(BSP_MSEC_TO_TICK(1000));
    }
    HAL_GPIO_WritePin(m_config->csPort, m_config->csPin, GPIO_PIN_SET);
    return status;
}

void Ili9341::WriteCmd(uint8_t cmd) {
    HAL_GPIO_WritePin(m_config->dcPort, m_config->dcPin, GPIO_PIN_RESET);
    bool status = SpiWriteDma(&cmd, 1);
    FW_ASSERT(status);
}

void Ili9341::WriteDataBuf(uint8_t const *buf, uint16_t len) {
    HAL_GPIO_WritePin(m_config->dcPort, m_config->dcPin, GPIO_PIN_SET);
    bool status = SpiWriteDma(buf, len);
    FW_ASSERT(status);
}

void Ili9341::WriteData1(uint8_t b0) {
    WriteDataBuf(&b0, 1);
}
void Ili9341::WriteData2(uint8_t b0, uint8_t b1) {
    uint8_t buf[2];
    buf[0] = b0;
    buf[1] = b1;
    WriteDataBuf(buf, sizeof(buf));
}
void Ili9341::WriteData3(uint8_t b0, uint8_t b1, uint8_t b2) {
    uint8_t buf[3];
    buf[0] = b0;
    buf[1] = b1;
    buf[2] = b2;
    WriteDataBuf(buf, sizeof(buf));
}

void Ili9341::WriteData4(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
    uint8_t buf[4];
    buf[0] = b0;
    buf[1] = b1;
    buf[2] = b2;
    buf[3] = b3;
    WriteDataBuf(buf, sizeof(buf));
}

void Ili9341::InitDisp() {
    WriteCmd(0x01);              // SW reset.
    m_container.DelayMs(10);

    WriteCmd(ILI9341_PWCTR1);    // Power control.
    WriteData1(0x23);            // VRH[5:0].

    WriteCmd(ILI9341_PWCTR2);    // Power control.
    WriteData1(0x10);            // SAP[2:0];BT[3:0].

    WriteCmd(ILI9341_VMCTR1);    // VCM control.
    WriteData2(0x3e, 0x28);

    WriteCmd(ILI9341_VMCTR2);    // VCM control2.
    WriteData1(0x86);  //--

    WriteCmd(ILI9341_MADCTL);    // Memory Access Control.
    WriteData1(0x48);

    WriteCmd(ILI9341_VSCRSADD);  // Vertical scroll.
    WriteData1(0);

    WriteCmd(ILI9341_PIXFMT);
    WriteData1(0x55);

    WriteCmd(ILI9341_FRMCTR1);
    WriteData2(0x00, 0x18);

    WriteCmd(ILI9341_DFUNCTR);    // Display Function Control.
    WriteData3(0x08, 0x82, 0x27);

    WriteCmd(0xF2);               // 3Gamma Function Disable.
    WriteData1(0x00);

    WriteCmd(ILI9341_GAMMASET);   // Gamma curve selected.
    WriteData1(0x01);

    WriteCmd(ILI9341_GMCTRP1);    // Set Gamma.
    const uint8_t dataGmcTrp1[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    WriteDataBuf(dataGmcTrp1, sizeof(dataGmcTrp1));

    WriteCmd(ILI9341_GMCTRN1);    // Set Gamma.
    const uint8_t dataGmcTrn1[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    WriteDataBuf(dataGmcTrn1, sizeof(dataGmcTrn1));

    WriteCmd(ILI9341_SLPOUT);     // Exit Sleep.
    m_container.DelayMs(120);
    WriteCmd(ILI9341_DISPON);     // Display on.
}

// Range checking is not done in this function. Caller is responsible for validating range.
void Ili9341::SetAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t x2 = x + w - 1;
    uint16_t y2 = y + h - 1;
    WriteCmd(ILI9341_CASET);      // Column addr set.
    WriteData4(BYTE_1(x), BYTE_0(x), BYTE_1(x2), BYTE_0(x2));

    WriteCmd(ILI9341_PASET);      // Row addr set.
    WriteData4(BYTE_1(y), BYTE_0(y), BYTE_1(y2), BYTE_0(y2));

    WriteCmd(ILI9341_RAMWR);      // Write to RAM.
}

void Ili9341::PushColor(uint16_t color, uint32_t pixelCnt) {
    uint8_t color1 = BYTE_1(color);
    uint8_t color0 = BYTE_0(color);
    uint32_t pixelLen = pixelCnt * sizeof(color);
    uint32_t fillLen = LESS(pixelLen, sizeof(m_buffer));
    uint32_t i = 0;
    while (i < fillLen) {
        m_buffer[i++] = color1;
        m_buffer[i++] = color0;

    }
    while (pixelLen){
        uint32_t writeLen = LESS(pixelLen, sizeof(m_buffer));
        WriteDataBuf(m_buffer, writeLen);
        pixelLen -= writeLen;
    }
}

void Ili9341::SetRotation(uint8_t rotation) {
    uint8_t m = 0;
    switch (rotation) {
        case 0:
            m = (MADCTL_MX | MADCTL_BGR);
            m_width  = m_config->width;
            m_height = m_config->height;
            break;
        case 1:
            m = (MADCTL_MV | MADCTL_BGR);
            m_width  = m_config->height;
            m_height = m_config->width;
            break;
        case 2:
            m = (MADCTL_MY | MADCTL_BGR);
            m_width  = m_config->width;
            m_height = m_config->height;
            break;
        case 3:
            m = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
            m_width  = m_config->height;
            m_height = m_config->width;
            break;
        default: FW_ASSERT(0);
    }

    WriteCmd(ILI9341_MADCTL);
    WriteData1(m);
}

void Ili9341::WritePixel(int16_t x, int16_t y, uint16_t color) {
    if ((x < 0) || (x >= m_width) || (y < 0) || (y >= m_height)) {
        return;
    }
    SetAddrWindow(x, y, 1, 1);
    PushColor(color);
}

void Ili9341::FillRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t color) {
  if ((x >= m_width) || (y >= m_height)) {
      return;
  }
  int16_t x2 = x + w - 1;
  int16_t y2 = y + h - 1;
  if ((x2 < 0) || (y2 < 0)) {
      return;
  }
  if (x < 0) {
      x = 0;
      w = x2 + 1;
  }
  if (y < 0) {
      y = 0;
      h = y2 + 1;
  }
  if (x2 >= m_width) {
      w = m_width - x;
  }
  if (y2 >= m_height) {
      h = m_height - y;
  }

  SetAddrWindow(x, y, w, h);
  PushColor(color, w * h);
}

void Ili9341::WriteBitmap(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t *buf, uint32_t len) {
    FW_ASSERT(w*h*2 == len);
    if ((x >= m_width) || (y >= m_height)) {
        return;
    }
    int16_t x2 = x + w - 1;
    int16_t y2 = y + h - 1;
    if ((x2 < 0) || (y2 < 0)) {
        return;
    }
    if (x < 0) {
        x = 0;
        w = x2 + 1;
    }
    if (y < 0) {
        y = 0;
        h = y2 + 1;
    }
    if (x2 >= m_width) {
        w = m_width - x;
    }
    if (y2 >= m_height) {
        h = m_height - y;
    }

    SetAddrWindow(x, y, w, h);
    WriteDataBuf(buf, len);
}


Ili9341::Ili9341(XThread &container) :
    Disp((QStateHandler)&Ili9341::InitialPseudoState, ILI9341, "ILI9341"),
    m_client(HSM_UNDEF), m_stateTimer(GetHsmn(), STATE_TIMER), m_config(&CONFIG[0]), m_container(container) {
    m_spiSem.init(0,1);
    memset(&m_hal, 0, sizeof(m_hal));
    memset(&m_txDmaHandle, 0, sizeof(m_txDmaHandle));
    memset(&m_rxDmaHandle, 0, sizeof(m_rxDmaHandle));
    m_width = m_config->width;
    m_height = m_config->height;
}

QState Ili9341::InitialPseudoState(Ili9341 * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Ili9341::Root);
}

QState Ili9341::Root(Ili9341 * const me, QEvt const * const e) {
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
            return Q_TRAN(&Ili9341::Stopped);
        }
        case DISP_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new DispStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case DISP_DRAW_BEGIN_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new DispDrawBeginCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case DISP_DRAW_END_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new DispDrawEndCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Ili9341::Stopped(Ili9341 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DISP_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new DispStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case DISP_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->InitSpi();
            if (me->InitHal()) {
                me->m_client = req.GetFrom();
                me->SendCfm(new DispStartCfm(ERROR_SUCCESS), req);
                me->Raise(new Evt(START));
            } else {
                ERROR("InitHal() failed");
                me->SendCfm(new DispStartCfm(ERROR_HAL, me->GetHsmn()), req);
            }
            return Q_HANDLED();
        }
        case START: {
            EVENT(e);
            return Q_TRAN(&Ili9341::Started);
        }
    }
    return Q_SUPER(&Ili9341::Root);
}

QState Ili9341::Started(Ili9341 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_container.DelayMs(100);
            me->InitDisp();
            me->SetRotation(0);
            me->FillScreen(COLOR565_WHITE);

            // Test only.
            /*
            me->DrawChar(0, 0, 'A', COLOR565_RED, COLOR565_BLACK, 1);
            me->DrawChar(0, 8, 'B', COLOR565_RED, COLOR565_BLACK, 5);
            me->DrawChar(0, 48, 'C', COLOR565_RED, COLOR565_BLACK, 10);
            char ch='0';
            for (uint32_t i=0; i<500; i++) {
                me->Write(ch++);
                if (ch > 'z') {
                    ch = '0';
                }
            }
            */
            //me->SetFont(&FreeSans24pt7b);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Ili9341::Idle);
        }
        case DISP_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->DeInitHal();
            me->DeInitSpi();
            me->SendCfm(new DispStopCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&Ili9341::Stopped);
        }
    }
    return Q_SUPER(&Ili9341::Root);
}

QState Ili9341::Idle(Ili9341 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DISP_DRAW_BEGIN_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new DispDrawBeginCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&Ili9341::Busy);
        }
    }
    return Q_SUPER(&Ili9341::Started);
}

QState Ili9341::Busy(Ili9341 * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DISP_DRAW_END_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new DispDrawEndCfm(ERROR_SUCCESS), req);
            return Q_TRAN(&Ili9341::Idle);
        }
        case DISP_DRAW_TEXT_REQ: {
            EVENT(e);
            DispDrawTextReq const &req = static_cast<DispDrawTextReq const &>(*e);
            me->SetCursor(req.GetX(), req.GetY());
            me->SetTextColor(Color565(req.GetTextColor()), Color565(req.GetBgColor()));
            me->SetTextSize(req.GetMultiplier());
            char const *str = req.GetText();
            uint32_t len = strlen(str);
            FW_ASSERT(len < DispDrawTextReq::MAX_TEXT_LEN);
            while (len--) {
                me->Write(*str++);
            }
            return Q_HANDLED();
        }
        case DISP_DRAW_RECT_REQ: {
            EVENT(e);
            DispDrawRectReq const &req = static_cast<DispDrawRectReq const &>(*e);
            me->FillRect(req.GetX(), req.GetY(), req.GetW(), req.GetH(), Color565(req.GetColor()));
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Ili9341::Started);
}


/*
QState Ili9341::MyState(Ili9341 * const me, QEvt const * const e) {
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
            return Q_TRAN(&Ili9341::SubState);
        }
    }
    return Q_SUPER(&Ili9341::SuperState);
}
*/

} // namespace APP

