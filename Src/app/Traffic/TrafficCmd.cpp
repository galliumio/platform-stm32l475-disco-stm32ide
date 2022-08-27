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

#include <string.h>
#include "fw_log.h"
#include "fw_assert.h"
#include "Console.h"
#include "TrafficCmd.h"
#include "TrafficInterface.h"

FW_DEFINE_THIS_FILE("TrafficCmd.cpp")

namespace APP {

static CmdStatus NorthSouth(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("Car arriving in NS direction\n\r");
            console.Send(new TrafficCarNSReq(), TRAFFIC);
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus EastWest(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("Car arriving in EW direction\n\r");
            console.Send(new TrafficCarEWReq(), TRAFFIC);
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus ErrorReq(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("Critical error occurred\n\r");
            console.Send(new TrafficErrorReq(), TRAFFIC);
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus List(Console &console, Evt const *e);
static CmdHandler const cmdHandler[] = {
    { "n",       NorthSouth,       "North", 0 },
    { "s",       NorthSouth,       "South", 0 },
    { "e",       EastWest,         "East", 0 },
    { "w",       EastWest,         "West", 0 },
    { "r",       ErrorReq,         "Error", 0 },
    { "?",       List,             "List commands", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus TrafficCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
