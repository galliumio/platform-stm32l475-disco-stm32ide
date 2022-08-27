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

#ifndef FW_BITSET_H
#define FW_BITSET_H

#include "fw_def.h"
#include "fw_assert.h"

#define FW_BITSET_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_bitset.h", (int_t)__LINE__))

namespace FW {

// Critical sections MUST be enforced externally by caller.
class Bitset {
public:
    // storCount is the number of elements in the stor[] array, i.e. number of uint32_t rather than bytes.
    Bitset(uint32_t stor[], uint32_t storCount, uint32_t maxBitCount) :
        m_stor(stor), m_storCount(storCount), m_maxBitCount(maxBitCount), m_setBitCount(0) {
        FW_BITSET_ASSERT(stor && storCount && maxBitCount);
        FW_BITSET_ASSERT(sizeof(stor[0]) == (BIT_PER_STOR / 8));
        FW_BITSET_ASSERT((storCount * BIT_PER_STOR) >= maxBitCount);
        Reset();
    }

    void Reset();
    void Set(uint32_t bit);
    void SetAll();
    void Clear(uint32_t bit);
    void ClearAll() { Reset(); }
    bool IsSet(uint32_t bit);
    bool IsAllSet() { return m_setBitCount == m_maxBitCount; }
    bool IsAllCleared() { return m_setBitCount == 0; }
    uint32_t GetMaxBitCount() { return m_maxBitCount; }
    uint32_t GetSetBitCount() { return m_setBitCount; }

protected:
    enum {
        BIT_PER_STOR = 32
    };

    uint32_t *m_stor;
    uint32_t m_storCount;
    uint32_t m_maxBitCount;
    uint32_t m_setBitCount;

    // Unimplemented to disallow built-in memberwise copy constructor and assignment operator.
    Bitset(Bitset const &);
    Bitset& operator= (Bitset const &);
};

} // namespace FW

#endif // FW_BITSET_H
