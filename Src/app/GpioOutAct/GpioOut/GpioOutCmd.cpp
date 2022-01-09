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
#include "fw_log.h"
#include "fw_assert.h"
#include "Console.h"
#include "GpioOutInterface.h"
#include "GpioOutCmd.h"

FW_DEFINE_THIS_FILE("GpioOutCmd.cpp")

namespace APP {

static CmdStatus Test(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("GpioOutCmd Test\n\r");
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus On(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            Console::ConsoleCmd const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if (ind.Argc() >= 2) {
                uint32_t pattern = STRING_TO_NUM(ind.Argv(1), 0);
                bool repeat = true;
                if (ind.Argc() >= 3 && STRING_EQUAL(ind.Argv(2), "0")) {
                    repeat = false;
                }
                console.Print("pattern = %d, repeat = %d\n\r", pattern, repeat);
                console.Send(new GpioOutPatternReq(pattern, repeat), GPIO_OUT);
                break;
            }
            console.Print("led on <pattern idx> [0=once,*other=repeat]\n\r");
            return CMD_DONE;
        }
        case GPIO_OUT_PATTERN_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            console.PrintErrorEvt(cfm);
            return CMD_DONE;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus Off(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.Send(new GpioOutOffReq(), GPIO_OUT);
            break;
        }
        case GPIO_OUT_OFF_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            console.PrintErrorEvt(cfm);
            return CMD_DONE;
        }
    }
    return CMD_CONTINUE;
}


static CmdStatus List(Console &console, Evt const *e);
static CmdHandler const cmdHandler[] = {
    { "test",       Test,       "Test function", 0 },
    { "on",         On,         "Start a pattern", 0 },
    { "off",        Off,        "Stop a pattern", 0 },
    { "?",          List,       "List commands", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus GpioOutCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
