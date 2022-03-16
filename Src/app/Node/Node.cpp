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

#include "app_hsmn.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "LevelMeterInterface.h"
#include "NodeMsgInterface.h"
#include "SrvMsgInterface.h"
#include "SensorMsgInterface.h"
#include "WifiInterface.h"
#include "NodeInterface.h"
#include "NodeParserInterface.h"
#include "Node.h"

FW_DEFINE_THIS_FILE("Node.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "NODE_TIMER_EVT_START",
    NODE_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "NODE_INTERNAL_EVT_START",
    NODE_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "NODE_INTERFACE_EVT_START",
    NODE_INTERFACE_EVT
};

bool Node::SendMsg(Msg &m, uint32_t len) {
    // For log.
    auto me = this;
    // Fixes up "to" and "from" of msg. If "to" is undefined, set it to the m_srvId.
    // If "from" is undefined, set it to m_nodeId (which may still be undefined).
    if (STRING_EQUAL(m.GetTo(), MSG_UNDEF)) {
        m.SetTo(me->m_srvId);
    }
    if (STRING_EQUAL(m.GetFrom(), MSG_UNDEF)) {
        m.SetFrom(me->m_nodeId);
    }
    bool wasEmpty;
    FW_ASSERT(m.GetLen() == len);
    uint32_t sentLen = m_dataOutFifo.Write(reinterpret_cast<uint8_t const*>(&m), len, &wasEmpty);
    if (sentLen != len) {
        WARNING("SendMsg (%s len=%d) failed", m.GetType(), m.GetLen());
        return false;
    }
    if (wasEmpty) {
        Send(new WifiWriteReq(), WIFI);
    }
    return true;
}

bool Node::SendMsg(Msg &m, uint32_t len, char const *to, uint16_t seq) {
    FW_ASSERT(to);
    m.SetTo(to);
    m.SetSeq(seq);
    return SendMsg(m, len);
}

MsgEvt *Node::HandleMsg(NodeParserMsgInd const &ind, Hsmn &to) {
    to = HSM_UNDEF;
    if (auto m = CHECK_MSG_LOG(SensorControlReqMsg, ind)) {
        to = LEVEL_METER;
        return new LevelMeterControlReq(*m);
    }
    if (auto m = CHECK_MSG_LOG(SensorDataRspMsg, ind)) {
        to = LEVEL_METER;
        return new LevelMeterDataRsp(*m);
    }
    // @todo Add other message handling here.
    return nullptr;
}

Node::Node() :
    Active((QStateHandler)&Node::InitialPseudoState, NODE, "NODE"),
    m_dataOutFifo(m_dataOutFifoStor, DATA_OUT_FIFO_ORDER),
    m_dataInFifo(m_dataInFifoStor, DATA_IN_FIFO_ORDER), m_port(0), m_authSeq(0), m_pingSeq(0), m_pingTime(0), m_inEvt(QEvt::STATIC_EVT),
    m_stateTimer(GetHsmn(), STATE_TIMER), m_retryTimer(GetHsmn(), RETRY_TIMER),
    m_pingReqTimer(GetHsmn(), PING_REQ_TIMER), m_pingCfmTimer(GetHsmn(), PING_CFM_TIMER),
    m_recoveryWaitTimer(GetHsmn(), RECOVERY_WAIT_TIMER) {
    SET_EVT_NAME(NODE);
    STRBUF_COPY(m_srvId, "Srv");
    STRBUF_COPY(m_nodeId, MSG_UNDEF);
}

QState Node::InitialPseudoState(Node * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Node::Root);
}

QState Node::Root(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_nodeParser.Init(me);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Node::Stopped);
        }
        case NODE_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new NodeStartCfm(ERROR_STATE, me->GetHsmn()), req);
            return Q_HANDLED();
        }
        case NODE_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Node::Stopping);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState Node::Stopped(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case NODE_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->SendCfm(new NodeStopCfm(ERROR_SUCCESS), req);
            return Q_HANDLED();
        }
        case NODE_START_REQ: {
            EVENT(e);
            NodeStartReq const &req = static_cast<NodeStartReq const &>(*e);
            me->m_inEvt = req;
            STRBUF_COPY(me->m_domain, req.GetDomain());
            me->m_port = req.GetPort();
            return Q_TRAN(&Node::Starting);
        }
    }
    return Q_SUPER(&Node::Root);
}

