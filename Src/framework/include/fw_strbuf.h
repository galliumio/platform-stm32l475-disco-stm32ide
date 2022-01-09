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

#ifndef FW_STRBUF_H
#define FW_STRBUF_H

#include <stdint.h>
#include "qpcpp.h"
#include "fw_macro.h"
#include "fw_array.h"
#include "fw_assert.h"

#define FW_STRBUF_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_strbuf.h", (int_t)__LINE__))

namespace FW {

template <size_t N>
class StrBuf : public Array<char, N> {
public:
    StrBuf() {
        Clear();
    }
    StrBuf(char const *s) {
        FW_STRBUF_ASSERT(s);
        // STRING_COPY guarantees null termination.
        STRING_COPY((Array<char, N>::GetRaw()), s, N);
        // Count includes null termination.
        Array<char, N>::SetCount(strlen(Array<char, N>::GetRaw())+1);
    }
    // Alternative implementation.
#if 0
    StrBuf(char const *s) {
        FW_STRBUF_ASSERT(s);
        uint32_t len = strlen(s);
        len = LESS(len, N-1);
        for (uint32_t i = 0; i < len; i++) {
            Array<char, N>::Append(s[i]);
        }
        // Adds null termination.
        Array<char, N>::Append(0);
    }
#endif
    virtual ~StrBuf() {}

    // Overridden copy constructor for efficiency.
    StrBuf(StrBuf const &s) : Array<char, N>() {
        STRING_COPY((Array<char, N>::GetRaw()), s.GetRawConst(), N);
        Array<char, N>::SetCount(s.GetCount());
    }
    // Overridden copy assignment operator for efficiency.
    StrBuf &operator=(StrBuf const &s) {
        STRING_COPY((Array<char, N>::GetRaw()), s.GetRawConst(), N);
        Array<char, N>::SetCount(s.GetCount());
        return *this;
    }

    bool operator==(StrBuf const &s) const {
        if (GetLen() == s.GetLen()) {
            // Needs to add scope operator for 1st parameter or it causes compiler error. Alternatively, add "this->".
            return !strncmp(Array<char, N>::GetRawConst(), s.GetRawConst(), N);
        }
        return false;
    }
    bool operator!=(StrBuf const &s) const { return !(*this == s); }

    void Clear() {
        Array<char, N>::SetCount(0);
        // Adds null termination.
        Array<char, N>::Append(0);
    }

    // Overridden function (non-virtual)
    // Asserts if full.
    void Append(char ch) {
        // Deletes null termination.
        Array<char, N>::DelLast();
        Array<char, N>::Append(ch);
        Array<char, N>::Append(0);
    }
    // Overridden function (non-virtual)
    void DelLast() {
        // Deletes null termination.
        Array<char, N>::DelLast();
        Array<char, N>::DelLast();
        Array<char, N>::Append(0);
    }
    // GetMaxLen() and GetLen() do NOT include null termination.
    uint32_t GetMaxLen() const { return N - 1; }
    uint32_t GetLen() const {
        uint32_t count = Array<char, N>::GetCount();
        FW_STRBUF_ASSERT(count > 0);
        return count - 1;
    }
    bool IsEmpty() const { return GetLen() == 0; }
    bool IsFull() const {
        // Minus 1 for null termination.
        return GetLen() == (N - 1);
    }
};

} // namespace FW

#endif // FW_STRBUF_H
