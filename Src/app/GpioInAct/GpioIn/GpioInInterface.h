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

#ifndef GPIO_IN_INTERFACE_H
#define GPIO_IN_INTERFACE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

#define GPIO_IN_INTERFACE_EVT \
    ADD_EVT(GPIO_IN_START_REQ) \
    ADD_EVT(GPIO_IN_START_CFM) \
    ADD_EVT(GPIO_IN_STOP_REQ) \
    ADD_EVT(GPIO_IN_STOP_CFM) \
    ADD_EVT(GPIO_IN_INACTIVE_IND) \
    ADD_EVT(GPIO_IN_ACTIVE_IND) \
    ADD_EVT(GPIO_IN_PULSE_IND) \
    ADD_EVT(GPIO_IN_HOLD_IND)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    GPIO_IN_INTERFACE_EVT_START = INTERFACE_EVT_START(GPIO_IN),
    GPIO_IN_INTERFACE_EVT
};

enum {
    GPIO_IN_REASON_UNSPEC = 0,
};

class GpioInStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    GpioInStartReq(bool debouncing = true) :
        Evt(GPIO_IN_START_REQ), m_debouncing(debouncing) {}
    bool IsDebouncing() const { return m_debouncing; }
private:
    bool m_debouncing;        // True to enable debouncing.
};

class GpioInStartCfm : public ErrorEvt {
public:
    GpioInStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(GPIO_IN_START_CFM, error, origin, reason) {}
};

class GpioInStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    GpioInStopReq() :
        Evt(GPIO_IN_STOP_REQ) {}
};

class GpioInStopCfm : public ErrorEvt {
public:
    GpioInStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(GPIO_IN_STOP_CFM, error, origin, reason) {}
};

class GpioInInactiveInd : public Evt {
public:
    GpioInInactiveInd() :
        Evt(GPIO_IN_INACTIVE_IND) {}
};

class GpioInActiveInd : public Evt {
public:
    GpioInActiveInd() :
        Evt(GPIO_IN_ACTIVE_IND) {}
};

class GpioInPulseInd : public Evt {
public:
    GpioInPulseInd() :
        Evt(GPIO_IN_PULSE_IND) {}
};

class GpioInHoldInd : public Evt {
public:
    GpioInHoldInd() :
        Evt(GPIO_IN_HOLD_IND) {}
};

} // namespace APP

#endif // GPIO_IN_INTERFACE_H
