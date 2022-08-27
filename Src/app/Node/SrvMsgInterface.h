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

#ifndef SRV_MSG_INTERFACE_H
#define SRV_MSG_INTERFACE_H

#include "fw_def.h"
#include "fw_msg.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

// This file defines messages for the "Srv" (Server) role.

class SrvAuthReqMsg final: public Msg {
public:
    SrvAuthReqMsg(char const *username = MSG_UNDEF, char const *password = MSG_UNDEF, char const *nodeId = MSG_UNDEF) :
        Msg("SrvAuthReqMsg") {
        m_len = sizeof(*this);
        STRBUF_COPY(m_username, username);
        STRBUF_COPY(m_password, password);
        STRBUF_COPY(m_nodeId, nodeId);
    }
    char const *GetUsername() const { return m_username; }
    char const *GetPassword() const { return m_password; }
    char const *GetNodeId() const { return m_nodeId; }
    enum {
        USERNAME_LEN = 32,
        PASSWORD_LEN = 32,
        NODE_ID_LEN = 16
    };
protected:
    char m_username[USERNAME_LEN];
    char m_password[PASSWORD_LEN];
    char m_nodeId[NODE_ID_LEN];
} __attribute__((packed));

class SrvAuthCfmMsg final: public ErrorMsg {
public:
    SrvAuthCfmMsg(char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC,
                  char const *nodeId = MSG_UNDEF) :
        ErrorMsg("SrvAuthCfmMsg", error, origin, reason) {
        m_len = sizeof(*this);
        STRBUF_COPY(m_nodeId, nodeId);
    }
    char const *GetNodeId() const { return m_nodeId; }
    enum {
        NODE_ID_LEN = 16
    };
protected:
    char m_nodeId[NODE_ID_LEN];
} __attribute__((packed));

class SrvPingReqMsg final: public Msg {
public:
    SrvPingReqMsg() :
        Msg("SrvPingReqMsg") {
        m_len = sizeof(*this);
    }
} __attribute__((packed));

class SrvPingCfmMsg final: public ErrorMsg {
public:
    SrvPingCfmMsg(char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC) :
        ErrorMsg("SrvPingCfmMsg", error, origin, reason) {
        m_len = sizeof(*this);
    }
} __attribute__((packed));

} // namespace APP

#endif // SRV_MSG_INTERFACE_H
