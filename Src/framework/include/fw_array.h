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

#ifndef FW_ARRAY_H
#define FW_ARRAY_H

#include <stdint.h>
#include <fw_assert.h>

#define FW_ARRAY_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_array.h", (int_t)__LINE__))

namespace FW {

template <class Type, size_t N>
class Array {
public:
    Array() : m_count(0) {}
    Array(uint32_t count, Type const &unused) : m_unused(unused), m_count(count) {
        FW_ARRAY_ASSERT(count <= N);
        for (uint32_t i = 0; i < count; i++) {
            m_array[i] = unused;
        }
    }
    // Use built-in copy constructor and copy assignment operators.
    virtual ~Array() {}

    Type &operator[](uint32_t index) {
        FW_ARRAY_ASSERT(index < m_count);
        return m_array[index];
    }
    // Array must have space to hold the new entry.
    void Append(Type const &t) {
        FW_ARRAY_ASSERT(m_count < N);
        m_array[m_count++] = t;
    }
    // Array can be empty and if so it does nothing.
    void DelLast() {
        if (m_count) {
            m_count--;
        }
    }
    void DelAt(uint32_t index) {
        if (index < m_count) {
            // m_count must be > 0
            m_count--;
            for (uint32_t i = index; i < m_count; i++) {
                m_array[i] = m_array[i+1];
            }
        }
    }
    // Mark an allocated entry as unused.
    // It is different from DelAt() since it does not shift entries following it.
    void Clear(uint32_t index) {
        FW_ARRAY_ASSERT(index < m_count);
        m_array[index] = m_unused;
    }
    uint32_t GetMax() const { return N; }
    uint32_t GetCount() const { return m_count; }
    uint32_t GetUsedCount() const {
        uint32_t count = 0;
        for (uint32_t i = 0; i < m_count; i++) {
            // Assumes Type has == operator.
            count += (m_array[i] == m_unused) ? 0 : 1;
        }
        return count;
    }
    // The following functions must be used with extreme caution:
    void SetCount(uint32_t count) {
        FW_ARRAY_ASSERT(count <= N);
        m_count = count;
    }
    Type *GetRaw() { return m_array; }
    Type const *GetRawConst() const { return m_array; }
private:
    Type m_array[N];
    Type m_unused;      // Used to mark unused but allocated entries.
    uint32_t m_count;
};

} // namespace FW

#endif // FW_ARRAY_H
