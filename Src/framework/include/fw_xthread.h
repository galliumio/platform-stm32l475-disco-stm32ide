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

#ifndef FW_XTHREAD_H
#define FW_XTHREAD_H

#include <stdint.h>
#include "qpcpp.h"
#include "fw_maptype.h"
#include "fw_evt.h"
#include "bsp.h"

#define FW_XTHREAD_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_xthread.h", (int_t)__LINE__))

namespace FW {

class XThread : public QP::QXThread {
public:
    XThread() :
        QP::QXThread(XThreadHandler),
        m_hsmnRegMap(m_hsmnRegStor, ARRAY_COUNT(m_hsmnRegStor), HsmnReg(HSM_UNDEF, NULL)) {
    }
    void Start(uint8_t prio);
    void Add(Region *reg);
    static void DelayMs(uint32_t ms) { delay(BSP_MSEC_TO_TICK(ms)); }

protected:
    virtual void OnRun() = 0;

    enum {
        MAX_REGION_COUNT = 8,
        EVT_QUEUE_COUNT = 16,
        STACK_SIZE_BYTE = 4096
    };
    HsmnReg m_hsmnRegStor[MAX_REGION_COUNT];
    HsmnRegMap m_hsmnRegMap;
    QP::QEvt const *m_evtQueueStor[EVT_QUEUE_COUNT];
    uint64_t m_stackSto[ROUND_UP_DIV_8(STACK_SIZE_BYTE)];
    struct _reent m_tlsNewLib;      // Thread-local-storage for NewLib.

private:
    static void XThreadHandler(QXThread * const me) {
        XThread *t = static_cast<XThread *>(me);
        while(1) {
            QP::QEvt const *e = t->queueGet();
            FW_XTHREAD_ASSERT(e);
            t->Dispatch(e);
            QP::QF::gc(e);
        }
    }
    void Dispatch(QP::QEvt const * const e);
};

} // namespace FW


#endif // FW_XTHREAD_H
