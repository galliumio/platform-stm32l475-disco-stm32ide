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
#include "fw_macro.h"
#include "fw_timer.h"
#include "Console.h"
#include "ConsoleInterface.h"
#include "UartOutInterface.h"
#include "ConsoleCmd.h"
#include "LogCmd.h"
#include "SystemCmd.h"
//#include "WifiStCmd.h"
#include "DemoCmd.h"
#include "GpioOutCmd.h"
#include "AOWashingMachineCmd.h"
#include "TrafficCmd.h"
#include "SimpleActCmd.h"
#include "CompositeActCmd.h"
#include "TestCode.h"
#include <memory>

FW_DEFINE_THIS_FILE("ConsoleCmd.cpp")

namespace APP {

using std::make_shared;
using std::shared_ptr;
using std::static_pointer_cast;

static CmdStatus Test(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            Console::ConsoleCmd const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            console.PutStr("ConsoleCmd Test\n\r");
            console.Print("Command = %s\n\r", ind.Argv(0));
            if (ind.Argc() > 1) {
                console.Print("%d arguments:\n\r", ind.Argc() - 1);
                for (uint32_t i = 1; i < ind.Argc(); i++) {
                    console.Print("[%d] %s\n\r", i, ind.Argv(i));
                }
            }
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus Assert(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            Console::ConsoleCmd const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if ((ind.Argc() > 1) && STRING_EQUAL(ind.Argv(1), "1234")) {
                FW_ASSERT(0);
            } else {
                console.PutStr("Invalid passcode\n\r");
            }
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus Hsm(Console &console, Evt const *e) {
    uint32_t &hsmn = console.Var(0);
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.Print("HSMs in system:\n\r");
            console.Print("===============\n\r");
            hsmn = 0;
            break;
        }
        case UART_OUT_EMPTY_IND: {
            for (; hsmn < HSM_COUNT; hsmn++) {
                bool result = console.PrintItem(hsmn, 28, 4, "%s(%lu)", Log::GetHsmName(hsmn), hsmn);
                if (!result) {
                    return CMD_CONTINUE;
                }
            }
            console.PutStr("\n\r\n\r");
            return CMD_DONE;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus State(Console &console, Evt const *e) {
    uint32_t &hsmn = console.Var(0);
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            console.Print("HSM states:\n\r");
            console.Print("===========\n\r");
            hsmn = 0;
            break;
        }
        case UART_OUT_EMPTY_IND: {
            for (; hsmn < HSM_COUNT; hsmn++) {
                bool result = console.PrintItem(hsmn, 56, 2, "%s(%lu) - %s", Log::GetHsmName(hsmn), hsmn, Log::GetState(hsmn));
                if (!result) {
                    return CMD_CONTINUE;
                }
            }
            console.PutStr("\n\r\n\r");
            return CMD_DONE;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus Timer(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            Console::ConsoleCmd const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if (ind.Argc() < 2) {
                console.PutStr("timer period_ms - timer test\n\r");
                return CMD_DONE;
            }
            uint32_t period = STRING_TO_NUM(ind.Argv(1), 0);
            console.Print("period = %d\n\r", period);
            console.GetTimer().Start(period, Timer::PERIODIC);
            console.Var(0) = 0;
            break;
        }
        case Console::CONSOLE_TIMER: {
            console.Print("timeout %d\n\r", console.Var(0)++);
            break;
        }
    }
    return CMD_CONTINUE;
}

CmdStatus Fibonacci(Console &console, Evt const *e) {
    uint32_t &prev2 = console.Var(0);
    uint32_t &prev1 = console.Var(1);
    uint32_t &count = console.Var(2);
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            prev2 = 1;
            prev1 = 1;
            count = 0;
            console.Print("[%d] %d\n\r", count++, prev2);
            console.Print("[%d] %d\n\r", count++, prev1);
            break;
        }
        case UART_OUT_EMPTY_IND: {
            while (1) {
                uint32_t curr = prev2 + prev1;
                prev2 = prev1;
                prev1 = curr;
                uint32_t result = console.Print("[%d] %lu\n\r", count, curr);
                if (result == 0) {
                    break;
                }
                if (++count > 45) {
                    prev2 = 1;
                    prev1 = 1;
                    count = 2;
                }
            }
            break;
        }
    }
    return CMD_CONTINUE;
}

static CmdStatus Perf(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            uint32_t startMs = GetSystemMs();
            const uint32_t TEST_CNT = 100000;
            const uint16_t TEST_SIZE = 32;
            for (uint32_t i = 0; i < TEST_CNT; i++) {
                QEvt *evt = QF::newX_(TEST_SIZE, 0, 0);
                evt->sig = i;
                QF::gc(evt);
            }
            console.Print("Elapsed time with QF = %d\n\r", GetSystemMs() - startMs);

            startMs = GetSystemMs();
            for (uint32_t i = 0; i < TEST_CNT; i++) {
                auto evt = std::make_shared<QEvt>(0);
                evt->sig = i;
            }
            console.Print("Elapsed time with new = %d\n\r", GetSystemMs() - startMs);
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus Cpp(Console &console, Evt const *e) {
    switch (e->sig) {
        case Console::CONSOLE_CMD: {
            auto const &ind = static_cast<Console::ConsoleCmd const &>(*e);
            if (ind.Argc() < 2) {
                console.Print("Enter test case number (>=0)\n\r");
                break;
            }
            uint32_t testNum = STRING_TO_NUM(ind.Argv(1), 0);
            switch(testNum) {
                case 0: {
                    ModelA carA1("A001");
                    ModelA carA2("A002");
                    ModelA carA3("A003");
                    ModelB carB1("B001");
                    ModelB carB2("B002", "Autodrive");
                    ModelB carB3("B003", "Entertainment");
                    CarInventory inventory;
                    inventory.Add(&carA1);
                    inventory.Add(&carA2);
                    inventory.Add(&carA3);
                    inventory.Add(&carB1);
                    inventory.Add(&carB2);
                    inventory.Add(&carB3);
                    inventory.Show(console);
                    inventory.Remove("A002");
                    inventory.Remove("B002");
                    inventory.Remove("B004");
                    inventory.Show(console, "After deleting some cars...");
                    inventory.Clear();
                    inventory.Show(console, "After clearing all...");
                    break;
                }
                case 1: {
                    auto carA1 = make_shared<ModelA>("A001");
                    auto carB1 = make_shared<ModelB>("B002", "Navigation");
                    CarInventorySp inventory;
                    inventory.Add(static_pointer_cast<Vehicle>(carA1));
                    inventory.Add(static_pointer_cast<Vehicle>(carB1));
                    inventory.Show(console);
                    inventory.Remove("A001");
                    carA1.reset();
                    inventory.Show(console, "After removing car A001...");
                    // reset sp before removing, still OK.
                    carB1.reset();
                    inventory.Show(console, "After resetting car B001 shared_ptr...");
                    inventory.Clear();
                    inventory.Show(console, "After clearning all...");
                    break;
                }
                case 2: {
                    Vehicle car1("CAR1", 5, 35);
                    Vehicle car2("CAR2", 7, 24);
                    ModelA carA("A001");
                    ModelB carB("B001");
                    console.Print("CAR1:   A 100-mile trip costs $%f\n\r", car1.RunningCost(100, 2.89));
                    console.Print("CAR2:   A 100-mile trip costs $%f\n\r", car2.RunningCost(100, 2.89));
                    console.Print("ModelA: A 100-mile trip costs $%f\n\r", carA.RunningCost(100, 2.89));
                    console.Print("ModelB: A 100-mile trip costs $%f\n\r", carB.RunningCost(100, 2.89));
                    break;
                }
                case 3: {
                    Vehicle car1("VEHICLE", 5, 35);
                    ModelA car2("MODEL_A");
                    char info[100];
                    car1.GetInfo(info, sizeof(info));
                    console.Print("Vehicle::GetInfo() is called: %s\n\r", info);
                    car2.GetInfo(info, sizeof(info));
                    console.Print("ModelA::GetInfo() is called: %s\n\r", info);
                    break;
                }
                case 4: {
                    ModelA car("MODEL_A");
                    Vehicle *pv = &car;  // OK
                    console.Print("VIN = %s\n\r", pv->GetVin());
                    // Compile error:
                    // invalid conversion from 'APP::Vehicle*' to 'APP::ModelA*' [-fpermissive]
                    //ModelA *pa = pv;
                    ModelA *pa = static_cast<ModelA *>(pv);
                    console.Print("VIN = %s\n\r", pa->GetVin());
                    break;
                }
            }
            break;
        }
    }
    return CMD_DONE;
}

static CmdStatus List(Console &console, Evt const *e);
static CmdHandler const cmdHandler[] = {
    { "test",       Test,       "Test function", 0 },
    { "assert",     Assert,     "Trigger assert", 0 },
    { "hsm",        Hsm,        "List all HSMs", 0 },
    { "state",      State,      "List HSM states", 0 },
    { "log",        LogCmd,     "Log control", 0 },
    { "timer",      Timer,      "Timer test function", 0 },
    { "fib",        Fibonacci,  "Fibonacci generator", 0 },
    { "sys",        SystemCmd,  "System", 0 },
    //{ "wifi",       WifiStCmd,  "Wifi(stm32) control", 0 },
    { "demo",       DemoCmd,    "Demo from Psicc", 0 },
    { "gpio",       GpioOutCmd, "GPIO output control", 0 },
    { "wash",       AOWashingMachineCmd, "Washing machine", 0 },
    { "traffic",    TrafficCmd, "Traffic light", 0 },
    { "perf",       Perf,       "Performance demo", 0 },
    { "cpp",        Cpp,        "C++ testing", 0 },
    { "simp",       SimpleActCmd,    "Template/SimpleAct testing", 0 },
    { "comp",       CompositeActCmd, "Template/CompositeAct testing", 0 },
    { "?",          List,       "List commands", 0 },
};

static CmdStatus List(Console &console, Evt const *e) {
    return console.ListCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler));
}

CmdStatus ConsoleCmd(Console &console, Evt const *e) {
    return console.HandleCmd(e, cmdHandler, ARRAY_COUNT(cmdHandler), true);
}

}
