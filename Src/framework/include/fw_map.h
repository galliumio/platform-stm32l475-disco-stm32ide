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

#ifndef FW_MAP_H
#define FW_MAP_H

#include <stddef.h>
#include "fw_kv.h"
#include "fw_assert.h"

#define FW_MAP_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_map.h", (int_t)__LINE__))

namespace FW {

// Critical sections MUST be enforced externally by caller.
template <class Key, class Value>
class Map {
public:
    Map(KeyValue<Key, Value> kv[], uint32_t count, KeyValue<Key, Value> const &unusedKv) :
        m_kv(kv), m_count(count), m_unusedKv(unusedKv) {
        // m_kv and m_count valided in Reset().
        Reset();
    }
    // Default constructor with m_kv and m_count set to null/0. It allows an array of Map objects
    // to be initialized in its owner's constructor.
    // A default constructed object is unusable until initialized.
    Map(): m_kv(NULL), m_count (0) {}
    virtual ~Map() {}

    // Init() can only be called once if a Map object is default constructed.
    void Init(KeyValue<Key, Value> kv[], uint32_t count, KeyValue<Key, Value> const &unusedKv) {
        FW_MAP_ASSERT((m_kv == NULL) && (m_count == 0));
        m_kv = kv;
        m_count = count;
        m_unusedKv = unusedKv;
        // m_kv and m_count valided in Reset().
        Reset();
    }
    void Reset() {
        FW_MAP_ASSERT(m_kv && m_count);
        for (uint32_t i = 0; i < m_count; i++) {
            m_kv[i] = m_unusedKv;
        }
    }

    KeyValue<Key, Value> *GetByIndex(uint32_t index) {
        FW_MAP_ASSERT(m_kv && (index < m_count));
        return &m_kv[index];
    }
    void ClearByIndex(uint32_t index) {
        FW_MAP_ASSERT(m_kv && (index < m_count));
        m_kv[index] = m_unusedKv;
    }
    // OK to pass unused key to find first empty slot.
    // It returns "by-reference" instead of "by-value", and therefore a caller can modify the returned entry.
    KeyValue<Key, Value> *GetByKey(Key const &k) {
        FW_MAP_ASSERT(m_kv && m_count);
        for (uint32_t i = 0; i < m_count; i++) {
            if (m_kv[i].GetKey() == k) {
                return &m_kv[i];
            }
        }
        return NULL;
    }
    bool ClearByKey(Key const &k) {
        KeyValue<Key, Value> *kv = GetByKey(k);
        if (kv) {
            *kv = m_unusedKv;
            return true;
        }
        return false;
    }
    // Since there can be more that one keys that map to the same value,
    // this function returns the first one found.
    KeyValue<Key, Value> *GetFirstByValue(Value const &v) {
        FW_MAP_ASSERT(m_kv && m_count);
        for (uint32_t i = 0; i < m_count; i++) {
            if (m_kv[i].GetValue() == v) {
                return &m_kv[i];
            }
        }
        return NULL;
    }
    void Save(KeyValue<Key, Value> const &kv) {
        KeyValue<Key, Value> *unusedKv = GetByKey(m_unusedKv.GetKey());
        FW_MAP_ASSERT(unusedKv);
        *unusedKv = kv;
    }
    void Put(uint32_t index, KeyValue<Key, Value> const &kv) {
        KeyValue<Key, Value> *putKv = GetByIndex(index);
        FW_MAP_ASSERT(putKv);
        *putKv = kv;
    }

    KeyValue<Key, Value> const &GetUnusedKv() const { return m_unusedKv; }
    Key const &GetUnusedKey() const { return m_unusedKv.GetKey(); }

    uint32_t GetUnusedCount() const {
        FW_MAP_ASSERT(m_kv && m_count);
        uint32_t count = 0;
        for (uint32_t i = 0; i < m_count; i++) {
            if (m_kv[i].GetKey() == m_unusedKv.GetKey()) {
                count++;
            }
        }
        return count;
    }
    uint32_t GetUsedCount() const {
        return m_count - GetUnusedCount();
    }
    uint32_t GetTotalCount() const { return m_count; }
    bool IsFull() const { return GetUnusedCount() == 0; }
    bool IsEmpty() const { return GetUsedCount() == 0; }

protected:
    KeyValue<Key, Value> *m_kv;
    uint32_t m_count;
    KeyValue<Key, Value> m_unusedKv;

    // Unimplemented to disallow built-in memberwise copy constructor and assignment operator.
    Map(Map const &);
    Map& operator= (Map const &);
};

} // namespace FW

#endif // FW_MAP_H
