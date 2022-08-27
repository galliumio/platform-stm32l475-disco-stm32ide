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

#ifndef FW_EVT_H
#define FW_EVT_H

#include "qpcpp.h"
#include "fw_def.h"
#include "fw_error.h"

#define EVT_CAST(e_)            static_cast<FW::Evt const &>(e_)
#define ERROR_EVT_CAST(e_)      static_cast<FW::ErrorEvt const &>(e_)

namespace FW {

class Evt : public QP::QEvt {
public:
    static void *operator new(size_t s);
    static void operator delete(void *evt);

    // Constructor to create a dynamic event allocated from an event pool.
    Evt(QP::QSignal signal, Hsmn to = HSM_UNDEF, Hsmn from = HSM_UNDEF, Sequence seq = 0) :
        QP::QEvt(signal), m_to(to), m_from(from), m_seq(seq) {}
    // Constructor to create a static event.
    Evt(QP::QSignal signal, Hsmn to, Hsmn from, Sequence seq, QP::QEvt::StaticEvt /*dummy*/) :
        QP::QEvt(signal, QP::QEvt::STATIC_EVT), m_to(to), m_from(from), m_seq(seq) {}
    // A "default" constructor creates an unused static event.
    Evt(QP::QEvt::StaticEvt /*dummy*/) :
        QP::QEvt(0, QP::QEvt::STATIC_EVT), m_to(HSM_UNDEF), m_from(HSM_UNDEF), m_seq(0) {}
    ~Evt() {}

    // Explicit copy assignment operator, mainly used to copy to a "default" constructed static event.
    // We make sure it DOESN'T copy the poolId_ and refCtr_ in the base class.
    // (Note - Copy assignment operator of its base class is NOT automatically invoked.)
    Evt &operator=(Evt const &e) {
        sig = e.sig;
        m_to = e.m_to;
        m_from = e.m_from;
        m_seq = e.m_seq;
        return *this;
    }

    void Clear() {
        sig = 0;
        m_to = HSM_UNDEF;
        m_from = HSM_UNDEF;
        m_seq = 0;
    }
    bool InUse() const { return sig != 0; }
    Hsmn GetTo() const { return m_to; }
    Hsmn GetFrom() const { return m_from; }
    Sequence GetSeq() const { return m_seq; }
    void SetTo(Hsmn to) { m_to = to; }
    void SetFrom(Hsmn from) { m_from = from; }
    void SetSeq(Sequence seq) { m_seq = seq; }

protected:
    Hsmn m_to;
    Hsmn m_from;
    Sequence m_seq;
};

class ErrorEvt : public Evt {
public:
    ErrorEvt(QP::QSignal signal, Hsmn to = HSM_UNDEF, Hsmn from = HSM_UNDEF, Sequence seq = 0,
             Error error = ERROR_SUCCESS, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        Evt(signal, to, from, seq), m_error(error), m_origin(origin), m_reason(reason) {}
    ErrorEvt(QP::QSignal signal, Error error = ERROR_SUCCESS, Hsmn origin = HSM_UNDEF, Reason reason = 0) :
        Evt(signal), m_error(error), m_origin(origin), m_reason(reason) {}
    // A "default" constructor creates an unused static event.
    ErrorEvt(QP::QEvt::StaticEvt /*dummy*/) :
        Evt(QP::QEvt::STATIC_EVT), m_error(ERROR_SUCCESS), m_origin(HSM_UNDEF), m_reason(0) {}

    // Assignment operator, used to copy to a "default" constructed static event.
    // (Note - Copy assignment operator of its base class is NOT automatically invoked.)
    ErrorEvt &operator=(ErrorEvt const &e) {
        Evt::operator=(e);
        m_error = e.m_error;
        m_origin = e.m_origin;
        m_reason = e.m_reason;
        return *this;
    }

    void Clear() {
        m_error = ERROR_SUCCESS;
        m_origin = HSM_UNDEF;
        m_reason = 0;
    }
    Error GetError() const { return m_error; }
    Hsmn GetOrigin() const { return m_origin; }
    Reason GetReason() const {return m_reason; }
protected:
    Error m_error;      // Common error code.
    Hsmn m_origin;      // HSM originating the error.
    Reason m_reason;    // CFM/RSP event specific reason code
};

} // namespace FW

#endif // FW_EVT_H
