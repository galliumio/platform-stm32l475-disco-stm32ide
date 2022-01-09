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

#include <string.h>
#include "qpcpp.h"
#include "fw_bitset.h"
#include "fw_assert.h"

FW_DEFINE_THIS_FILE("fw_bitset.cpp")

using namespace QP;

namespace FW {

void Bitset::Reset() {
    m_setBitCount = 0;
    uint32_t count = ROUND_UP_DIV(m_maxBitCount, BIT_PER_STOR);
    FW_ASSERT(count <= m_storCount);
    for (uint32_t index = 0; index < count; index++) {
        m_stor[index] = 0;
    }
}

void Bitset::Set(uint32_t bit) {
    FW_ASSERT(bit < m_maxBitCount);
    uint32_t index = bit / BIT_PER_STOR;
    uint32_t bitmask = BIT_MASK_AT(bit % BIT_PER_STOR);
    if ((m_stor[index] & bitmask) == 0) {
        m_stor[index] |= bitmask;
        FW_ASSERT(m_setBitCount < m_maxBitCount);
        m_setBitCount++;
    }
}

// Only set the bits actually used (depending on m_maxBitCount)
// for ease of comparison between Bitset objects.
void Bitset::SetAll() {
    m_setBitCount = m_maxBitCount;
    uint32_t count = ROUND_UP_DIV(m_maxBitCount, BIT_PER_STOR);
    FW_ASSERT((count > 0) && (count <= m_storCount));
    uint32_t index;
    for (index = 0; index < count - 1; index++) {
        m_stor[index] = BIT_MASK_OF_SIZE(BIT_PER_STOR);
    }
    uint32_t remainder = m_maxBitCount - (index * BIT_PER_STOR);
    FW_ASSERT((remainder > 0) && (remainder <= BIT_PER_STOR));
    m_stor[index] = BIT_MASK_OF_SIZE(remainder);
}

void Bitset::Clear(uint32_t bit) {
    FW_ASSERT(bit < m_maxBitCount);
    uint32_t index = bit / BIT_PER_STOR;
    uint32_t bitmask = BIT_MASK_AT(bit % BIT_PER_STOR);
    if (m_stor[index] & bitmask) {
        m_stor[index] &= ~bitmask;
        FW_ASSERT(m_setBitCount > 0);
        m_setBitCount--;
    }
}

bool Bitset::IsSet(uint32_t bit) {
    FW_ASSERT(bit < m_maxBitCount);
    uint32_t index = bit / BIT_PER_STOR;
    uint32_t bitmask = BIT_MASK_AT(bit % BIT_PER_STOR);
    return m_stor[index] & bitmask;
}

} // namespace FW
