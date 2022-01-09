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

#ifndef NODE_H
#define NODE_H

#include "qpcpp.h"
#include "fw_active.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "fw_log.h"
#include "app_hsmn.h"
#include "NodeParser.h"
#include "NodeParserInterface.h"
#include "NodeInterface.h"
#include "SrvMsgInterface.h"

using namespace QP;
using namespace FW;

#define CHECK_MSG(T_, e_)       CheckMsg<T_>(e_, #T_, false)
#define CHECK_MSG_LOG(T_, e_)   CheckMsg<T_>(e_, #T_, true)

namespace APP {

class Node : public Active {
public:
    Node();

protected:
    static QState InitialPseudoState(Node * const me, QEvt const * const e);
    static QState Root(Node * const me, QEvt const * const e);
        static QState Stopped(Node * const me, QEvt const * const e);
        static QState Starting(Node * const me, QEvt const * const e);
        static QState Stopping(Node * const me, QEvt const * const e);
        static QState Started(Node * const me, QEvt const * const e);
            static QState Idle(Node * const me, QEvt const * const e);
            static QState Fault(Node * const me, QEvt const * const e);
            static QState Running(Node * const me, QEvt const * const e);
                static QState Connecting(Node * const me, QEvt const * const e);
                    static QState ConnectWait(Node * const me, QEvt const * const e);
                    static QState AuthWait(Node * const me, QEvt const * const e);
                static QState Disconnecting(Node * const me, QEvt const * const e);
                static QState Connected(Node * const me, QEvt const * const e);
                    static QState Normal(Node * const me, QEvt const * const e);
                    static QState RecoveryWait(Node * const me, QEvt const * const e);

    template<class T>
    T const *CheckMsg(NodeParserMsgInd const &e, char const *type, bool isLog) {
        if (e.MatchMsgType(type) && e.MatchMsgLen(sizeof(T))) {
            auto m = reinterpret_cast<T const *>(e.GetMsgBuf());
            if (isLog) {
                Log::DebugBuf(Log::TYPE_LOG, GetHsmn(), e.GetMsgBuf(), e.GetMsgLen(), 1, 0);
                LogMsg(*m);
            }
            return m;
        }
        return nullptr;
    }

    void LogMsg(Msg const &m) {
        Log::Debug(Log::TYPE_LOG, GetHsmn(), "%s to %s from %s len=%d seq=%d",
                   m.GetType(), m.GetTo(), m.GetFrom(), m.GetLen(), m.GetSeq());
    }
    void LogErrorMsg(ErrorMsg const &m) {
        Log::Debug(Log::TYPE_LOG, GetHsmn(), "%s to %s from %s len=%d seq=%d error=%s origin=%s reason=%s",
                   m.GetType(), m.GetTo(), m.GetFrom(), m.GetLen(), m.GetSeq(), m.GetError(), m.GetOrigin(), m.GetReason());
    }
    bool SendMsg(Msg &m, uint32_t len);
    bool SendMsg(Msg &m, uint32_t len, char const *to, uint16_t seq);
    MsgEvt *HandleMsg(NodeParserMsgInd const &e, Hsmn &to);

    NodeParser m_nodeParser;
    enum {
        DATA_OUT_FIFO_ORDER = 10,
        DATA_IN_FIFO_ORDER = 10,
    };
    uint8_t m_dataOutFifoStor[ROUND_UP_32(1 << DATA_OUT_FIFO_ORDER)] __attribute__((aligned(32)));
    uint8_t m_dataInFifoStor[ROUND_UP_32(1 << DATA_IN_FIFO_ORDER)] __attribute__((aligned(32)));
    Fifo m_dataOutFifo;
    Fifo m_dataInFifo;
    char m_domain[NodeStartReq::DOMAIN_LEN];    // Server domain or IP address string.
    uint16_t m_port;                            // Server port.
    char m_srvId[SrvAuthCfmMsg::NODE_ID_LEN];   // Currently fixed to "Srv" in ctor.
    char m_nodeId[SrvAuthCfmMsg::NODE_ID_LEN];
    uint16_t m_authSeq;             // Seq no. of SrvAutReqMsg sent.
    uint16_t m_pingSeq;             // Seq no. of SrvPingReqMsg sent.
    uint32_t m_pingTime;            // Timestamp when SrvPingReqMsg is sent.
    Evt m_inEvt;                    // Static event copy of a generic incoming req to be confirmed. Added more if needed.

    enum {
        RECOVER_PING_CFM_THRES_MS = 400
    };

    enum {
        CONNECT_WAIT_TIMEOUT_MS = 30000,
        AUTH_WAIT_TIMEOUT_MS = 1000,
        DISCONNECTING_TIMEOUT_MS = 5000,
        RETRY_TIMEOUT_MS = 500,
        PING_REQ_TIMEOUT_MS = 3000, //1000,
        PING_CFM_TIMEOUT_MS = 600,
        RECOVERY_WAIT_TIMEOUT_MS = 5000,
    };

    Timer m_stateTimer;
    Timer m_retryTimer;
    Timer m_pingReqTimer;
    Timer m_pingCfmTimer;
    Timer m_recoveryWaitTimer;

#define NODE_TIMER_EVT \
    ADD_EVT(STATE_TIMER) \
    ADD_EVT(RETRY_TIMER) \
    ADD_EVT(PING_REQ_TIMER) \
    ADD_EVT(PING_CFM_TIMER) \
    ADD_EVT(RECOVERY_WAIT_TIMER)

#define NODE_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(FAILED) \
    ADD_EVT(FAULT_EVT) \
    ADD_EVT(RETRY) \
    ADD_EVT(SRV_ERROR) \
    ADD_EVT(RECOVERED)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        NODE_TIMER_EVT_START = TIMER_EVT_START(NODE),
        NODE_TIMER_EVT
    };

    enum {
        NODE_INTERNAL_EVT_START = INTERNAL_EVT_START(NODE),
        NODE_INTERNAL_EVT
    };

    class Failed : public ErrorEvt {
    public:
        Failed(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
            ErrorEvt(FAILED, error, origin, reason) {}
    };

    class FaultEvt : public ErrorEvt {
    public:
        FaultEvt(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
            ErrorEvt(FAULT_EVT, error, origin, reason) {}
    };
};

} // namespace APP

#endif // NODE_H
