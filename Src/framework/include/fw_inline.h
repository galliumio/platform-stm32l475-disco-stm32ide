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

#ifndef FW_INLINE_H
#define FW_INLINE_H

#include "qpcpp.h"

// Include the following inline functions defined in ../qpcpp/source/qf_pkg.h.
// They are required to access private members of QEvt.
// Alternatively we can include ../qpcpp/source/qf_pkg.h. But that header file
// was intended for internal use within qpcpp.
// Note - In qpcpp 5.9.7 QF_EVT_POOL_ID_() and QF_EVT_REF_CTR_() were
//        removed from qf_pkg.h. They are still "friends" of class QEvt.
//        If they are removed as "friends" in the future they need to be patched.

//! helper macro to cast const away from an event pointer @p e_
#define QF_EVT_CONST_CAST_(e_) const_cast<QEvt *>(e_)
namespace QP {
//! access to the poolId_ of an event @p e
inline uint8_t QF_EVT_POOL_ID_(QEvt const * const e) { return e->poolId_; }
//! access to the refCtr_ of an event @p e
inline uint8_t QF_EVT_REF_CTR_(QEvt const * const e) { return e->refCtr_; }
//! decrement the refCtr_ of an event @p e
inline void QF_EVT_REF_CTR_DEC_(QEvt const * const e) {
    --(QF_EVT_CONST_CAST_(e))->refCtr_;
}
} // namespace QP

#endif // FW_INLINE_H
