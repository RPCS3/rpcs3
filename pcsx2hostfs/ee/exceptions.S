/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

# ASM exception handlers

#include "r5900_regs.h"
        
.set noat
.set noreorder


.text
.p2align 4

        .global _savedRegs
        
        .global pkoStepBP
        .ent    pkoStepBP
pkoStepBP:
        # Should check for cause in cop0 Cause reg
        # If Bp, increase EPC (DO NOT PUT 'break' in a branch delay slot!!!)
        mfc0    k0, EPC         # cop0 EPC
        addiu   k0, k0, 4     # Step over breakpoint
        mtc0    k0, EPC
        sync.p
        eret
        .end pkoStepBP

     
        # Save all user regs
        # Save HI/LO, SR, BadVAddr, Cause, EPC, ErrorEPC, 
        # ShiftAmount, cop0: $24, $25
        # Save float regs??
        # Set EPC to debugger
        # Set stack to 'exception stack'
        # eret
        .global pkoExceptionHandler
        .ent	pkoExceptionHandler
pkoExceptionHandler:
        la      k0, _savedRegs
        sq      $0,  0x00(k0)
        sq      at,  0x10(k0)
        sq      v0,  0x20(k0)
        sq      v1,  0x30(k0)
        sq      a0,  0x40(k0)
        sq      a1,  0x50(k0)
        sq      a2,  0x60(k0)
        sq      a3,  0x70(k0)
        sq      t0,  0x80(k0)
        sq      t1,  0x90(k0)
        sq      t2,  0xa0(k0)
        sq      t3,  0xb0(k0)
        sq      t4,  0xc0(k0)
        sq      t5,  0xd0(k0)
        sq      t6,  0xe0(k0)
        sq      t7,  0xf0(k0)
        sq      t8,  0x100(k0)
        sq      t9,  0x110(k0)
        sq      s0,  0x120(k0)
        sq      s1,  0x130(k0)
        sq      s2,  0x140(k0)
        sq      s3,  0x150(k0)
        sq      s4,  0x160(k0)
        sq      s5,  0x170(k0)
        sq      s6,  0x180(k0)
        sq      s7,  0x190(k0)
#        sq      k0,  0x1a0(k0)          # $k0
        sq      zero, 0x1a0(k0)         # zero instead
        sq      k1,  0x1b0(k0)          # $k1
        sq      gp,  0x1c0(k0)
        sq      sp,  0x1d0(k0)          # sp
        sq      fp,  0x1e0(k0)
        sq      ra,  0x1f0(k0)          # $ra

        pmfhi   t0                      # HI
        pmflo   t1                      # LO
        sq      t0, 0x200(k0)
        sq      t1, 0x210(k0)
       
        mfc0    t0, BadVAddr            # Cop0 state regs
        mfc0    t1, Status
        sw      t0, 0x220(k0)
        sw      t1, 0x224(k0)

        mfc0    t0, Cause
        mfc0    t1, EPC
        sw      t0, 0x228(k0)
        sw      t1, 0x22c(k0)

        # Kernel saves these two also..
        mfc0    t0, DEPC
        mfc0    t1, PerfCnt
        sw      t0, 0x230(k0)
        sw      t1, 0x234(k0)

        mfsa    t0
        sw      t0, 0x238(k0)

        # Use our own stack..
        la      sp, _exceptionStack+0x2000-16
        la       gp, _gp                 # Use exception handlers _gp
        
        # Return from exception and start 'debugger'
        mfc0    a0, Cause               # arg0
        mfc0    a1, BadVAddr
        mfc0    a2, Status
        mfc0    a3, EPC
        addu    t0, zero, k0            # arg4 = registers
        move    t1, sp
        la      k0, pkoDebug
        mtc0    k0, EPC                 # eret return address
        sync.p
        mfc0    k0, Status                  # check this out..
        li      v0, 0xfffffffe
        and     k0, v0
        mtc0    k0, Status
        sync.p
        nop
        nop
        nop
        nop
        eret
        nop
        .end	pkoExceptionHandler


        
        # Put EE in kernel mode
        # Restore all user regs etc
        # Restore PC? & Stack ptr
        # Restore interrupt sources
        # Jump to EPC
        .ent pkoReturnFromDebug
        .global pkoReturnFromDebug
pkoReturnFromDebug:

        lui     t1, 0x1
_disable:
        di
        sync
        mfc0    t0, Status
        and     t0, t1
        beqz    t0, _disable
        nop
        
        la      k0, _savedRegs
                
        lq      t0, 0x200(k0)
        lq      t1, 0x210(k0)
        pmthi   t0                      # HI
        pmtlo   t1                      # LO
       
        lw      t0, 0x220(k0)
        lw      t1, 0x224(k0)
        mtc0    t0, BadVAddr
        mtc0    t1, Status

        lw      t0, 0x228(k0)
        lw      t1, 0x22c(k0)
        mtc0    t0, Cause
        mtc0    t1, EPC

        # Kernel saves these two also..
        lw      t0, 0x230(k0)
        lw      t1, 0x234(k0)
        mtc0    t0, DEPC
        mtc0    t1, PerfCnt

        # Shift Amount reg
        lw      t0, 0x238(k0)
        mtsa    t0


#        ori     t2, 0xff
#        sw      t2, 0(k1)
        
        #        lq      $0,  0x00(k0)
        lq      $1,  0x10(k0)
        lq      $2,  0x20(k0)
        lq      $3,  0x30(k0)
        lq      $4,  0x40(k0)
        lq      $5,  0x50(k0)
        lq      $6,  0x60(k0)
        lq      $7,  0x70(k0)
        lq      $8,  0x80(k0)
        lq      $9,  0x90(k0)
        lq      $10, 0xa0(k0)
        lq      $11, 0xb0(k0)
        lq      $12, 0xc0(k0)
        lq      $13, 0xd0(k0)
        lq      $14, 0xe0(k0)
        lq      $15, 0xf0(k0)
        lq      $16, 0x100(k0)
        lq      $17, 0x110(k0)
        lq      $18, 0x120(k0)
        lq      $19, 0x130(k0)
        lq      $10, 0x140(k0)
        lq      $21, 0x150(k0)
        lq      $22, 0x160(k0)
        lq      $23, 0x170(k0)
        lq      $24, 0x180(k0)
        lq      $25, 0x190(k0)
#        lq      $26, 0x1a0(k0)          # $k0
        lq      $27, 0x1b0(k0)          # $k1
        lq      $28, 0x1c0(k0)
        lq      $29, 0x1d0(k0)          # $sp
        lq      $30, 0x1e0(k0)
#        lq      $31, 0x1f0(k0)          # $ra

        lw      ra, 0x22c(k0)
        # Guess one should have some check here, and only advance PC if 
        # we are going to step over a Breakpoint or something
        # (i.e. do stuff depending on Cause)
        addiu   ra, 4
        sync.p
        ei
        sync.p

        jr      ra
        nop        
        .end pkoReturnFromDebug
