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

    .set push
    .set noreorder
    .set noat
    .section .text.delayslot, "ax", @progbits
    .align 2

# Each probe returns $v0 with bit 1 set if the code after the faulting
# instruction ran. The exception handler resumes at delayslot_resume,
# which sets bit 3.

# SYSCALL in the delay slot of a taken BEQ.
    .global delayslot_syscall
    .type delayslot_syscall, @function
delayslot_syscall:
    move  $v0, $0
    beq   $0, $0, 1f
    syscall
    nop
1:  ori   $v0, $v0, 2
    jr    $ra
    nop
    .size delayslot_syscall, . - delayslot_syscall

# A not-taken BNE in the delay slot of a taken BEQ. The BNE's own delay
# slot is the BEQ's target, which holds the SYSCALL.
    .global delayslot_nested
    .type delayslot_nested, @function
delayslot_nested:
    move  $v0, $0
    beq   $0, $0, 1f
    bne   $0, $0, 2f
    nop
    nop
1:  syscall
    ori   $v0, $v0, 2
2:  jr    $ra
    nop
    .size delayslot_nested, . - delayslot_nested

# A Cause write raising software interrupt 0, in the delay slot of a
# taken BEQ.
    .global delayslot_swi
    .type delayslot_swi, @function
delayslot_swi:
    move  $v0, $0
    li    $t0, 0x100
    beq   $0, $0, 1f
    mtc0  $t0, $13
    nop
1:  ori   $v0, $v0, 2
    jr    $ra
    nop
    .size delayslot_swi, . - delayslot_swi

    .global delayslot_resume
    .type delayslot_resume, @function
delayslot_resume:
    jr    $ra
    ori   $v0, $v0, 8
    .size delayslot_resume, . - delayslot_resume

    .set pop
