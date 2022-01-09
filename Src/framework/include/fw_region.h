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

#ifndef FW_REGION_H
#define FW_REGION_H

#include <stdint.h>
#include "qpcpp.h"
#include "fw_hsm.h"
#include "fw_evt.h"

namespace FW {

class Active;
class XThread;

class Region : public QP::QHsm {
public:
    Region(QP::QStateHandler const initial, Hsmn hsmn, char const *name) :
        QP::QHsm(initial),
        m_hsm(hsmn, name, this),
        m_container(NULL) {}

    void Init(Active *container);
    void Init(XThread *container);
    Hsm &GetHsm() { return m_hsm; }
    virtual void dispatch(QP::QEvt const * const e);

    // Redirection to m_hsm.
    Hsmn GetHsmn() const { return m_hsm.GetHsmn(); }
    char const *GetName() const { return m_hsm.GetName(); }
    char const *GetState() const { return m_hsm.GetState(); }
    void SetState(char const *s) { m_hsm.SetState(s); }
    Sequence GenSeq() { return m_hsm.GenSeq(); }
    bool Defer(QP::QEvt const *e) { return m_hsm.Defer(e); }
    void Recall() { m_hsm.Recall(); }

    void Send(Evt *e) { m_hsm.Send(e); }
    void Send(Evt *e,  Hsmn to) { m_hsm.Send(e, to); }
    void Send(Evt *e, Hsmn to, Sequence seq) { m_hsm.Send(e, to ,seq); }
    void SendNotInQ(Evt *e) { m_hsm.SendNotInQ(e); }
    void SendNotInQ(Evt *e,  Hsmn to) { m_hsm.SendNotInQ(e, to); }
    void SendNotInQ(Evt *e, Hsmn to, Sequence seq) { m_hsm.SendNotInQ(e, to ,seq); }

    void SendReq(Evt *e, Hsmn to, bool reset, EvtSeqRec &seqRec) { m_hsm.SendReq(e, to, reset, seqRec); }
    void SendInd(Evt *e, Hsmn to, bool reset, EvtSeqRec &seqRec) { m_hsm.SendInd(e, to, reset, seqRec); }
    // Using built-in event sequence record.
    void SendReq(Evt *e, Hsmn to, bool reset) { m_hsm.SendReq(e, to, reset); }
    void SendInd(Evt *e, Hsmn to, bool reset) { m_hsm.SendInd(e, to, reset); }
    bool CheckCfm(ErrorEvt const &e, bool &allReceived, EvtSeqRec &seqRec) { return m_hsm.CheckCfm(e, allReceived, seqRec); }
    bool CheckRsp(ErrorEvt const &e, bool &allReceived, EvtSeqRec &seqRec) { return m_hsm.CheckRsp(e, allReceived, seqRec); }
    bool CheckCfm(ErrorEvt const &e, bool &allReceived) { return m_hsm.CheckCfm(e, allReceived); }
    bool CheckRsp(ErrorEvt const &e, bool &allReceived) { return m_hsm.CheckRsp(e, allReceived); }

    void SendCfm(Evt *e, Evt const &req) { m_hsm.SendCfm(e, req); }
    void SendCfm(Evt *e, Evt &savedReq) { m_hsm.SendCfm(e, savedReq); }
    void SendRsp(Evt *e, Evt const &ind) { m_hsm.SendCfm(e, ind); }
    void SendRsp(Evt *e, Evt &savedInd) { m_hsm.SendRsp(e, savedInd); }

    void SendReqMsg(MsgEvt *e, Hsmn to, char const *msgTo, bool reset, MsgSeqRec &seqRec) {
        m_hsm.SendReqMsg(e, to, msgTo, reset, seqRec);
    }
    void SendIndMsg(MsgEvt *e, Hsmn to, char const *msgTo, bool reset, MsgSeqRec &seqRec) {
        m_hsm.SendIndMsg(e, to, msgTo, reset, seqRec);
    }
    void SendCfmMsg(MsgEvt *e, MsgEvt const &req) { m_hsm.SendCfmMsg(e, req); }
    void SendCfmMsg(MsgEvt *e, MsgEvt &savedReq) { m_hsm.SendCfmMsg(e, savedReq); }
    void SendRspMsg(MsgEvt *e, MsgEvt const &req) { m_hsm.SendRspMsg(e, req); }
    void SendRspMsg(MsgEvt *e, MsgEvt &savedReq) { m_hsm.SendRspMsg(e, savedReq); }
    bool CheckCfmMsg(ErrorMsgEvt const &e, bool &allReceived, MsgSeqRec &seqRec) {
        return m_hsm.CheckCfmMsg(e, allReceived, seqRec);
    }
    bool CheckRspMsg(ErrorMsgEvt const &e, bool &allReceived, MsgSeqRec &seqRec) {
        return m_hsm.CheckRspMsg(e, allReceived, seqRec);
    }

protected:
    // Obsolete. Use PostFront() instead to post to the front of main event queue of the active object.
    // This is used for immediate event communications among HSMs within the same active object.
    void PostSync(Evt const *e);
    void PostFront(Evt const *e) { PostSync(e); }
    // Obsolete. Use Raise() instead to post to the reminder/internal event queue of an HSM.
    // This is used for immediate event communications within an HSM.
    void PostReminder(Evt const *e) { m_hsm.PostReminder(e); }
    void Raise(Evt *e) { m_hsm.Raise(e); }

    QP::QActive *GetContainer() { return m_container; }

    Hsm m_hsm;
    QP::QActive *m_container;
};

} // namespace FW

#endif // FW_REGION_H
