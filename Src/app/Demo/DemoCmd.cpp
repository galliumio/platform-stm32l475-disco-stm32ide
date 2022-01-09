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
#include "DemoCmd.h"
#include "DemoInterface.h"

FW_DEFINE_THIS_FILE("DemoCmd.cpp")

namespace APP {

static CmdStatus Test(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("DemoCmd Test\n\r");
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
            FW_ASSERT(str);
            char ch = str[0];
            QSignal sig = 0;    // Invalid
            switch(ch) {
                case 'a': sig = DEMO_A_REQ; break;
                case 'b': sig = DEMO_B_REQ; break;
                case 'c': sig = DEMO_C_REQ; break;
                case 'd': sig = DEMO_D_REQ; break;
                case 'e': sig = DEMO_E_REQ; break;
                case 'f': sig = DEMO_F_REQ; break;
                case 'g': sig = DEMO_G_REQ; break;
                case 'h': sig = DEMO_H_REQ; break;
                case 'i': sig = DEMO_I_REQ; break;
            }
            if (sig == 0) {
                console.PutStr("Enter a char from a to i\n\r");
                break;
            }
            console.Send(new Evt(sig), DEMO);
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus List(Console &console, Evt const *e);
static CmdHandler const cmdHandler[] = {
    { "test",    Test,       "Test function", 0 },
    { "a",       PostEvt,    "A evt", 0 },
    { "b",       PostEvt,    "B evt", 0 },
    { "c",       PostEvt,    "C evt", 0 },
    { "d",       PostEvt,    "D evt", 0 },
    { "e",       PostEvt,    "E evt", 0 },
    { "f",       PostEvt,    "F evt", 0 },
    { "g",       PostEvt,    "G evt", 0 },
    { "h",       PostEvt,    "H evt", 0 },
    { "i",       PostEvt,    "I evt", 0 },
    { "?",       List,       "List commands", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus DemoCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
