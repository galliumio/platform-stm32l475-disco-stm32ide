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

#ifndef FW_H
#define FW_H

#include "bsp.h"
#include "qpcpp.h"
#include "fw_evt.h"
#include "fw_map.h"
#include "fw_maptype.h"
#include "fw_def.h"

namespace FW {

class Fw {
public:
    static void Init();
    static void Add(Hsmn hsmn, Hsm *hsm, QP::QActive *container);
    static void Post(Evt const *e);
    static void PostNotInQ(Evt const *e);
    static Hsm *GetHsm(Hsmn hsmn);
    static QP::QActive *GetContainer(Hsmn hsmn);

protected:
    enum {
        EVT_POOL_COUNT = 4,     // Number of event pools (small, medium and large).
        EVT_SIZE_SMALL = 32,
        EVT_SIZE_MEDIUM = 64,
        EVT_SIZE_LARGE = 256,
        EVT_SIZE_XLARGE = 2048,
        EVT_COUNT_SMALL = 32,
        EVT_COUNT_MEDIUM = 8,
        EVT_COUNT_LARGE = 4,
        EVT_COUNT_XLARGE = 2
    };

    static HsmAct m_hsmActStor[MAX_HSM_COUNT];
    static HsmActMap m_hsmActMap;
    static uint32_t m_evtPoolSmall[ROUND_UP_DIV_4(EVT_SIZE_SMALL * EVT_COUNT_SMALL)];
    static uint32_t m_evtPoolMedium[ROUND_UP_DIV_4(EVT_SIZE_MEDIUM * EVT_COUNT_MEDIUM)];
    static uint32_t m_evtPoolLarge[ROUND_UP_DIV_4(EVT_SIZE_LARGE * EVT_COUNT_LARGE)];
    static uint32_t m_evtPoolXLarge[ROUND_UP_DIV_4(EVT_SIZE_XLARGE * EVT_COUNT_XLARGE)];

    static bool EventMatched(Evt const *e1, QP::QEvt const *e2);
    static bool EventInQNoCrit(Evt const *e, QP::QEQueue *queue);
};

} // namespace FW

#endif // FW_H
