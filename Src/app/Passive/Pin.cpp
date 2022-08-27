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

#include "fw_assert.h"
#include "Pin.h"

FW_DEFINE_THIS_FILE("Pin.cpp")

using namespace QP;
using namespace FW;

namespace APP {

void Pin::Init(GPIO_TypeDef *port, uint16_t pin, uint32_t mode, uint32_t pull, uint32_t af, uint32_t speed) {
    if (m_port) {
        // If already initialized, deinit first.
        DeInit();
    }
    // Clock is initialized by System via Periph.
    m_port = port;
    m_pin = pin;
    GPIO_InitTypeDef gpioInit;
    memset(&gpioInit, 0, sizeof(gpioInit));
    gpioInit.Pin = m_pin;
    gpioInit.Mode = mode;
    gpioInit.Pull = pull;
    gpioInit.Alternate = af;
    gpioInit.Speed = speed;
    // Enforces critical section since some registers are shared among different pins.
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    HAL_GPIO_Init(m_port, &gpioInit);
    QF_CRIT_EXIT(crit);

}

void Pin::DeInit() {
    // Does nothing if Init() has not been called.
    if (m_port) {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        HAL_GPIO_DeInit(m_port, m_pin);
        QF_CRIT_EXIT(crit);
        m_port = NULL;
        m_pin = 0;
    }
}

}
