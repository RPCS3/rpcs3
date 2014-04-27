
.set noreorder
.set noat

    .global EXCEP_Sys_handler
    .ent EXCEP_Sys_handler
EXCEP_Sys_handler:

    .word 0
    .word 0
    
    jal EXCEP_Sys_8
    nop                 # branch delay
    
    # restore status
    lw  $k0, EX_ST($0)
    mtc0 $k0, $12
    
    lw  $k0, EX_PC($0)
    add $k0, $k0, 4
    jr $k0
    cop0 0x10
    nop
    .end EXCEP_Sys_handler
    
        
.equiv STACK_FRAME_SIZE, 0x90
.equiv MODE0_ID, 0xAC0000FE
.equiv MODE1_ID, 0xFF00FFFE
.equiv MODE2_ID, 0xFFFFFFFE    

.equiv EX_AT, 0x400
.equiv EX_PC, 0x404
.equiv EX_ST, 0x408
.equiv EX_CAUSE, 0x40C

.equiv INTR_STACK_SIZE, 0x800

.equiv I_REG,  0x1070
.equiv I_MASK, 0x1074

# ------------------------------------------------------------
# This is installed as priority 3 - the end of the chain.
# does nothing.
#
    .global EXCEP_Int_priority_exception_handler
    .ent EXCEP_Int_priority_exception_handler
EXCEP_Int_priority_exception_handler:

    .word 0
    .word 0

    lw  $at, EX_AT($0)
    
    # restore status
    lw  $k0, EX_ST($0)
    mtc0 $k0, $12
    
    lw  $k0, EX_PC($0)
    jr $k0
    cop0 0x10
    nop
    .end EXCEP_Int_priority_exception_handler    

# ------------------------------------------------------------
    
    .global EXCEP_Int_handler
    .ent EXCEP_Int_handler
EXCEP_Int_handler:
    .word 0
    .word 0

    # Initialise the mode 0 stack frame
    sw  $sp, 0x74-STACK_FRAME_SIZE($sp)
    addiu $sp, $sp, -STACK_FRAME_SIZE
    
    lw  $at, EX_AT($0) 
    sw  $at, 0x04($sp)
    sw  $v0, 0x08($sp)
    sw  $v1, 0x0C($sp)
    sw  $a0, 0x10($sp)
    sw  $a1, 0x14($sp)
    sw  $a2, 0x18($sp)
    sw  $a3, 0x1C($sp)
    sw  $ra, 0x7C($sp) 
    mfhi $v0
    sw  $v0, 0x80($sp)
    mflo $v0
    sw  $v0, 0x84($sp)
    lw  $v0, EX_ST($0)
    sw  $v0, 0x88($sp)
    lw  $v0, EX_PC($0)
    sw  $v0, 0x8C($sp)
    la  $v0, MODE0_ID
    jal QueryIntrContext
    sw  $v0, 0x00($sp)              # branch delay slot
    
    # Check whether this is a nested interrupt
    # If it is not a nested interrupt then switch the stack to the interrupt context.
    beq $v0, $0, $L_not_nested
    or $v0, $sp, $sp               # branch delay slot
        # it is a nested interrupt
        j $L1
        sub $sp, $sp, 0x0C       # branch delay slot
        
$L_not_nested:
        # not a nested interrupt, so initialise to the top of the interrupt
        # context stack.
        la $sp, tempstack
		addiu $sp, $sp,  INTR_STACK_SIZE - 0x0C
$L1:

    sw $v0, 0x4($sp)               # link the location of the previous stack frame
    sw $0,  0x8($sp)               # interrupt number (bit mask)

    lw $v0, EX_CAUSE($0)
    andi $v0, $v0, 0x30
    bne $v0, $0, $L_SoftwareInt
    nop
        # The interrupt must have been caused by a hardware event
        lui $k0, 0xBF80
        lw  $a0, I_REG($k0)
        lw  $k0, I_MASK($k0)            # k0 = I_MASK
        and $a0, $a0, $k0       
        lw  $k1, soft_hw_intr_mask
        and $a0, $a0, $k1              # a0 = I_REG & I_MASK & soft_hw_intr_mask
        
        beq $a0, $0, $L_ChainNextHandler    # Interrupt is masked
        nop
        # The interrupt is not masked, so we should try and handle it
   
        # Figure out what interrupt is being handled. Look at the bits from the LSB and
        # find the first one that is set. The LSB is interrupt 0
        la $a1, 0xFFFFFFFF      # The interrupt being handled
