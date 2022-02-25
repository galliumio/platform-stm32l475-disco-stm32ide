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

#ifndef FW_SEQREC_H
#define FW_SEQREC_H

#include <stddef.h>
#include "fw_def.h"
#include "fw_map.h"
#include "fw_strbuf.h"
#include "fw_msg.h"
#include "fw_assert.h"

#define FW_SEQREC_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_seqrec.h", (int_t)__LINE__))

namespace FW {

// Critical sections MUST be enforced externally by caller.
// A Sequence Record class to maintain records of outgoing sequence numbers sent in req/ind events/messages.
// It helps match a received sequence no. in a cfm/rsp event/message against those sent.
template <class Type, size_t N>
class SeqRec {
public:
    SeqRec(Type unusedKey) : m_map(m_mapStor, N, KeyValue<Type, Sequence>(unusedKey, 0)) {}
    void Reset() {
        m_map.Reset();
    }
    void Save(Type key, Sequence seq, bool reset) {
        FW_SEQREC_ASSERT(key != m_map.GetUnusedKey());
        if (reset) {
            Reset();
        }
        m_map.Save(KeyValue<Type, Sequence>(key, seq));
    }
    bool Match(Type key, Sequence seqToMatch) {
        if (key == m_map.GetUnusedKey()) {
            return false;
        }
        KeyValue<Type, Sequence> *kv = m_map.GetByKey(key);
        if (kv == NULL) {
            return false;
        }
        if (kv->GetValue() != seqToMatch) {
            return false;
        }
        // Clear entry once matched.
        *kv = m_map.GetUnusedKv();
        return true;
    }
    void Clear(Type key) { m_map.ClearByKey(key); }
    bool IsCleared(Type key) {
        FW_SEQREC_ASSERT(key != m_map.GetUnusedKey());
        return (m_map.GetByKey(key) == NULL);
    }
    bool IsAllCleared() {
        return (m_map.GetUsedCount() == 0);
    }

protected:
    KeyValue<Type, Sequence> m_mapStor[N];
    Map<Type, Sequence> m_map;
};

// Common template instantiation
using MsgSeqRec = SeqRec<StrBuf<Msg::TO_LEN>, 8>;
using EvtSeqRec = SeqRec<Hsmn, 16>;

} // namespace FW

#endif // FW_SEQREC_H
