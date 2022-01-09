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

#ifndef NODE_PARSER_INTERFACE_H
#define NODE_PARSER_INTERFACE_H

#include "fw_def.h"
#include "fw_macro.h"
#include "fw_evt.h"
#include "fw_pipe.h"
#include "fw_assert.h"
#include "app_hsmn.h"
#include "fw_msg.h"

#define NODE_PARSER_INTERFACE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("NodeParserInterface.h", (int_t)__LINE__))

using namespace QP;
using namespace FW;

namespace APP {

#define NODE_PARSER_INTERFACE_EVT \
    ADD_EVT(NODE_PARSER_START_REQ) \
    ADD_EVT(NODE_PARSER_START_CFM) \
    ADD_EVT(NODE_PARSER_STOP_REQ) \
    ADD_EVT(NODE_PARSER_STOP_CFM) \
    ADD_EVT(NODE_PARSER_ERROR_IND) \
    ADD_EVT(NODE_PARSER_MSG_IND)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

enum {
    NODE_PARSER_INTERFACE_EVT_START = INTERFACE_EVT_START(NODE_PARSER),
    NODE_PARSER_INTERFACE_EVT
};

enum {
    NODE_PARSER_REASON_UNSPEC = 0,
};

class NodeParserStartReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    NodeParserStartReq(Fifo *dataInFifo) :
        Evt(NODE_PARSER_START_REQ), m_dataInFifo(dataInFifo) {}
    Fifo *GetDataInFifo() const { return m_dataInFifo; }
private:
    Fifo *m_dataInFifo;
};

// Not used.
class NodeParserStartCfm : public ErrorEvt {
public:
    NodeParserStartCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(NODE_PARSER_START_CFM, error, origin, reason) {}
};

class NodeParserStopReq : public Evt {
public:
    enum {
        TIMEOUT_MS = 100
    };
    NodeParserStopReq() :
        Evt(NODE_PARSER_STOP_REQ) {}
};

// Not used.
class NodeParserStopCfm : public ErrorEvt {
public:
    NodeParserStopCfm(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(NODE_PARSER_STOP_CFM, error, origin, reason) {}
};

class NodeParserErrorInd : public ErrorEvt {
public:
    NodeParserErrorInd(Error error, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        ErrorEvt(NODE_PARSER_ERROR_IND, error, origin, reason) {}
};

class NodeParserMsgInd : public Evt {
public:
    NodeParserMsgInd() :
        Evt(NODE_PARSER_MSG_IND) {
        NODE_PARSER_INTERFACE_ASSERT(IS_ALIGNED_4(MAX_BUF_LEN));
        NODE_PARSER_INTERFACE_ASSERT(MAX_BUF_LEN >= sizeof(Msg));
        memset(m_msgBuf, 0, sizeof(m_msgBuf));
    }
    enum {
        MAX_BUF_LEN = 2000      // Limited to max block size of event pools (see fw.h)
    };
    uint8_t *GetMsgBufMutable() { return reinterpret_cast<uint8_t *>(m_msgBuf); }
    uint8_t const *GetMsgBuf() const { return reinterpret_cast<uint8_t const *>(m_msgBuf); }
    char const *GetMsgType() const { return reinterpret_cast<Msg const *>(m_msgBuf)->GetType(); }
    uint16_t GetMsgSeq() const { return reinterpret_cast<Msg const *>(m_msgBuf)->GetSeq(); }
    uint32_t GetMsgLen() const { return reinterpret_cast<Msg const *>(m_msgBuf)->GetLen(); }
    bool IsMsgLenValid() const { return (GetMsgLen() >= sizeof(Msg)) && (GetMsgLen() <= MAX_BUF_LEN); }
    bool MatchMsgType(char const *type) const {
        NODE_PARSER_INTERFACE_ASSERT(type);
        return STRING_EQUAL(GetMsgType(), type);
    }
    bool MatchMsgLen(uint32_t len) const { return IsMsgLenValid() && (GetMsgLen() == len); }
private:
    // Ensures 4-byte aligned. This buffer will be casted into a message object.
    uint32_t m_msgBuf[MAX_BUF_LEN/4];
};

} // namespace APP

#endif // NODE_PARSER_INTERFACE_H
