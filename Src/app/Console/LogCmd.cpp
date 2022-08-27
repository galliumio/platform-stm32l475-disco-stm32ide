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
#include "LogCmd.h"
#include "UartOutInterface.h"

FW_DEFINE_THIS_FILE("LogCmd.cpp")

namespace APP {

static CmdStatus Show(Console &console, Evt const *e) {
    uint32_t &hsmn = console.Var(0);
    uint32_t &count = console.Var(1);
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.Print("Verbosity = %d\n\r", Log::GetVerbosity());
            console.Print("Log enabled in:\n\r");
            console.Print("===============\n\r");
            hsmn = 0;
            count = 0;
            break;
        }
        case UART_OUT_EMPTY_IND: {
            for (; hsmn < HSM_COUNT; hsmn++) {
                if (Log::IsOn(hsmn)) {
                    bool result = console.PrintItem(count, 28, 4, "%s(%lu)", Log::GetHsmName(hsmn), hsmn);
                    if (!result) {
                        return CMD_CONTINUE;
                    }
                    count++;
                }
            }
            console.PutStr("\n\r\n\r");
            return CMD_DONE;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus On(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            Console::ConsoleCmd const &cmd = static_cast<Console::ConsoleCmd const &>(*e);
            if (cmd.Argc() > 1) {
                // We don't support enabling for all since it will enable UART and Console related HSMs causing a flood of log messages.
                // Uses type uint32_t instead of Hsmn for range checking.
                uint32_t num = STRING_TO_NUM(cmd.Argv(1), 0);
                if (num < HSM_COUNT) {
                    Log::On(num);
                    console.Print("Enabled log for %s\n\r", Log::GetHsmName(num));
                    break;
                }
                console.Print("Invalid hsmn. Must < %d\n\r", HSM_COUNT);
                break;
            }
            // Print usage.
            console.Print("log on <hmsn>\n\r");
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus Off(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            Console::ConsoleCmd const &cmd = static_cast<Console::ConsoleCmd const &>(*e);
            if (cmd.Argc() > 1) {
                if (STRING_EQUAL(cmd.Argv(1), "all")) {
                    Log::OffAll();
                    console.Print("Disabled log for all\n\r");
                    break;
                }
                // Uses type uint32_t instead of Hsmn for range checking.
                uint32_t num = STRING_TO_NUM(cmd.Argv(1), 0);
                if (num < HSM_COUNT) {
                    Log::Off(num);
                    console.Print("Disabled log for %s\n\r", Log::GetHsmName(num));
                    break;
                }
                console.Print("Invalid hsmn. Must < %d\n\r", HSM_COUNT);
                break;
            }
            // Print usage.
            console.Print("log off <hmsn>|all\n\r");
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus Ver(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            Console::ConsoleCmd const &cmd = static_cast<Console::ConsoleCmd const &>(*e);
            if (cmd.Argc() > 1) {
                // Uses type uint32_t instead of Hsmn for range checking.
                uint32_t num = STRING_TO_NUM(cmd.Argv(1), 0);
                if (num <= Log::MAX_VERBOSITY) {
                    Log::SetVerbosity(num);
                    console.Print("Verbosity set to %d\n\r", num);
                    break;
                }
                console.Print("Invalid verbosity. Must be 0-%d\n\r", Log::MAX_VERBOSITY);
                break;
            }
            // Print current verbosity and usage.
            console.Print("Current verbosity = %d\n\r", Log::GetVerbosity());
            console.Print("Usage:\n\r");
            console.Print("log ver <0-5>\n\r");
            console.Print("0 - Nothing\n\r");
            console.Print("1 - ERROR\n\r");
            console.Print("2 - ERROR, WARNING\n\r");
            console.Print("3 - ERROR, WARNING, CRITICAL\n\r");
            console.Print("4 - ERROR, WARNING, CRITICAL, LOG\n\r");
            console.Print("5 - ERROR, WARNING, CRITICAL, LOG, INFO\n\r");
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus List(Console &console, Evt const *e);
static CmdHandler const cmdHandler[] = {
    { "show",       Show,       "Show config", 0 },
    { "on",         On,         "Enable log", 0 },
    { "off",        Off,        "Disable log", 0 },
    { "ver",        Ver,        "Set verbosity", 0 },
    { "?",          List,       "List commands", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus LogCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
