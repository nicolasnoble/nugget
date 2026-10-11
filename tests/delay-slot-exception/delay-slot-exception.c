/*

MIT License

Copyright (c) 2026 PCSX-Redux authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "common/hardware/dma.h"
#include "common/hardware/hwregs.h"
#include "common/syscalls/syscalls.h"

#undef unix
#define CESTER_NO_SIGNAL
#define CESTER_NO_TIME
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#include "exotic/cester.h"

// clang-format off

#include "../cop0/cester-cop0.c"

CESTER_BODY(
    uint32_t delayslot_syscall();
    uint32_t delayslot_nested();
    uint32_t delayslot_swi();
    uint32_t delayslot_resume();

    static uint32_t getSR() {
        uint32_t r;
        __asm__ volatile("mfc0 %0, $12\nnop" : "=r"(r));
        return r;
    }

    static void setSR(uint32_t r) {
        __asm__ volatile("mtc0 %0, $12\nnop\nnop" : : "r"(r));
    }

    static uint32_t clearCause() {
        __asm__ volatile("mtc0 $0, $13\nnop");
        return (uint32_t)delayslot_resume;
    }

    static uint32_t runProbe(uint32_t (*probe)()) {
        s_got80 = 0;
        s_cause = 0;
        s_epc = 0;
        s_resume = (uint32_t *)delayslot_resume;
        uint32_t paths = probe();
        s_resume = NULL;
        return paths;
    }
)

// The exception is taken: the handler runs and the branch target never does.
CESTER_TEST(delay_slot_syscall, cpu_tests,
    uint32_t paths = runProbe(delayslot_syscall);
    uint32_t excode = (s_cause >> 2) & 0x1f;
    uint32_t bd = s_cause >> 31;
    uint32_t epc = (uint32_t)delayslot_syscall + 4;
    cester_assert_uint_eq(8, paths);
    cester_assert_uint_eq(1, s_got80);
    cester_assert_uint_eq(8, excode);
    cester_assert_uint_eq(1, bd);
    cester_assert_uint_eq(epc, s_epc);
)

// EPC and Cause.BD are not checked here: they have not been measured on
// hardware for this case.
CESTER_TEST(delay_slot_nested_syscall, cpu_tests,
    uint32_t paths = runProbe(delayslot_nested);
    uint32_t excode = (s_cause >> 2) & 0x1f;
    cester_assert_uint_eq(8, paths);
    cester_assert_uint_eq(1, s_got80);
    cester_assert_uint_eq(8, excode);
)

// EPC and Cause.BD are not checked here: they have not been measured on
// hardware for this case.
CESTER_TEST(delay_slot_software_interrupt, cpu_tests,
    uint32_t sr = getSR();
    s_customhandler = clearCause;
    setSR((sr & ~0xff00) | 0x101);
    uint32_t paths = runProbe(delayslot_swi);
    setSR(sr);
    s_customhandler = NULL;
    uint32_t excode = (s_cause >> 2) & 0x1f;
    cester_assert_uint_eq(8, paths);
    cester_assert_uint_eq(1, s_got80);
    cester_assert_uint_eq(0, excode);
)
