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
#include "app_hsmn.h"
#include "Console.h"
#include "SystemCmd.h"
#include "SystemInterface.h"
#include "LevelMeterInterface.h"
#include "DispInterface.h"

FW_DEFINE_THIS_FILE("SystemCmd.cpp")

// Uncomment the following to enable tensorflow demo.
//#define ENABLE_TENSOR
#ifdef ENABLE_TENSOR
#include "tensorflow/lite/micro/examples/hello_world/main_functions.h"
#endif

namespace APP {

static CmdStatus Test(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("SystemCmd Test\n\r");
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus Start(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("Starting SYSTEM\n\r");
            console.Send(new SystemStartReq(SYSTEM, console.GetHsmn(), 0));
            break;
        }
        case SYSTEM_START_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            console.PrintErrorEvt(cfm);
        	return CMD_DONE;
        }
    }
    return CMD_CONTINUE;
}


static CmdStatus Stop(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("Stopping SYSTEM\n\r");
            console.Send(new SystemStopReq(SYSTEM, console.GetHsmn(), 0));
            break;
        }
        case SYSTEM_STOP_CFM: {
            ErrorEvt const &cfm = ERROR_EVT_CAST(*e);
            console.PrintErrorEvt(cfm);
        	return CMD_DONE;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus Cpu(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            Console::ConsoleCmd const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if (ind.Argc() < 2) {
                console.Print("on - enable, off - disable\n\r");
                break;
            }
            bool enable = STRING_EQUAL(ind.Argv(1), "on");
            console.Send(new SystemCpuUtilReq(enable), SYSTEM);
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus Tensor(Console &console, Evt const *e) {
#ifdef ENABLE_TENSOR
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            setup();
            console.Send(new LevelMeterStopReq(), LEVEL_METER);
            break;
        }
        case LEVEL_METER_STOP_CFM: {
            console.GetTimer().Start(100, Timer::PERIODIC);
            console.Send(new DispStartReq(), ILI9341);
            console.Send(new DispDrawBeginReq(), ILI9341);
            console.Send(new DispDrawRectReq(0, 0, 240, 320, COLOR24_BLUE), ILI9341);
            break;
        }
        case Console::CONSOLE_TIMER: {
            loop();
            break;
        }
    }
    return CMD_CONTINUE;
#else
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.PutStr("Tensorflow demo currently disabled.\n\r");
            break;
        }
    }
    return CMD_DONE;
#endif
}

static CmdStatus List(Console &console, Evt const *e);
static CmdHandler const cmdHandler[] = {
    { "test",       Test,       "Test function", 0 },
    { "?",          List,       "List commands", 0 },
    { "stop",       Stop,       "Stop HSM", 0 },
    { "start",      Start,      "Start HSM", 0 },
    { "cpu",        Cpu,        "Report CPU util", 0 },
    { "tensor",     Tensor,     "Tensorflow Lite demo", 0},
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus SystemCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

}
