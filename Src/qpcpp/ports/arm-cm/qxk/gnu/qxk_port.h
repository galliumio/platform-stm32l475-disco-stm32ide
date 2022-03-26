/// @file
/// @brief QXK/C++ port to ARM Cortex-M, GNU-ARM toolset
/// @cond
///***************************************************************************
/// Last updated for version 6.0.3
/// Last updated on  2017-12-09
///
///                    Q u a n t u m     L e a P s
///                    ---------------------------
///                    innovating embedded systems
///
/// Copyright (C) Quantum Leaps, LLC. All rights reserved.
///
/// This program is open source software: you can redistribute it and/or
/// modify it under the terms of the GNU General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// Alternatively, this program may be distributed and modified under the
/// terms of Quantum Leaps commercial licenses, which expressly supersede
/// the GNU General Public License and are specifically designed for
/// licensees interested in retaining the proprietary status of their code.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program. If not, see <http://www.gnu.org/licenses/>.
///
/// Contact information:
/// https://state-machine.com
/// mailto:info@state-machine.com
///***************************************************************************
/// @endcond

#ifndef qxk_port_h
#define qxk_port_h

//****************************************************************************
//! activate the context-switch callback
// Gallium - Enables hook QXK_onContextSw() in bsp.cpp to be called for NewLib
//           _impure_ptr switching.
#define QXK_ON_CONTEXT_SW 1

// determination if the code executes in the ISR context
#define QXK_ISR_CONTEXT_() (QXK_get_IPSR() != static_cast<uint32_t>(0))

__attribute__((always_inline))
static inline uint32_t QXK_get_IPSR(void) {
    uint32_t regIPSR;
    __asm volatile ("mrs %0,ipsr" : "=r" (regIPSR));
    return regIPSR;
}

// trigger the PendSV exception to pefrom the context switch
#define QXK_CONTEXT_SWITCH_() \
    (*Q_UINT2PTR_CAST(uint32_t, 0xE000ED04U) = \
        static_cast<uint32_t>(1U << 28))

// QXK ISR entry and exit
#define QXK_ISR_ENTRY() ((void)0)

// Gallium - Fixed in version 6.9.1. Added call to QXK_ARM_ERRATUM_838869().
#define QXK_ISR_EXIT()  do { \
    QF_INT_DISABLE(); \
    if (QXK_sched_() != static_cast<uint_fast8_t>(0)) { \
        *Q_UINT2PTR_CAST(uint32_t, 0xE000ED04U) = \
            static_cast<uint32_t>(1U << 28); \
    } \
    QF_INT_ENABLE(); \
    QXK_ARM_ERRATUM_838869();  \
} while (false)

// Gallium - Fixed in version 6.9.1.
#if (__ARM_ARCH == 6) // Cortex-M0/M0+/M1 (v6-M, v6S-M)?
    #define QXK_ARM_ERRATUM_838869() ((void)0)
#else // Cortex-M3/M4/M7 (v7-M)
    // The following macro implements the recommended workaround for the
    // ARM Erratum 838869. Specifically, for Cortex-M3/M4/M7 the DSB
    // (memory barrier) instruction needs to be added before exiting an ISR.
    //
    #define QXK_ARM_ERRATUM_838869() \
        __asm volatile ("dsb" ::: "memory")
#endif

// initialization of the QXK kernel
#define QXK_INIT() QXK_init()
extern "C" void QXK_init(void);

#include "qxk.h" // QXK platform-independent public interface

#endif // qxk_port_h