QState Node::Starting(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = NodeStartReq::TIMEOUT_MS;
            FW_ASSERT(timeout > WifiStartReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new WifiStartReq(), WIFI, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case WIFI_START_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            if (e->sig == FAILED) {
                ErrorEvt const &failed = ERROR_EVT_CAST(*e);
                me->SendCfm(new NodeStartCfm(failed.GetError(), failed.GetOrigin(), failed.GetReason()), me->m_inEvt);
            } else {
                me->SendCfm(new NodeStartCfm(ERROR_TIMEOUT, me->GetHsmn()), me->m_inEvt);
            }

            return Q_TRAN(&Node::Stopping);
        }
        case DONE: {
            EVENT(e);
            me->SendCfm(new NodeStartCfm(ERROR_SUCCESS), me->m_inEvt);
            return Q_TRAN(&Node::Started);
        }
    }
    return Q_SUPER(&Node::Root);
}

QState Node::Stopping(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            uint32_t timeout = NodeStopReq::TIMEOUT_MS;
            FW_ASSERT(timeout > WifiStopReq::TIMEOUT_MS);
            me->m_stateTimer.Start(timeout);
            me->SendReq(new WifiStopReq(), WIFI, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case NODE_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case WIFI_STOP_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case FAILED:
        case STATE_TIMER: {
            EVENT(e);
            FW_ASSERT(0);
            // Will not reach here.
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Node::Stopped);
        }
    }
    return Q_SUPER(&Node::Root);
}

QState Node::Started(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Node::Running);
        }
        case FAULT_EVT: {
            EVENT(e);
            return Q_TRAN(&Node::Fault);
        }
    }
    return Q_SUPER(&Node::Root);
}

QState Node::Idle(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            LOG("Starting retry timer %dms", RETRY_TIMEOUT_MS);
            me->m_retryTimer.Start(RETRY_TIMEOUT_MS);
            STRBUF_COPY(me->m_nodeId, MSG_UNDEF);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_retryTimer.Stop();
            return Q_HANDLED();
        }
        case RETRY_TIMER: {
            EVENT(e);
            return Q_TRAN(&Node::Running);
        }
    }
    return Q_SUPER(&Node::Started);
}

QState Node::Fault(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Node::Started);
}

QState Node::Running(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_dataOutFifo.Reset();
            me->m_dataInFifo.Reset();
            // No cfm expected.
            me->Send(new NodeParserStartReq(&me->m_dataInFifo), NODE_PARSER);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            // No cfm expected.
            me->Send(new NodeParserStopReq(), NODE_PARSER);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Node::Connecting);
        }
        case WIFI_DATA_IND: {
            EVENT(e);
            // Forward to NodeParser. The source (WIFI) is forwarded so NodeParser knows which HSM to send events to.
            Evt const &ind = EVT_CAST(*e);
            me->Send(new WifiDataInd(NODE_PARSER, ind.GetFrom(), ind.GetSeq()));
            return Q_HANDLED();
        }
        case NODE_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_TRAN(&Node::Disconnecting);
        }
        case NODE_PARSER_ERROR_IND:
        case SRV_ERROR: {
            EVENT(e);
            return Q_TRAN(&Node::Disconnecting);
        }
        case WIFI_DISCONNECT_IND: {
            EVENT(e);
            return Q_TRAN(&Node::Idle);
        }
    }
    return Q_SUPER(&Node::Started);
}

QState Node::Connecting(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Node::ConnectWait);
        }
        case FAILED:
        case STATE_TIMER: {
            if (e->sig == FAILED) {
                ERROR_EVENT(ERROR_EVT_CAST(*e));
            } else {
                EVENT(e);
            }
            return Q_TRAN(&Node::Disconnecting);
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Node::Connected);
        }
    }
    return Q_SUPER(&Node::Running);
}

QState Node::ConnectWait(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_stateTimer.Start(CONNECT_WAIT_TIMEOUT_MS);
            me->SendReq(new WifiConnectReq(me->m_domain, me->m_port, &me->m_dataOutFifo, &me->m_dataInFifo), WIFI, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case WIFI_CONNECT_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new Failed(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Node::AuthWait);
        }
    }
    return Q_SUPER(&Node::Connecting);
}

QState Node::AuthWait(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_stateTimer.Start(AUTH_WAIT_TIMEOUT_MS);
            me->m_authSeq = GEN_SEQ();
            // "LevelMeter" is the suggested nodeId.
            SrvAuthReqMsg msg("user", "pwd", "LevelMeter");
            me->SendMsg(msg, sizeof(msg), me->m_srvId, me->m_authSeq);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            return Q_HANDLED();
        }
        case NODE_PARSER_MSG_IND: {
            EVENT(e);
            NodeParserMsgInd const &ind = static_cast<NodeParserMsgInd const &>(*e);
            if (auto m = me->CHECK_MSG(SrvAuthCfmMsg, ind)) {
                if (m->MatchSeq(me->m_authSeq)) {
                    if (m->IsSuccess()) {
                        STRBUF_COPY(me->m_nodeId, m->GetNodeId());
                        LOG("assigned nodeId = '%s'", me->m_nodeId);
                        me->Raise(new Evt(DONE));
                    } else {
                        me->Raise(new Failed(ERROR_AUTH, me->GetHsmn()));
                    }
                }
                return Q_HANDLED();
            }
            break;
        }
    }
    return Q_SUPER(&Node::Connecting);
}

