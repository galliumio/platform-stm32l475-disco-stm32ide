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

#ifndef FW_KV_H
#define FW_KV_H

#include <stddef.h>
#include "fw_assert.h"

#define FW_KV_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_kv.h", (int_t)__LINE__))

namespace FW {

// Critical sections MUST be enforced externally by caller.
template <class Key, class Value>
class KeyValue final {
public:
    KeyValue(Key k, Value v) :
        m_key(k), m_value(v) {}
    KeyValue() {}
    // Note This class cannot be inherited (final) so a virtual destructor is not needed.
    // This is to save a pointer to the virtual table per object of which the overhead can
    // be significant for small objects.

    // Non-constant version of Get functions will be added if needed.
    Key const &GetKey() const { return m_key; }
    Value const &GetValue() const { return m_value; }
    void SetKey(Key const &k) { m_key = k; }
    void SetValue(Value const &v) { m_value = v; }

protected:
    Key m_key;
    Value m_value;

    // Use built-in memberwise copy constructor and assignment operator.
    // KeyValue(KeyValue const &);
    // KeyValue& operator= (KeyValue const &);
};

} // namespace FW

#endif // FW_KV_H
