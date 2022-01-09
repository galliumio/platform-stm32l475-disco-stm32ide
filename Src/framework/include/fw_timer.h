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

#ifndef FW_TIMER_H
#define FW_TIMER_H

#include <stdint.h>
#include "qpcpp.h"
#include "fw.h"

namespace FW {

class Timer : public QP::QTimeEvt {
public:
    enum Type {
        ONCE,
        PERIODIC,
        INVALID
    };

    // tickRate - 0 is default rate triggered by SysTick_Handler().
    //            Others up to QF_MAX_TICK_RATE are controlled by application (see bsp.h).
    Timer(Hsmn hsmn, QP::QSignal signal, uint8_t tickRate = 0, uint32_t msPerTick = BSP_MSEC_PER_TICK);
    ~Timer() {}

    Hsmn GetHsmn() const { return m_hsmn; }
    void Start(uint32_t timeoutMs, Type type = ONCE);
    void Restart(uint32_t timeoutMs, Type type = ONCE);
    void Stop();

protected:
    bool IsMatch(QEvt const *other) {
        // For a match, both the signal and the associated hsmn must match
        // since the same signal can be used by multiple region instances.
        // Note if sig matches, it is guaranteed that 'other' is a timer event,
        // so casting is safe.
        return other && (other->sig == sig) && (static_cast<Timer const *>(other)->GetHsmn() == m_hsmn);
    }

    Hsmn m_hsmn;
    uint32_t m_msPerTick;
};

} // namespace FW

#endif // FW_TIMER_H