$L_countInts:
            addi $a1, $a1, 1
            andi $a2, $a0, 0x01
            beq $a2, $0, $L_countInts
            srl $a0, 1
        
        addiu $a0, $0, 0x1
        sllv $a0, $a0, $a1  # a0 = the bitmask of the int being handled
        sw $a0, 0x8($sp)
        nor $a0, $a0, $0
        and $k0, $a0, $k0
        lui $k1, 0xBF80
        sw $k0, I_MASK($k1)
        sw $a0, I_REG($k1)
        
        sll $a0, $a1, 3     # Int num * 8
        lw $k0, intrtable
        add $k0, $a0, $k0
        lw $a2, 0($k0)      # handler
        bne $a2, $0, $L_HandleInterrupt
        lw $a3, 4($k0)      # arg

$L_ChainNextHandler:

        # Pass control to the next exception handler
        lw  $sp, 0x4($sp)     # pop the value of the previous stack frame (could be a context switch)
        
        lw  $at, 0x04($sp)
        lw  $v0, 0x08($sp)
        lw  $v1, 0x0C($sp)
        lw  $a0, 0x10($sp)
        lw  $a1, 0x14($sp)
        lw  $a2, 0x18($sp)
        lw  $a3, 0x1C($sp)
        lw  $ra, 0x7C($sp)
        addiu $sp, $sp, STACK_FRAME_SIZE
        lw  $k0, EXCEP_Int_handler
        jr  $k0
        nop
        
$L_SoftwareInt:
    # $v0 = CAUSE & 0x30 is non-zero so the interrupt must have been caused by a software event
    
    andi $v0, $v0, 0x10
    bne $v0, $0, $L2
    addiu $a0, $0, 0x0100       # sw Int 1
    addiu $a0, $0, 0x0200       # sw Int 2
$L2:
    mfc0 $a1, $13               # cause
    nor $a2, $a0, $0
    and $a1, $a1, $a2           # reset the relevent cause bit
    mtc0 $a1, $13
   
    srl $a0, $a0, 8
    addiu $a0, $a0, 0x2D        # interrupt vector is 0x2E or 0x2F
    sll $a0, $a0, 3             # Int num * 8
    lw $k0, intrtable
    addu $k0, $a0, $k0
    lw $a2, 0($k0)              # handler
    beq $a2, $0, $L_ChainNextHandler        # No handler defined
    lw $a3, 4($k0)              # arg    
    
    # Both paths for SW and HW interrupts come together here.
    #
    # At this point:
    #                 0x8($sp) = int mask (0x01 << interrupt number)
    #                 $a2 = handler
    #                 $a3 = arg
$L_HandleInterrupt:

    andi $a0, $a2, 0x03     # $a0 = mode
    srl $a2, $a2, 2
    beq $a0, $0, $L_Mode0
    sll $a2, $a2, 2         # $a2 = masked handler
    
    la $v1, MODE1_ID
    srl $a0, $a0, 1
    beq $a0, $0, $L_Mode1
    lw $v0, 4($sp)
    # Mode 2 or 3 (save t and s)
    sw $s0, 0x40($v0)
    sw $s1, 0x44($v0)
    sw $s2, 0x48($v0)
    sw $s3, 0x4C($v0)
    sw $s4, 0x50($v0)
    sw $s5, 0x54($v0)
    sw $s6, 0x58($v0)
    sw $s7, 0x5C($v0)
        
    la $v1, MODE2_ID
    
$L_Mode1:
    # Mode 1, save t
    sw $t0, 0x20($v0)
    sw $t1, 0x24($v0)
    sw $t2, 0x28($v0)
    sw $t3, 0x2C($v0)
    sw $t4, 0x30($v0)
    sw $t5, 0x34($v0)
    sw $t6, 0x38($v0)
    sw $t7, 0x3C($v0)
    sw $t8, 0x60($v0)
    sw $t9, 0x64($v0)
    sw $gp, 0x70($v0)
    sw $fp, 0x78($v0)
    
    sw $v1, 0x00($v0)       # mark the current state of the stack
    
