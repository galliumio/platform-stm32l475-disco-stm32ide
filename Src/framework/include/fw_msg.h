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

#ifndef FW_MSG_H
#define FW_MSG_H

#include "qpcpp.h"
#include "fw_def.h"
#include "fw_macro.h"
#include "fw_evt.h"

#define FW_MSG_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_msg.h", (int_t)__LINE__))

#define MSG_CAST(m_)            static_cast<FW::Msg const &>(m_)
#define ERROR_MSG_CAST(m_)      static_cast<FW::ErrorMsg const &>(m_)

namespace FW {

#define MSG_UNDEF           "UNDEF"         // Undefined value for message fields.

#define MSG_ERROR_SUCCESS   "SUCCESS"       // No error, success.
#define MSG_ERROR_UNSPEC    "UNSPEC"        // Unspecified.
#define MSG_ERROR_TIMEOUT   "TIMEOUT"       // Timeout.
#define MSG_ERROR_HAL       "HAL"           // HAL driver error.
#define MSG_ERROR_HARDWARE  "HARDWARE"      // Hardware error.
#define MSG_ERROR_HSMN      "HSM"           // Invalid HSMN.
#define MSG_ERROR_STATE     "STATE"         // Invalid state.
#define MSG_ERROR_UNAVAIL   "UNAVAIL"       // Resource unavailable, busy.
#define MSG_ERROR_PARAM     "PARAM"         // Invalid parameter, out of range.
#define MSG_ERROR_AUTH      "AUTH"          // Authentication error.
#define MSG_ERROR_NETWORK   "NETWORK"       // Network error.
#define MSG_ERROR_ABORT     "ABORT"         // Operation aborted, e.g. gets a stop req or close req before an ongoing request is completed.

#define MSG_REASON_UNSPEC   "UNSPEC"        // Unspecified reason.

// Messages are for external communication (e.g. between Node and Srv).
class Msg {
public:
    Msg(char const *type = MSG_UNDEF, char const *to = MSG_UNDEF, char const *from = MSG_UNDEF, uint16_t seq = 0):
        m_seq(seq), m_len(sizeof(*this)) {
        // m_len to be updated in derived class constructors.
        FW_MSG_ASSERT(type && to && from);
        STRBUF_COPY(m_type, type);
        STRBUF_COPY(m_to, to);
        STRBUF_COPY(m_from, from);
    }
    // Must be non-virtual destructor to ensure no virtual table pointer is added.
    ~Msg() {}

    char const *GetType() const { return m_type; }
    char const *GetTo() const { return m_to; }
    char const *GetFrom() const { return m_from; }
    uint16_t GetSeq() const { return m_seq; }
    uint32_t GetLen() const { return m_len; }
    bool MatchSeq(uint16_t seq) const { return m_seq == seq; }
    enum {
        TYPE_LEN = 38,      // Role(16) ServicePrimitive(22)
        TO_LEN = 16,
        FROM_LEN = 16,
    };
    // Used by Node.cpp to fix up "to" and "from" when sending a msg.
    void SetTo(char const *to) {
        FW_MSG_ASSERT(to);
        STRBUF_COPY(m_to, to);
    }
    void SetFrom(char const *from) {
        FW_MSG_ASSERT(from);
        STRBUF_COPY(m_from, from);
    }
    void SetLen(uint32_t len) { m_len = len; }
    void SetSeq(uint16_t seq) { m_seq = seq; }
protected:
    char m_type[TYPE_LEN];
    char m_to[TO_LEN];
    char m_from[FROM_LEN];
    uint16_t m_seq;
    uint32_t m_len;         // Total length of message, including base and derived parts.
} __attribute__((packed));  // It must be 4-byte aligned.

class ErrorMsg : public Msg {
public:
    ErrorMsg(char const *type = MSG_UNDEF, char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC):
        Msg(type) {
        m_len = sizeof(*this);
        FW_MSG_ASSERT(error && origin && reason);
        STRBUF_COPY(m_error, error);
        STRBUF_COPY(m_origin, origin);
        STRBUF_COPY(m_reason, reason);
    }

