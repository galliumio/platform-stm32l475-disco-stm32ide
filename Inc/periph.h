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

#ifndef PERIPH_H
#define PERIPH_H

#include "fw_def.h"
#include "fw_map.h"
#include "bsp.h"

using namespace QP;
using namespace FW;

#define PERIPH_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("periph.h", (int_t)__LINE__))

namespace APP {

typedef KeyValue<TIM_TypeDef *, TIM_HandleTypeDef *> TimHal;
typedef Map<TIM_TypeDef *, TIM_HandleTypeDef *> TimHalMap;

// This is a static class to setup shared peripherals such as TIM, GPIO, etc.
// For dedicated peripherals, they should be setup in the corresponding HSM's.
class Periph {
public:
    // The following Setup/Reset functions MUST only be called from one active object.
    // No critical sections are enforced inside them.
    static void SetupNormal();
    static void SetupLowPower();
    // Add more low power modes here if needed.
    static void Reset();

    // The following Set/Get functions may be called from more than one active objects.
    //Critical sections are enforced for safety (though may not be necessary).
    static void SetHal(TIM_TypeDef *tim, TIM_HandleTypeDef *hal) {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        m_timHalMap.Save(TimHal(tim, hal));
        QF_CRIT_EXIT(crit);
    }
    static TIM_HandleTypeDef *GetHal(TIM_TypeDef *tim) {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        TimHal *kv = m_timHalMap.GetByKey(tim);
        QF_CRIT_EXIT(crit);
        PERIPH_ASSERT(kv);
        TIM_HandleTypeDef *hal = kv->GetValue();
        PERIPH_ASSERT(hal);
        return hal;
    }

private:
    enum {
        MAX_TIM_COUNT = 10     // Maximum HW timers supported. Can increase.
    };
    static TimHal m_timHalStor[MAX_TIM_COUNT];
    static TimHalMap m_timHalMap;

    // Persistent HAL handle objects.
    static TIM_HandleTypeDef m_tim1Hal;
    static TIM_HandleTypeDef m_tim2Hal;
    // Add more HAL handles here.
};


} // namespace APP

#endif // PERIPH_H