$L_Mode0:
    or $a0, $a3, $a3
    jalr $a2                # call the interrupt handler
    mtc0 $0, $12            # status
    beq $v0, $0, $L_NotHandled
    lui $k0, 0xBF80
   
    lw $a0, I_MASK($k0)
    lw $a1, 0x08($sp)       # interrupt mask
    or $a0, $a0, $a1
    sw $a0, I_MASK($k0)
    
$L_NotHandled:

    jal QueryIntrContext
    nop
  
	bne $v0, $0, $L_afterContextSwitch
	lw $a0, 4($sp)      # previous stack frame 

	# test if a context switch is pending. Returns true ($v0 != 0) if there is.
	jal callContextSwitchRequiredHandler          
    nop

    beq $v0, $0, $L_afterContextSwitch
    lw $a0, 4($sp)      # previous stack frame
    
    lw $v0, 0($a0)      # current stack context state
    la $a1, MODE2_ID
    beq $a1, $v0, $DoContextSwitch
    nop
    la $a1, MODE1_ID
    beq $a1, $v0, $Mode1OnStack
    nop
    # stack state is currently mode 0
    sw $t0, 0x20($a0)
    sw $t1, 0x24($a0)
    sw $t2, 0x28($a0)
    sw $t3, 0x2C($a0)
    sw $t4, 0x30($a0)
    sw $t5, 0x34($a0)
    sw $t6, 0x38($a0)
    sw $t7, 0x3C($a0)
    sw $t8, 0x60($a0)
    sw $t9, 0x64($a0)
    sw $gp, 0x70($a0)
    sw $fp, 0x78($a0)    
$Mode1OnStack:    
    sw $s0, 0x40($a0)
    sw $s1, 0x44($a0)
    sw $s2, 0x48($a0)
    sw $s3, 0x4C($a0)
    sw $s4, 0x50($a0)
    sw $s5, 0x54($a0)
    sw $s6, 0x58($a0)
    sw $s7, 0x5C($a0)
        
    la $v1, MODE2_ID
    sw $v1, 0($a0)
    
$DoContextSwitch:
    la $v0, context_switch_handler
    lw $v0, 0($v0)
    jalr $v0           # do the context switch, returns the new stack in $v0
    nop
    addu $a0, $v0, $0

$L_afterContextSwitch:

    # At this point:
    #
    #  $a0 = stack frame to switch to

    or $sp, $a0, $a0
    lw $a0, 0($sp)
    la $a1, MODE0_ID

    beq $a0, $a1, $L_RestoreMode0
    nop
    
    la $a1, MODE1_ID
    beq $a0, $a1, $L_RestoreMode1
    nop
    # mode 2, restore t and s
    lw $s0, 0x40($sp)
    lw $s1, 0x44($sp)
    lw $s2, 0x48($sp)
    lw $s3, 0x4C($sp)
    lw $s4, 0x50($sp)
    lw $s5, 0x54($sp)
    lw $s6, 0x58($sp)
    lw $s7, 0x5C($sp)
    
$L_RestoreMode1:
    # mode 1, restore t
    lw $t0, 0x20($sp)
    lw $t1, 0x24($sp)
    lw $t2, 0x28($sp)
    lw $t3, 0x2C($sp)
    lw $t4, 0x30($sp)
    lw $t5, 0x34($sp)
    lw $t6, 0x38($sp)
    lw $t7, 0x3C($sp)
    lw $t8, 0x60($sp)
    lw $t9, 0x64($sp)
    lw $gp, 0x70($sp)
    lw $fp, 0x78($sp)
    
$L_RestoreMode0:    
    lw $a0, 0x80($sp)
    mthi $a0
    lw $a0, 0x84($sp)
    mtlo $a0
    lw $a0, 0x88($sp)
    srl $a0, $a0, 1
    sll $a0, $a0, 1
    mtc0 $a0, $12       # status (reset bit 0)
    lw  $at, 0x04($sp)
    lw  $v0, 0x08($sp)
    lw  $v1, 0x0C($sp)
    lw  $a0, 0x10($sp)
    lw  $a1, 0x14($sp)
    lw  $a2, 0x18($sp)
    lw  $a3, 0x1C($sp)
    lw  $ra, 0x7C($sp)
    lw  $k0, 0x8C($sp)
    lw  $sp, 0x74($sp)
    jr $k0
    cop0 0x10           # rfe
    
    .end EXCEP_Int_handler
