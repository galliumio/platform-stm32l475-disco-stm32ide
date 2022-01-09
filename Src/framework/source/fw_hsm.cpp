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

#include "fw_hsm.h"
#include "fw_inline.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "fw.h"

FW_DEFINE_THIS_FILE("fw_hsm.cpp")

using namespace QP;

namespace FW {

Hsm::Hsm(Hsmn hsmn, char const *name, QP::QHsm *qhsm) :
    m_hsmn(hsmn), m_name(name), m_qhsm(qhsm), m_state(Log::GetUndefName()),
    m_nextSequence(0), m_evtSeq(HSM_UNDEF) {}

void Hsm::Init(QActive *container) {
    m_deferEQueue.Init(container, m_deferQueueStor, ARRAY_COUNT(m_deferQueueStor));
    m_reminderQueue.init(m_reminderQueueStor, ARRAY_COUNT(m_reminderQueueStor));
}

void Hsm::DispatchReminder() {
    while (QEvt const *reminder = m_reminderQueue.get()) {
        m_qhsm->QHsm::dispatch(reminder);
        // A reminder event must be dynamic and is garbage collected after being processed.
        FW_ASSERT(QF_EVT_POOL_ID_(reminder) != 0);
        // A reminder event must be an internal or interface event (but not a timer event).
        FW_ASSERT(IS_EVT_HSMN_VALID(reminder->sig) && (!IS_TIMER_EVT(reminder->sig)));
        FW_ASSERT(EVT_CAST(*reminder).GetTo() == m_hsmn);
        QF::gc(reminder);
    }
}

void Hsm::SendReq(Evt *e, Hsmn to, bool reset, EvtSeqRec &seqRec) {
    FW_ASSERT(e);
    Sequence seq = GenSeq();
    e->SetTo(to);
    e->SetSeq(seq);
    seqRec.Save(to, seq, reset);
    Send(e);
}

// @description Standard handling of a confirmation or response event.
// @param[in] e - Event to handle.
// @param[out] allReceived -  Set to true if all expected confirmation/response have been received without errors.
// @return Handling status - true if the event was handled without error. This includes the event being ignored
//                           due to sequence number mismatch.
//                           false if the event matches sequence number and reports an error.
bool Hsm::CheckCfm(ErrorEvt const &e, bool &allReceived, EvtSeqRec &seqRec) {
    allReceived = false;
    if (seqRec.Match(e.GetFrom(), e.GetSeq())) {
        if (e.GetError() != ERROR_SUCCESS) {
            return false;
        }
        if(seqRec.IsAllCleared()) {
            allReceived = true;
        }
    }
    return true;
}

void Hsm::SendReqMsg(MsgEvt *e, Hsmn to, char const *msgTo, bool reset, MsgSeqRec &seqRec) {
    FW_ASSERT(e);
    Sequence seq = GenSeq();
    e->SetTo(to);
    e->SetSeq(seq);
    e->SetMsgTo(msgTo);
    e->SetMsgSeq(seq);
    seqRec.Save(msgTo, seq, reset);
    Send(e);
}

void Hsm::SendCfmMsg(MsgEvt *e, MsgEvt const &req) {
    FW_ASSERT(e);
    e->SetTo(req.GetFrom());
    e->SetSeq(req.GetSeq());
    e->SetMsgTo(req.GetMsgFrom());
    e->SetMsgSeq(req.GetMsgSeq());
    // Optional - Call e->SetMsgFrom(). If not set here, it will be added by Node::SendMsg().
    //            For internal messages, the "msg from" field will be left as MSG_UNDEF.
    Send(e);
}

void Hsm::SendCfmMsg(MsgEvt *e, MsgEvt &savedReq) {
    SendCfmMsg(e, const_cast<MsgEvt const &>(savedReq));
    savedReq.Clear();
}

// @description Standard handling of a confirmation or response message.
// @param[in] e - Message event to handle.
// @param[out] allReceived -  Set to true if all expected confirmation/response have been received without errors.
// @return Handling status - true if the message was handled without error. This includes the message being ignored
//                           due to sequence number mismatch.
//                           false if the message matches sequence number and reports an error.
bool Hsm::CheckCfmMsg(ErrorMsgEvt const &e, bool &allReceived, MsgSeqRec &seqRec) {
    allReceived = false;
    if (seqRec.Match(e.GetMsgFrom(), e.GetMsgSeq())) {
        if (!e.GetErrorMsg().IsSuccess()) {
            return false;
        }
        if(seqRec.IsAllCleared()) {
            allReceived = true;
        }
    }
    return true;
}

} // namespace FW

