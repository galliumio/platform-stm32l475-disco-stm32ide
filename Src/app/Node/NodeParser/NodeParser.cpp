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
#include "fw_macro.h"
#include "WifiInterface.h"
#include "NodeParserInterface.h"
#include "NodeParser.h"

FW_DEFINE_THIS_FILE("NodeParser.cpp")

namespace APP {

#undef ADD_EVT
#define ADD_EVT(e_) #e_,

static char const * const timerEvtName[] = {
    "NODE_PARSER_TIMER_EVT_START",
    NODE_PARSER_TIMER_EVT
};

static char const * const internalEvtName[] = {
    "NODE_PARSER_INTERNAL_EVT_START",
    NODE_PARSER_INTERNAL_EVT
};

static char const * const interfaceEvtName[] = {
    "NODE_PARSER_INTERFACE_EVT_START",
    NODE_PARSER_INTERFACE_EVT
};

NodeParser::NodeParser() :
    Region((QStateHandler)&NodeParser::InitialPseudoState, NODE_PARSER, "NODE_PARSER"),
    m_manager(HSM_UNDEF), m_dataInFifo(nullptr), m_msgInd(nullptr), m_dataLen(0), m_msgIdx(0),
    m_stateTimer(GetHsmn(), STATE_TIMER) {
    SET_EVT_NAME(NODE_PARSER);
}

QState NodeParser::InitialPseudoState(NodeParser * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&NodeParser::Root);
}

QState NodeParser::Root(NodeParser * const me, QEvt const * const e) {
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
            return Q_TRAN(&NodeParser::Stopped);
        }
    }
    return Q_SUPER(&QHsm::top);
}

QState NodeParser::Stopped(NodeParser * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case NODE_PARSER_START_REQ: {
            EVENT(e);
            NodeParserStartReq const &req = static_cast<NodeParserStartReq const &>(*e);
            me->m_manager = req.GetFrom();
            me->m_dataInFifo = req.GetDataInFifo();
            FW_ASSERT(me->m_dataInFifo);
            return Q_TRAN(&NodeParser::Started);
        }
    }
    return Q_SUPER(&NodeParser::Root);
}

QState NodeParser::Started(NodeParser * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            FW_ASSERT(me->m_msgInd);
            QF::gc(me->m_msgInd);
            return Q_HANDLED();
        }
        case Q_INIT_SIG: {
            return Q_TRAN(&NodeParser::HeaderWait);
        }
        case NODE_PARSER_STOP_REQ: {
            EVENT(e);
            me->m_dataInFifo = nullptr;
            return Q_TRAN(&NodeParser::Stopped);
        }
        case WIFI_DATA_IND: {
        /* readLen = MIN((dataLen, dataInFifo used cnt)
          append readLen bytes from dataInFifo to msgInd->getBuf()
          dataLen -= readLen
          if (dataLen == 0) ^^DATA_DONE
          if (dataInFifo not empty) ^WIFI_DATA_IND
          else ^WIFI_DATA_RSP
          */
            EVENT(e);
            uint32_t readLen;
            while ((readLen = LESS(me->m_dataLen, me->m_dataInFifo->GetUsedBlockCount()))) {
                FW_ASSERT((me->m_msgIdx + readLen) <= NodeParserMsgInd::MAX_BUF_LEN);
                memcpy(&me->m_msgInd->GetMsgBufMutable()[me->m_msgIdx],
                       reinterpret_cast<uint8_t const*>(me->m_dataInFifo->GetReadAddr()), readLen);
                INFO_BUF(reinterpret_cast<uint8_t const*>(me->m_dataInFifo->GetReadAddr()), readLen, 1, 0);
                me->m_dataInFifo->IncReadIndex(readLen);
                me->m_dataLen -= readLen;
                me->m_msgIdx += readLen;
            }
            if (me->m_dataLen == 0) {
                me->Raise(new Evt(DATA_DONE));
            }
            if (me->m_dataInFifo->GetUsedCount()) {
                // Sends reminder to itself as some data remains in fifo.
                Evt const &ind = EVT_CAST(*e);
                me->Send(new WifiDataInd(me->GetHsmn(), ind.GetFrom(), ind.GetSeq()));
            } else {
                // @todo ^WifiReadMoreReq.
                // Purpose is to let Wifi know that it may resume receiving more data as fifo has been emptied,
                // in case it is waiting for it.
            }
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&NodeParser::Root);
}

QState NodeParser::HeaderWait(NodeParser * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_msgInd = new NodeParserMsgInd();
            // m_msgInd will be filled in as data are received.
            // It will either be sent upon completion or be freed upon exit of Started.
            me->m_dataLen = sizeof(Msg);
            me->m_msgIdx = 0;
            LOG("HeaderWait dataLen = %d", me->m_dataLen);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DATA_DONE: {
            EVENT(e);
            if (me->m_msgInd->IsMsgLenValid()) {
                me->Raise(new Evt(NEXT));
            } else {
                // @todo ^ERROR_EVT
            }
            return Q_HANDLED();
        }
        case NEXT: {
            EVENT(e);
            return Q_TRAN(&NodeParser::BodyWait);
        }
    }
    return Q_SUPER(&NodeParser::Started);
}

QState NodeParser::BodyWait(NodeParser * const me, QEvt const * const e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->m_dataLen = me->m_msgInd->GetMsgLen() - sizeof(Msg);
            LOG("BodyWait dataLen = %d", me->m_dataLen);
            return Q_HANDLED();
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            return Q_HANDLED();
        }
        case DATA_DONE: {
            EVENT(e);
            me->Send(me->m_msgInd, me->m_manager);
            me->m_msgInd = nullptr;
            me->Raise(new Evt(DONE));
            return Q_HANDLED();
        }
        case DONE: {
            EVENT(e);
            return Q_TRAN(&NodeParser::HeaderWait);
        }
    }
    return Q_SUPER(&NodeParser::Started);
}

/*
QState NodeParser::MyState(NodeParser * const me, QEvt const * const e) {
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
            return Q_TRAN(&NodeParser::SubState);
        }
    }
    return Q_SUPER(&NodeParser::SuperState);
}
*/

} // namespace APP

