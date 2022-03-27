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

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_active.h"
#include "fw_xthread.h"
#include "fw_assert.h"

FW_DEFINE_THIS_FILE("fw_region.cpp")

using namespace QP;

namespace FW {

void Region::Init(Active *container) {
    FW_ASSERT(container);
    m_container = container;
    container->Add(this);
    m_hsm.Init(container);
    QHsm::init(0);
}

void Region::Init(XThread *container) {
    FW_ASSERT(container);
    m_container = container;
    container->Add(this);
    m_hsm.Init(container);
    QHsm::init(0);
}

void Region::Dispatch(QEvt const * const e) {
    // For region, e can be from the container active object's event queue (dynamic or static/timer),
    // or be a static event on the stack of the container active object.
    // Garbage collection, if needed, is done by the caller.
    QHsm::dispatch(e, 0);
    // Handle all reminder events generated as a result of e.
    m_hsm.DispatchReminder();
}

void Region::PostSync(Evt const *e) {
    FW_ASSERT(e && m_container);
    m_container->postLIFO(e);
}

} // namespace FW
