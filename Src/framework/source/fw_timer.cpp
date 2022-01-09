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

#include "bsp.h"
#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_inline.h"
#include "fw_assert.h"

FW_DEFINE_THIS_FILE("fw_timer.cpp")

using namespace QP;

namespace FW {

// The timer signal is set to Q_USER_SIG which is associated with an undefined HSM.
// It will be discarded by Active::dispatch().
// The signal is set to Q_USER_SIG in case the QTimeEvt constructor asserts (signal >= Q_USER_SIG).
Timer const CANCELED_TIMER(HSM_UNDEF, Q_USER_SIG);

// Allow hsmn == HSM_UNDEF. In that case GetContainer() returns NULL.
Timer::Timer(Hsmn hsmn, QP::QSignal signal, uint8_t tickRate, uint32_t msPerTick) :
    // QP requires an active object to be provided along with tickRate, which is not available during construction.
    // A placeholder is used here and it will be set in Start().
    QTimeEvt(NULL, signal, tickRate), m_hsmn(hsmn), m_msPerTick(msPerTick) {}

void Timer::Start(uint32_t timeoutMs, Type type) {
    QActive *act = Fw::GetContainer(m_hsmn);
    FW_ASSERT(act && (type < INVALID) && (m_msPerTick > 0));
    QTimeEvtCtr timeoutTick = ROUND_UP_DIV(timeoutMs, m_msPerTick);
    if (type == ONCE) {
        QTimeEvt::postIn(act, timeoutTick);
    } else {
        QTimeEvt::postEvery(act, timeoutTick);
    }
}

void Timer::Restart(uint32_t timeoutMs, Type type) {
    Stop();
    Start(timeoutMs, type);
}

void Timer::Stop() {
    // Doesn't care what disarm returns. For a periodic timer, even when it is still armed
    // a previous timeout event might still be in event queue and must be removed.
    // In any cases we need to purge residue timer events in event queue.
    QTimeEvt::disarm();
    QActive *act = Fw::GetContainer(m_hsmn);
    FW_ASSERT(act);
    QEQueue *queue = &act->m_eQueue;
    FW_ASSERT(queue);
    // Critical section must support nesting.
    QF_CRIT_STAT_TYPE crit;
    QF_CRIT_ENTRY(crit);
    if (queue->m_frontEvt) {
        if (IsMatch(queue->m_frontEvt)) {
            FW_ASSERT(IS_TIMER_EVT(queue->m_frontEvt->sig) && QF_EVT_POOL_ID_(queue->m_frontEvt) == 0);
            queue->m_frontEvt = &CANCELED_TIMER;
        }
        // nFree includes frontEvt, so nFree <= m_end + 1 (where m_end is the size of m_ring).
        // Since frontEvt is used, nFree <= (m_end + 1) - 1 = m_end
        FW_ASSERT(queue->m_nFree <= queue->m_end);
        // Total used entries = total - nFree = (m_end + 1) - nFree.
        // Used entries in m_ring = total used - 1 = m_end - nFree, which must be >= 0.
        QEQueueCtr count = queue->m_end - queue->m_nFree;
        QEQueueCtr i = queue->m_tail;
        while (count--) {
            QEvt const *e = queue->m_ring[i];
            if (IsMatch(e)) {
                FW_ASSERT(IS_TIMER_EVT(e->sig) && QF_EVT_POOL_ID_(e) == 0);
                queue->m_ring[i] = &CANCELED_TIMER;
            }
            if (i == static_cast<QEQueueCtr>(0)) {
                i = queue->m_end;
            }
            --i;
        }
    }
    // Nothing to do if queue empty (frontEvt == NULL).
    QF_CRIT_EXIT(crit);
}


} // namespace FW
