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

#ifndef FW_PIPE_H
#define FW_PIPE_H

#include "fw_def.h"
#include "fw_evt.h"
#include "fw_error.h"
#include "fw_assert.h"

#define FW_PIPE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_pipe.h", (int_t)__LINE__))

namespace FW {

// Critical sections are enforced internally.
template <class Type>
class Pipe {
public:
    Pipe(Type stor[], uint8_t order) :
        m_stor(stor), m_mask(BIT_MASK_OF_SIZE(order)),
        m_writeIndex(0), m_readIndex(0), m_truncated(false) {
        // Arithmetic in this class (m_mask + 1) assumes order < 32.
        // BIT_MASK_OF_SIZE() assumes order > 0
        FW_PIPE_ASSERT(stor && (order > 0) and (order < 32));
    }
    virtual ~Pipe() {}

    void Reset() {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        m_writeIndex = 0;
        m_readIndex = 0;
        m_truncated = false;
        QF_CRIT_EXIT(crit);
    }
    bool IsTruncated() const { return m_truncated; }
    uint32_t GetWriteIndex() const { return m_writeIndex; }
    uint32_t GetReadIndex() const { return m_readIndex; }
    uint32_t GetUsedCount() const {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        uint32_t count = GetUsedCountNoCrit();
        QF_CRIT_EXIT(crit);
        return count;
    }
    uint32_t GetUsedCountNoCrit() const {
        return (m_writeIndex - m_readIndex) & m_mask;
    }
    // Gets the largest contiguous used block size in byte.
    uint32_t GetUsedBlockCount() const {
        uint32_t count;
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        count = GetUsedCountNoCrit();
        if ((m_readIndex + count) > (m_mask + 1)) {
            count = m_mask + 1 - m_readIndex;
        }
        QF_CRIT_EXIT(crit);
        return count;
    }
    uint32_t GetAvailCount() const {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        uint32_t count = GetAvailCountNoCrit();
        QF_CRIT_EXIT(crit);
        return count;
    }
    // Since (m_readIndex == m_writeIndex) is regarded as empty, the maximum available count =
    // total storage - 1, i.e. m_mask.
    uint32_t GetAvailCountNoCrit() const {
        return (m_readIndex - m_writeIndex - 1) & m_mask;
    }
    // Gets the largest contiguous unused block size in byte.
    uint32_t GetAvailBlockCount() const {
        uint32_t count;
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        count = GetAvailCountNoCrit();
        if ((m_writeIndex + count) > (m_mask + 1)) {
            count = m_mask + 1 - m_writeIndex;
        }
        QF_CRIT_EXIT(crit);
        return count;
    }
    uint32_t GetDiff(uint32_t a, uint32_t b) { return (a - b) & m_mask; }
    uint32_t GetAddr(uint32_t index) { return reinterpret_cast<uint32_t>(&m_stor[index & m_mask]); }
    Type&    GetRef(uint32_t index) { return m_stor[index & m_mask]; }
    uint32_t GetWriteAddr() { return GetAddr(m_writeIndex); }
    Type&    GetWriteRef() { return GetRef(m_writeIndex); }
    uint32_t GetReadAddr() { return GetAddr(m_readIndex); }
    Type&    GetReadRef() { return GetRef(m_readIndex); }
    // Returns one byte past the max buffer address. Important - It is not valid to write to /read from this address.
    uint32_t GetEndAddr() { return reinterpret_cast<uint32_t>(&m_stor[m_mask + 1]); }
    uint32_t GetBufSize() { return (m_mask + 1); }
    void IncWriteIndex(uint32_t count) {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        IncIndex(m_writeIndex, count);
        QF_CRIT_EXIT(crit);
    }
    void IncReadIndex(uint32_t count) {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        IncIndex(m_readIndex, count);
        QF_CRIT_EXIT(crit);
    }
    void IncWriteIndexNoCrit(uint32_t count) {
        IncIndex(m_writeIndex, count);
    }
    void IncReadIndexNoCrit(uint32_t count) {
        IncIndex(m_readIndex, count);
    }

    // Return written count. If not enough space to write all, return 0 (i.e. no partial write).
    // If overflow has occurred set m_truncated; otherwise clear m_truncated.
    uint32_t Write(Type const *src, uint32_t count, bool *status = NULL) {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        count = WriteNoCrit(src, count, status);
        QF_CRIT_EXIT(crit);
        return count;
    }

    // Without critical section.
    uint32_t WriteNoCrit(Type const *src, uint32_t count, bool *status = NULL) {
        FW_PIPE_ASSERT(src);
        bool wasEmpty = IsEmpty();
        if (count > GetAvailCountNoCrit()) {
            m_truncated = true;
            count = 0;
        } else {
            m_truncated = false;
            if ((m_writeIndex + count) <= (m_mask + 1)) {
                WriteBlock(src, count);
            } else {
                uint32_t partial = m_mask + 1 - m_writeIndex;
                WriteBlock(src, partial);
                WriteBlock(src + partial, count - partial);
            }
        }
        if (status) {
            if (count && wasEmpty) {
                *status = true;
            } else {
                *status = false;
            }
        }
        return count;
    }

    // Writes a single entry without critical section.
    bool WriteNoCrit(Type const &s) {
        if (GetAvailCountNoCrit() == 0) {
            return false;
        }
        m_stor[m_writeIndex] = s;
        IncIndex(m_writeIndex, 1);
        return true;
    }

    // Return actual read count. Okay if data in pipe < count.
    uint32_t Read(Type *dest, uint32_t count, bool *status = NULL) {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        count = ReadNoCrit(dest, count, status);
        QF_CRIT_EXIT(crit);
        return count;
    }

