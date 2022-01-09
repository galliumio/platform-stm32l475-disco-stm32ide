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

#ifndef NODE_PARSER_H
#define NODE_PARSER_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"
#include "NodeParserInterface.h"

using namespace QP;
using namespace FW;

namespace APP {

class NodeParser : public Region {
public:
    NodeParser();

protected:
    static QState InitialPseudoState(NodeParser * const me, QEvt const * const e);
    static QState Root(NodeParser * const me, QEvt const * const e);
        static QState Stopped(NodeParser * const me, QEvt const * const e);
        static QState Started(NodeParser * const me, QEvt const * const e);
            static QState HeaderWait(NodeParser * const me, QEvt const * const e);
            static QState BodyWait(NodeParser * const me, QEvt const * const e);

    Hsmn m_manager;                 // Managing HSM (i.e. Node)
    Fifo *m_dataInFifo;             // FIFO carrying data from Wifi.
    NodeParserMsgInd *m_msgInd;     // Saves partially received message event.
    uint32_t m_dataLen;             // No. of bytes of data to be received next.
    uint32_t m_msgIdx;              // Index to the msg buffer to write to next.

    Timer m_stateTimer;


#define NODE_PARSER_TIMER_EVT \
    ADD_EVT(STATE_TIMER)

#define NODE_PARSER_INTERNAL_EVT \
    ADD_EVT(DONE) \
    ADD_EVT(NEXT) \
    ADD_EVT(DATA_DONE)

#undef ADD_EVT
#define ADD_EVT(e_) e_,

    enum {
        NODE_PARSER_TIMER_EVT_START = TIMER_EVT_START(NODE_PARSER),
        NODE_PARSER_TIMER_EVT
    };

    enum {
        NODE_PARSER_INTERNAL_EVT_START = INTERNAL_EVT_START(NODE_PARSER),
        NODE_PARSER_INTERNAL_EVT
    };
};

} // namespace APP

#endif // NODE_PARSER_H
