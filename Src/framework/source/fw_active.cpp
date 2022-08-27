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

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_region.h"
#include "fw_evt.h"
#include "fw_timer.h"
#include "fw.h"
#include "fw_assert.h"

FW_DEFINE_THIS_FILE("fw_active.cpp")

using namespace QP;

namespace FW {

void Active::Start(uint8_t prio) {
    Fw::Add(m_hsm.GetHsmn(), &m_hsm, this);
    m_hsm.Init(this);
    m_tlsNewLib = _REENT_INIT(m_tlsNewLib);
    m_thread = &m_tlsNewLib;
    QActive::start(prio, m_evtQueueStor, ARRAY_COUNT(m_evtQueueStor), NULL, 0);
}

void Active::Add(Region *reg) {
    FW_ASSERT(reg);
    Hsmn regHsmn = reg->GetHsmn();
    FW_ASSERT(regHsmn != HSM_UNDEF);
    HsmnReg *existing = m_hsmnRegMap.GetByKey(regHsmn);
    FW_ASSERT(existing == NULL);
    m_hsmnRegMap.Save(HsmnReg(regHsmn, reg));
    Fw::Add(regHsmn, &reg->GetHsm(), this);
}

void Active::dispatch(QEvt const * const e, std::uint_fast8_t const qs_id) {
    (void)qs_id;
    Hsmn hsmn;
    // Discard event if it is associated with an undefined HSM.
    // This happens when a timer event already posted is canceled.
    if (!IS_EVT_HSMN_VALID(e->sig)) {
        return;
    }
    if (IS_TIMER_EVT(e->sig)) {
        Timer const *timerEvt = static_cast<Timer const *>(e);
        hsmn = timerEvt->GetHsmn();
    } else {
        Evt const *evt = static_cast<Evt const *>(e);
        hsmn = evt->GetTo();
    }
    if (hsmn == m_hsm.GetHsmn()) {
        // For active object, e must be from the active object's event queue (dynamic or static/timer).
        // Garbage collection, if needed, is done by the caller.
        QHsm::dispatch(e, 0);
        // Handle all reminder events generated as a result of e.
        m_hsm.DispatchReminder();
    } else {
        HsmnReg *hsmnReg = m_hsmnRegMap.GetByKey(hsmn);
        if (hsmnReg && hsmnReg->GetValue()) {
            hsmnReg->GetValue()->Dispatch(e);
        }
    }
}

void Active::PostSync(Evt const *e) {
    FW_ASSERT(e);
    postLIFO(e);
}

} // namespace FW
