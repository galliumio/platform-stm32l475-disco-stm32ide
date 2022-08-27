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

#ifndef FW_HISTBUF_H
#define FW_HISTBUF_H

#include <stdint.h>
#include "qpcpp.h"
#include "fw_pipe.h"
#include "fw_strbuf.h"
#include "fw_assert.h"

#define FW_HISTBUF_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_histbuf.h", (int_t)__LINE__))

namespace FW {

template <size_t N>
class HistBuf : public Pipe<StrBuf<N>> {
public:
    enum {
        ORDER = 3,
    };
    typedef StrBuf<N> Type;

    HistBuf() : Pipe<Type>(m_stor, ORDER), m_browseIndex(0) {};
    virtual ~HistBuf() {}

    void Reset() {
        Pipe<Type>::Reset();
        m_browseIndex = 0;
        for (uint32_t i = 0; i < ARRAY_COUNT(m_stor); i++) {
            m_stor[i].Clear();
        }
    }
    Type &EditBuf() { return Pipe<Type>::GetWriteRef(); }
    Type &BrowseBuf() { return Pipe<Type>::GetRef(m_browseIndex); }
    bool Up(uint32_t &oldLen) {
        oldLen = BrowseBuf().GetLen();
        if (m_browseIndex != Pipe<Type>::GetReadIndex()) {
            Pipe<Type>::DecIndex(m_browseIndex, 1);
            return true;
        }
        return false;
    }
    bool Down(uint32_t &oldLen) {
        oldLen = BrowseBuf().GetLen();
        if (m_browseIndex != Pipe<Type>::GetWriteIndex()) {
            Pipe<Type>::IncIndex(m_browseIndex, 1);
            return true;
        }
        return false;
    }

    void ResetBrowseIndex() {
        m_browseIndex = Pipe<Type>::GetWriteIndex();
    }

    void CopyBrowseToEdit() {
        if (m_browseIndex != Pipe<Type>::GetWriteIndex()) {
            Pipe<Type>::GetWriteRef() = Pipe<Type>::GetRef(m_browseIndex);
            ResetBrowseIndex();
        }
    }
    void CommitEdit() {
        if (EditBuf().IsEmpty()) {
            // No need to commit empty command.
            FW_HISTBUF_ASSERT(m_browseIndex == Pipe<Type>::GetWriteIndex());
            return;
        }
        // Delete the first found repeated entry to save space.
        uint32_t usedCount = Pipe<Type>::GetUsedCountNoCrit();
        uint32_t index = Pipe<Type>::GetReadIndex();
        for (uint32_t i = 0; i < usedCount; i++) {
            if (Pipe<Type>::GetRef(index) == EditBuf()) {
                Pipe<Type>::DeleteNoCrit(index);
                // Must break after deletion since index may not be valid any more.
                break;
            }
            Pipe<Type>::IncIndex(index, 1);
        }
        // If pipe full, discard the oldest entry at readIndex.
        if (Pipe<Type>::GetAvailCountNoCrit() == 0) {
            Pipe<Type>::IncReadIndexNoCrit(1);
            FW_HISTBUF_ASSERT(Pipe<Type>::GetAvailCountNoCrit() == 1);
        }
        Pipe<Type>::IncWriteIndexNoCrit(1);
        EditBuf().Clear();
        ResetBrowseIndex();
    }

protected:
    Type m_stor[1 << ORDER];
    uint32_t m_browseIndex;
};

} // namespace FW

#endif // FW_HISTBUF_H