QState Node::Disconnecting(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_stateTimer.Start(DISCONNECTING_TIMEOUT_MS);
            me->SendReq(new WifiDisconnectReq(), WIFI, true);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_stateTimer.Stop();
            me->Recall();
            return Q_HANDLED();
        }
        case NODE_STOP_REQ: {
            EVENT(e);
            me->Defer(e);
            return Q_HANDLED();
        }
        case WIFI_DISCONNECT_CFM: {
            EVENT(e);
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            bool allReceived;
            if (!me->CheckCfm(cfm, allReceived)) {
                me->Raise(new FaultEvt(cfm.GetError(), cfm.GetOrigin(), cfm.GetReason()));
            } else if (allReceived) {
                me->Raise(new Evt(DONE));
            }
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&Node::Idle);
        }
    }
    return Q_SUPER(&Node::Running);
}

QState Node::Connected(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Node::Normal);
        }
    }
    return Q_SUPER(&Node::Running);
}

QState Node::Normal(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_pingReqTimer.Start(PING_REQ_TIMEOUT_MS, Timer::PERIODIC);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_pingReqTimer.Stop();
            me->m_pingCfmTimer.Stop();
            return Q_HANDLED();
        }
        case PING_REQ_TIMER: {
            EVENT(e);
            me->m_pingTime = GetSystemMs();
            me->m_pingSeq = GEN_SEQ();
            SrvPingReqMsg msg;
            me->SendMsg(msg, sizeof(msg), me->m_srvId, me->m_pingSeq);
            me->m_pingCfmTimer.Start(PING_CFM_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case NODE_PARSER_MSG_IND: {
            EVENT(e);
            NodeParserMsgInd const &ind = static_cast<NodeParserMsgInd const &>(*e);
            if (auto m = me->CHECK_MSG(SrvPingCfmMsg, ind)) {
                if (m->MatchSeq(me->m_pingSeq) && m->IsSuccess()) {
                    me->m_pingCfmTimer.Stop();
                }
                return Q_HANDLED();
            }
            // Handles messages destined to other HSMs.
            Hsmn to;
            if (MsgEvt *evt = me->HandleMsg(ind, to)) {
                me->Send(evt, to, evt->GetMsgSeq());
                return Q_HANDLED();
            }
            break;
        }
        case PING_CFM_TIMER: {
            EVENT(e);
            return Q_TRAN(&Node::RecoveryWait);
        }
        // @todo Add events from other HSMs to generate messages.
        case LEVEL_METER_CONTROL_CFM:
        case LEVEL_METER_DATA_IND: {
            EVENT(e);
            auto const &msgEvt = static_cast<MsgEvt const &>(*e);
            me->SendMsg(msgEvt.GetMsgBase(), msgEvt.GetMsgLen());
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&Node::Connected);
}

QState Node::RecoveryWait(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_recoveryWaitTimer.Start(RECOVERY_WAIT_TIMEOUT_MS);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->m_recoveryWaitTimer.Stop();
            return Q_HANDLED();
        }
        case RECOVERY_WAIT_TIMER: {
            EVENT(e);
            me->Raise(new Evt(SRV_ERROR));
            return Q_HANDLED();
        }
        case NODE_PARSER_MSG_IND: {
            EVENT(e);
            NodeParserMsgInd const &ind = static_cast<NodeParserMsgInd const &>(*e);
            if (auto m = me->CHECK_MSG(SrvPingCfmMsg, ind)) {
                if (m->MatchSeq(me->m_pingSeq) && m->IsSuccess()) {
                    uint32_t rspTime = GetSystemMs() - me->m_pingTime;
                    LOG("ping rsp time = %dms", rspTime);
                    if (rspTime < RECOVER_PING_CFM_THRES_MS) {
                        me->Raise(new Evt(RECOVERED));
                    } else {
                        me->m_pingTime = GetSystemMs();
                        me->m_pingSeq = GEN_SEQ();
                        SrvPingReqMsg msg;
                        me->SendMsg(msg, sizeof(msg), me->m_srvId, me->m_pingSeq);
                    }
                }
                return Q_HANDLED();
            }
            break;
        }
        case RECOVERED: {
            EVENT(e);
            return Q_TRAN(&Node::Normal);
        }
    }
    return Q_SUPER(&Node::Connected);
}


/*
QState Node::MyState(Node * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&Node::SubState);
        }
    }
    return Q_SUPER(&Node::SuperState);
}
*/

} // namespace APP
