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
#include "AOWashingMachineCmd.h"
#include "AOWashingMachineInterface.h"

FW_DEFINE_THIS_FILE("AOWashingMachineCmd.cpp")

namespace APP {

static CmdStatus Test(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("AOWashingMachineCmd Test\n\r");
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus PostEvt(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            Console::ConsoleCmd const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            FW_ASSERT(ind.Argc() >= 1);
            char const *str = ind.Argv(0);
            char ch = str[0];
            Evt *evt = NULL;
            switch(ch) {
                case 'o': {
                    console.PutStr("  -- Open door selected --\r\n");
                    evt = new UserSimOpenDoorInd();
                    break;
                }
                case 'c': {
                    console.PutStr("  -- Close door selected --\r\n");
                    evt = new UserSimCloseDoorInd();
                    break;
                }
                case 's': {
                    console.PutStr(" -- Start/Pause button pressed --\r\n");
                    evt = new UserSimStartPauseInd();
                    break;
                }
                case 'n': {
                    console.PutStr(" -- NORMAL cycle selected --\r\n");
                    evt = new UserSimCycleSelectedInd(UserSimCycleSelectedInd::NORMAL);
                    break;
                }
                case 'd': {
                    console.PutStr(" -- DELICATE cycle selected --\r\n");
                    evt = new UserSimCycleSelectedInd(UserSimCycleSelectedInd::DELICATE);
                    break;
                }
                case 'b': {
                    console.PutStr(" -- BULKY cycle selected --\r\n");
                    evt = new UserSimCycleSelectedInd(UserSimCycleSelectedInd::BULKY);
                    break;
                }
                case 't': {
                    console.PutStr(" -- TOWELS cycle selected --\r\n");
                    evt = new UserSimCycleSelectedInd(UserSimCycleSelectedInd::TOWELS);
                    break;
                }
                default: {
                    FW_ASSERT(false);
                }
            }
            console.Send(evt, AO_WASHING_MACHINE);
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus List(Console &console, Evt const *e);
static CmdHandler const cmdHandler[] = {
    { "test",    Test,       "Test function", 0 },
    { "o",       PostEvt,    "Opens the door", 0 },
    { "c",       PostEvt,    "Closes the door", 0 },
    { "s",       PostEvt,    "Start/Pause button", 0 },
    { "n",       PostEvt,    "Select wash NORMAL", 0 },
    { "d",       PostEvt,    "Select wash DELICATE", 0 },
    { "b",       PostEvt,    "Select wash BULKY", 0 },
    { "t",       PostEvt,    "Select wash TOWELS", 0 },
    { "?",       List,       "List commands", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus AOWashingMachineCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