    char const *GetError() const { return m_error; }
    char const *GetOrigin() const { return m_origin; }
    char const *GetReason() const {return m_reason; }
    bool IsSuccess() const {
        return STRBUF_EQUAL(m_error, MSG_ERROR_SUCCESS);
    }
    enum {
        ERROR_LEN = 16,
        ORIGIN_LEN = 16,
        REASON_LEN = 16,
    };
protected:
    char m_error[ERROR_LEN];
    char m_origin[ORIGIN_LEN];
    char m_reason[REASON_LEN];
} __attribute__((packed));  // It must be 4-byte aligned.

// Abstract base class for message-carrying events.
// Provides easy access to Msg members.
class MsgEvt : public Evt {
public:
    Msg &GetMsgBase() const { return m_msgBase; }
    char const *GetMsgType() const { return m_msgBase.GetType(); }
    char const *GetMsgTo() const { return m_msgBase.GetTo(); }
    char const *GetMsgFrom() const { return m_msgBase.GetFrom(); }
    uint16_t GetMsgSeq() const { return m_msgBase.GetSeq(); }
    uint32_t GetMsgLen() const { return m_msgBase.GetLen(); }
    void SetMsgTo(char const *to) { m_msgBase.SetTo(to); }
    void SetMsgSeq(uint16_t seq) { m_msgBase.SetSeq(seq); }
protected:
    MsgEvt(QP::QSignal signal, Msg &msg) :
        Evt(signal), m_msgBase(msg) {}
    MsgEvt(Msg &msg, QP::QEvt::StaticEvt /*dummy*/) :
        Evt(QP::QEvt::STATIC_EVT), m_msgBase(msg) {}
    // MsgEvt provides an interface to the Msg members. It cannot be copied.
    MsgEvt(MsgEvt const &e) = delete;
    MsgEvt &operator=(MsgEvt const &e) = delete;
    Msg &m_msgBase;
};

// Abstract base class for error-message-carrying events.
// Provides easy access to ErrorMsg (including Msg) members.
class ErrorMsgEvt : public MsgEvt {
public:
    ErrorMsg &GetErrorMsg() const { return m_errorMsg; }
    char const *GetMsgError() const { return m_errorMsg.GetError(); }
    char const *GetMsgOrigin() const { return m_errorMsg.GetOrigin(); }
    char const *GetMsgReason() const { return m_errorMsg.GetReason(); }
protected:
    // Passes/shares the msg reference to its base class.
    ErrorMsgEvt(QP::QSignal signal, ErrorMsg &msg) :
        MsgEvt(signal, msg), m_errorMsg(msg) {}
    ErrorMsgEvt(ErrorMsg &msg, QP::QEvt::StaticEvt) :
        MsgEvt(msg, QP::QEvt::STATIC_EVT), m_errorMsg(msg) {}
    // ErrorMsgEvt provides an interface to the ErrorMsg members. It cannot be copied.
    ErrorMsgEvt(ErrorMsgEvt const &e) = delete;
    ErrorMsgEvt &operator=(ErrorMsgEvt const &e) = delete;
    ErrorMsg &m_errorMsg;
};

// A concrete event carrying a Msg base object.
class MsgBaseEvt : public MsgEvt {
public:
    // A "default" constructor creates an unused static event.
    MsgBaseEvt(QP::QEvt::StaticEvt /*dummy*/) :
        MsgEvt(m_msg, QP::QEvt::STATIC_EVT){
        FW_MSG_ASSERT(&GetMsgBase() == &m_msg);
    }
    // Explicit copy assignment operator, mainly used to copy the base part of a concrete msg event derived from MsgEvt
    // to a "default" constructed static MsgBaseEvt object.
    // We make sure it DOESN'T copy the poolId_ and refCtr_ in the base class.
    MsgBaseEvt &operator=(MsgEvt const &e) {
        Evt::operator=(e);
        m_msg = e.GetMsgBase();
        // Set len to 0 to indicate it's NOT a valid length of the contained message type.
        m_msg.SetLen(0);
        return *this;
    }
protected:
    Msg m_msg;      // Msg base object (NOT a reference).
};

// A concrete event carrying an ErrorMsg base object.
class ErrorMsgBaseEvt : public ErrorMsgEvt {
public:
    // A "default" constructor creates an unused static event.
    ErrorMsgBaseEvt(QP::QEvt::StaticEvt /*dummy*/) :
        ErrorMsgEvt(m_msg, QP::QEvt::STATIC_EVT){
        FW_MSG_ASSERT(&GetMsgBase() == &m_msg);
    }
    // Explicit copy assignment operator, mainly used to copy the base part of a concrete msg event derived from ErrorMsgEvt
    // to a "default" constructed static ErrorMsgBaseEvt object.
    // We make sure it DOESN'T copy the poolId_ and refCtr_ in the base class.
    ErrorMsgBaseEvt &operator=(ErrorMsgEvt const &e) {
        Evt::operator=(e);
        m_msg = e.GetErrorMsg();
        // Set len to 0 to indicate it's NOT a valid length of the contained message type.
        m_msg.SetLen(0);
        return *this;
    }
protected:
    ErrorMsg m_msg;      // ErrorMsg base object (NOT a reference).
};

} // namespace FW

#endif // FW_MSG_H