    // Without critical section.
    uint32_t ReadNoCrit(Type *dest, uint32_t count, bool *status = NULL) {
        FW_PIPE_ASSERT(dest);
        uint32_t used = GetUsedCountNoCrit();
        count = LESS(count, used);
        if ((m_readIndex + count) <= (m_mask + 1)) {
            ReadBlock(dest, count);
        } else {
            uint32_t partial = m_mask + 1 - m_readIndex;
            ReadBlock(dest, partial);
            ReadBlock(dest + partial, count - partial);
        }
        if (status) {
            if (count && IsEmpty()) {
                // Currently use "empty" as condition, but it can be half-empty, etc.
                *status = true;
            } else {
                *status = 0;
            }
        }
        return count;
    }

    // Reads a single entry without critical section.
    bool ReadNoCrit(Type &d) {
        if (IsEmpty()) {
            return false;
        }
        d = m_stor[m_readIndex];
        IncIndex(m_readIndex, 1);
        return true;
    }

    // Performs the cache operation passed in on the read buffer for the specified count (in unit of T).
    // The callback function will be called once or twice to receive a starting address and byte len to operate on.
    // The operation can be Clean (memory to device) or Clean-and-invalidate (device to memory).
    // Important - Cache line alignment (e.g. 32 bytes for Cortex-M7) is not handled by this function. It must be done
    // by the callback function.
    void CacheOp(void (*op)(uint32_t addr, uint32_t len), uint32_t count) {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        CacheOpNoCrit(op, count);
        QF_CRIT_EXIT(crit);
    }

    // Without critical section.
    void CacheOpNoCrit(void (*op)(uint32_t addr, uint32_t len), uint32_t count) {
        if ((m_readIndex + count) <= (m_mask + 1)) {
            op(GetReadAddr(), count * sizeof(Type));
        } else {
            uint32_t partial = m_mask + 1 - m_readIndex;
            op(GetReadAddr(), partial * sizeof(Type));
            op(GetAddr(0), (count - partial) * sizeof(Type));
        }
    }

    // To be used with caution.
    // index must be in the used range.
    void Delete(uint32_t index) {
        QF_CRIT_STAT_TYPE crit;
        QF_CRIT_ENTRY(crit);
        DeleteNoCrit(index);
        QF_CRIT_EXIT(crit);
    }

    // To be used with caution.
    // Without critical section.
    void DeleteNoCrit(uint32_t index) {
        // Ensures index is valid.
        uint32_t offset = GetDiff(index, m_readIndex);
        uint32_t usedCount = GetUsedCountNoCrit();
        FW_PIPE_ASSERT(offset < usedCount);
        // If index is closer to the readIndex end, move entries from that end to the overwrite deleted entry.
        // Otherwise, move entries from the writeIndex end to overwrite the deleted entry.
        uint32_t nextIndex = index;
        if (offset <= (usedCount >> 1)) {
            // Note unsigned comparison must not be >= 0.
            for (uint32_t i = offset; i > 0; i--) {
                DecIndex(nextIndex, 1);
                m_stor[index] = m_stor[nextIndex];
                index = nextIndex;
            }
            IncIndex(m_readIndex, 1);
        } else {
            // Normally the ending condition should be "i < (usedCount - 1)", since the entry
            // at writeIndex is regarded to be unused and does not need to be moved.
            // However some application may use the entry at writeIndex as a scratch buffer and
            // commit it only when it is finalized. To accommodate that use case, the loop below
            // covers the entry at the writeIndex as well. It won't hurt when it is not used.
            // This logic MUST NOT be changed, as it is depended on by fw_histbuf.h
            for (uint32_t i = offset; i < usedCount; i++) {
                IncIndex(nextIndex, 1);
                m_stor[index] = m_stor[nextIndex];
                index = nextIndex;
            }
            DecIndex(m_writeIndex, 1);
        }
    }

protected:
    // Write contiguous block to m_stor. count can be 0.
    // Without critical section.
    void WriteBlock(Type const *src, uint32_t count) {
        FW_PIPE_ASSERT(src && ((m_writeIndex + count) <= (m_mask + 1)));
        for (uint32_t i = 0; i < count; i++) {
            m_stor[m_writeIndex + i] = src[i];
        }
        IncIndex(m_writeIndex, count);
    }
    // Read contiguous block from m_stor. count can be 0.
    // Without critical section.
    void ReadBlock(Type *dest, uint32_t count) {
        FW_PIPE_ASSERT(dest && ((m_readIndex + count) <= (m_mask + 1)));
        for (uint32_t i = 0; i < count; i++) {
            dest[i] = m_stor[m_readIndex + i];
        }
        IncIndex(m_readIndex, count);
    }
    void IncIndex(uint32_t &index, uint32_t count) {
        index = (index + count) & m_mask;
    }
    void DecIndex(uint32_t &index, uint32_t count) {
        index = (index - count) & m_mask;
    }
    bool IsEmpty() {
        return (m_readIndex == m_writeIndex);
    }

    Type *      m_stor;
    uint32_t    m_mask;
    uint32_t    m_writeIndex;
    uint32_t    m_readIndex;
    bool        m_truncated;
    // For future enhancement.
    //QP::QSignal m_halfEmptySig; // signal to send when pipe has just crossed half-empty threshold upon read.

    // Unimplemented to disallow built-in memberwise copy constructor and assignment operator.
    Pipe(Pipe const &);
    Pipe& operator= (Pipe const &);
};

// Common template instantiation
typedef Pipe<uint8_t> Fifo;

} // namespace FW

#endif //FW_PIPE_H
