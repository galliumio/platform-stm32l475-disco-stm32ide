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

#ifndef TESTCODE_H
#define TESTCODE_H

#include <stdio.h>
#include <vector>
#include <algorithm>
#include <memory>
#include "qpcpp.h"
#include "periph.h"
#include "fw_macro.h"
#include "Console.h"
#include "fw_assert.h"

#define TEST_CODE_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("TestCode.h", (int_t)__LINE__))

namespace APP {

// For C++ demo only.
class Vehicle {
public:
    enum {
        VIN_SIZE = 18,
    };
    Vehicle(char const *vin, uint32_t seats, uint32_t mpg) :
        m_seats(seats), m_mpg(mpg) {
        STRBUF_COPY(m_vin, vin);
    }
    virtual ~Vehicle() {}
    char const *GetVin() const { return m_vin; }
    uint32_t GetSeats() const { return m_seats; }
    uint32_t GetMpg() const { return m_mpg; }
    virtual void GetInfo(char *buf, uint32_t bufSize) const {
        snprintf(buf, bufSize, "To be implemented in derived classes");
    }
    float RunningCost(float miles, float gasPrice) const {
        TEST_CODE_ASSERT(m_mpg);
        return (miles/m_mpg)*gasPrice;
    }
protected:
    char m_vin[VIN_SIZE];
    uint32_t m_seats;
    uint32_t m_mpg;
};

class Sedan : public Vehicle {
public:
    Sedan(char const *vin, uint32_t seats, uint32_t mpg, bool sporty) :
        Vehicle(vin, seats, mpg), m_sporty(sporty) {}
    bool IsSporty() const { return m_sporty; }
protected:
    bool m_sporty;
};

class Suv : public Vehicle {
public:
    Suv(char const *vin, uint32_t seats, uint32_t mpg, bool roofRack) :
        Vehicle(vin, seats, mpg), m_roofRack(roofRack) {}
    bool hasRoofRack() const { return m_roofRack; }
protected:
    bool m_roofRack;
};

class ModelA : public Sedan {
public:
    ModelA(char const *vin) :
        Sedan(vin, 5, 35, false) {}
    void GetInfo(char *buf, uint32_t bufSize) const {
        snprintf(buf, bufSize, "%s: ModelA sedan, seats=%lu, mpg=%lu, sporty=%d", m_vin, m_seats, m_mpg, m_sporty);
    }
};

class ModelB : public Suv {
public:
    enum {
        UPGRADE_SIZE = 128
    };
    ModelB(char const *vin, char const *upgrade = "") :
        Suv(vin, 7, 24, true) {
        STRBUF_COPY(m_upgrade, upgrade);
    }
    void GetInfo(char *buf, uint32_t bufSize) const {
        snprintf(buf, bufSize, "%s: ModelB SUV, seats=%lu, mpg=%lu, roofRack=%d, upgrade='%s'",
                m_vin, m_seats, m_mpg, m_roofRack, m_upgrade);
    }
private:
    char m_upgrade[UPGRADE_SIZE];
};

class CarInventory {
public:
    enum {
        INFO_SIZE = 256,
    };
    CarInventory() = default;
    ~CarInventory() = default;
    void Add(Vehicle *v) {
        m_vehicles.push_back(v);
    }
    void Remove(char const *vin) {
        m_vehicles.erase(std::remove_if(m_vehicles.begin(), m_vehicles.end(), [&](Vehicle const *v) {
            return STRING_EQUAL(v->GetVin(), vin);
        }), m_vehicles.end());
    }
    void Clear() {
        m_vehicles.clear();
    }
    void Show(Console &console, char const *msg = nullptr) {
        if (msg) {
            console.Print("%s\n\r", msg);
        }
        console.Print("Car inventory:\n\r");
        console.Print("==============\n\r");
        for (auto const &v: m_vehicles) {
            char info[INFO_SIZE];
            v->GetInfo(info, sizeof(info));
            console.Print("%s\n\r", info);
        }
    }
private:
    std::vector<Vehicle *> m_vehicles;
};

class CarInventorySp {
public:
    enum {
        INFO_SIZE = 256,
    };
    CarInventorySp() = default;
    ~CarInventorySp() = default;
    void Add(std::shared_ptr<Vehicle> v) {
        m_vehicles.push_back(v);
    }
    void Remove(char const *vin) {
        m_vehicles.erase(std::remove_if(m_vehicles.begin(), m_vehicles.end(), [&](std::shared_ptr<const Vehicle> v) {
            return STRING_EQUAL(v->GetVin(), vin);
        }), m_vehicles.end());
    }
    void Clear() {
        m_vehicles.clear();
    }
    void Show(Console &console, char const *msg = nullptr) {
        if (msg) {
            console.Print("%s\n\r", msg);
        }
        console.Print("Car inventory:\n\r");
        console.Print("==============\n\r");
        for (auto const &v: m_vehicles) {
            char info[INFO_SIZE];
            v->GetInfo(info, sizeof(info));
            console.Print("%s\n\r", info);
        }
    }
private:
    std::vector<std::shared_ptr<Vehicle>> m_vehicles;
};

};

#endif // TESTCODE_H
