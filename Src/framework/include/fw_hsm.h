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

#ifndef FW_HSM_H
#define FW_HSM_H

#include <stdint.h>
#include "qpcpp.h"
#include "fw_def.h"
#include "fw_defer.h"
#include "fw_map.h"
#include "fw_evt.h"
#include "fw_seqrec.h"
#include "fw_assert.h"
#include "fw.h"
#include "fw_log.h"

#define FW_HSM_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_hsm.h", (int_t)__LINE__))


#define GET_HSMN()  me->GetHsm().GetHsmn()
#define GEN_SEQ()   me->GetHsm().GenSeq()

namespace FW {

class Active;
class Region;

typedef KeyValue<Hsmn, Sequence> HsmnSeq;
typedef Map<Hsmn, Sequence> HsmnSeqMap;

class Hsm {
public:
    Hsm(Hsmn hsmn, char const *name, QP::QHsm *qhsm);
    void Init(QP::QActive *container);

    Hsmn GetHsmn() const { return m_hsmn; }
    char const *GetName() const { return m_name; }
    char const *GetState() const { return m_state; }
    void SetState(char const *s) { m_state = s; }

    Sequence GenSeq() { return m_nextSequence++; }
    bool Defer(QP::QEvt const *e) { return m_deferEQueue.Defer(e); }
    void Recall() { m_deferEQueue.Recall(); }

    // Obsolete. Use Raise() instead to post to the reminder/internal event queue of an HSM.
    // This is used for immediate event communications within an HSM.
    void PostReminder(Evt const *e) { m_reminderQueue.post(e, QP::QF_NO_MARGIN, 0); }
    void Raise(Evt *e) {
        e->SetTo(m_hsmn);
        e->SetFrom(m_hsmn);
        e->SetSeq(GenSeq());
        PostReminder(e);
    }

    // API to post events.
    void Send(Evt *e) {
        FW_HSM_ASSERT(e);
        if (e->GetFrom() == HSM_UNDEF) {
            e->SetFrom(m_hsmn);
        }
        Fw::Post(e);
    }
    void Send(Evt *e, Hsmn to) {
        FW_HSM_ASSERT(e);
        e->SetTo(to);
        e->SetSeq(GenSeq());
        Send(e);
    }
    void Send(Evt *e, Hsmn to, Sequence seq) {
        FW_HSM_ASSERT(e);
        e->SetTo(to);
        e->SetSeq(seq);
        Send(e);
    }
    void SendNotInQ(Evt *e) {
        FW_HSM_ASSERT(e);
        if (e->GetFrom() == HSM_UNDEF) {
            e->SetFrom(m_hsmn);
        }
        Fw::PostNotInQ(e);
    }
    void SendNotInQ(Evt *e, Hsmn to) {
        FW_HSM_ASSERT(e);
        e->SetTo(to);
        e->SetSeq(GenSeq());
        SendNotInQ(e);
    }
    void SendNotInQ(Evt *e, Hsmn to, Sequence seq) {
        FW_HSM_ASSERT(e);
        e->SetTo(to);
        e->SetSeq(seq);
        SendNotInQ(e);
    }

    void SendReq(Evt *e, Hsmn to, bool reset, EvtSeqRec &seqRec);
    void SendInd(Evt *e, Hsmn to, bool reset, EvtSeqRec &seqRec) { SendReq(e, to, reset, seqRec); }
    // Using built-in event sequence record.
    void SendReq(Evt *e, Hsmn to, bool reset) { SendReq(e, to, reset, m_evtSeq); }
    void SendInd(Evt *e, Hsmn to, bool reset) { SendReq(e, to, reset); }
    bool CheckCfm(ErrorEvt const &e, bool &allReceived, EvtSeqRec &seqRec);
    bool CheckRsp(ErrorEvt const &e, bool &allReceived, EvtSeqRec &seqRec) { return CheckCfm(e, allReceived, seqRec); }
    bool CheckCfm(ErrorEvt const &e, bool &allReceived) { return CheckCfm(e, allReceived, m_evtSeq); }
    bool CheckRsp(ErrorEvt const &e, bool &allReceived) { return CheckCfm(e, allReceived); }

    // Const req. An event being processed.
    void SendCfm(Evt *e, Evt const &req) {
        // OK if "from" is HSM_UNDEF.
        Send(e, req.GetFrom(), req.GetSeq());
    }
    // Non-const req. A saved event copy that can be cleared.
    void SendCfm(Evt *e, Evt &savedReq) {
        SendCfm(e, const_cast<Evt const &>(savedReq));
        savedReq.Clear();
    }
    void SendRsp(Evt *e, Evt const &ind) { SendCfm(e, ind); }
    void SendRsp(Evt *e, Evt &savedInd) { SendCfm(e, savedInd); }

    // API to send messages.
    void SendReqMsg(MsgEvt *e, Hsmn to, char const *msgTo, bool reset, MsgSeqRec &seqRec);
    void SendIndMsg(MsgEvt *e, Hsmn to, char const *msgTo, bool reset, MsgSeqRec &seqRec) {
        SendReqMsg(e, to, msgTo, reset, seqRec);
    }
    void SendCfmMsg(MsgEvt *e, MsgEvt const &req);
    void SendCfmMsg(MsgEvt *e, MsgEvt &savedReq);
    void SendRspMsg(MsgEvt *e, MsgEvt const &req) { SendCfmMsg(e, req); }
    void SendRspMsg(MsgEvt *e, MsgEvt &savedReq) { SendCfmMsg(e, savedReq); }
    bool CheckCfmMsg(ErrorMsgEvt const &e, bool &allReceived, MsgSeqRec &seqRec);
    bool CheckRspMsg(ErrorMsgEvt const &e, bool &allReceived, MsgSeqRec &seqRec) {
        return CheckCfmMsg(e, allReceived, seqRec);
    }

protected:
    enum {
        DEFER_QUEUE_COUNT = 16,
        REMINDER_QUEUE_COUNT = 4
    };

    // Called by Active::dispatch() and Region::dispatch().
    void DispatchReminder();

    Hsmn m_hsmn;
    char const * m_name;
    QP::QHsm *m_qhsm;
    char const *m_state;
    Sequence m_nextSequence;
    DeferEQueue m_deferEQueue;
    QP::QEQueue m_reminderQueue;
    EvtSeqRec m_evtSeq;         // Built-in record of sequence numbers of outgoing events. Application classes may add custom ones when needed.
    QP::QEvt const *m_deferQueueStor[DEFER_QUEUE_COUNT];
    QP::QEvt const *m_reminderQueueStor[REMINDER_QUEUE_COUNT];

    friend class Active;        // For calling DispatchReminder().
    friend class Region;        // For calling DispatchReminder().
};

} // namespace FW


#endif // FW_HSM_H
