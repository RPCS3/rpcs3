.set noat
.set noreorder

.global _start
.global	_exit

.text
        nop
        nop
_start:
        lui     $2,%hi(_args_ptr)
        addiu	$2,$2, %lo(_args_ptr)
    	sw      $4,($2)
# Clear bss
zerobss:
        lui     $2,%hi(_fbss)
        lui     $3,%hi(_end)
        addiu	$2,$2,%lo(_fbss)
        addiu	$3,$3,%lo(_end)
loop:
        nop
        nop
        sq      $0,($2)
        sltu	$1,$2,$3
        bne     $1,$0,loop
        addiu	$2,$2,16
# Thread
        lui     $4,%hi(_gp)
        lui     $5,%hi(_stack)
        lui		$6,%hi(_stack_size)
        lui		$7,%hi(_args)
        lui		$8,%hi(_exit)
        addiu   $4,$4,%lo(_gp)
        addiu	$5,$5,%lo(_stack)
        addiu	$6,$6,%lo(_stack_size)
        addiu	$7,$7,%lo(_args)
        addiu	$8,$8,%lo(_exit)
        move	$28,$4
        addiu	$3,$0,60
        syscall
        move	$29, $2

# Heap
        addiu	$3,$0,61
        lui     $4,%hi(_end)
        addiu	$4,$4,%lo(_end)
        lui     $5,%hi(_heap_size)
        addiu	$5,$5,%lo(_heap_size)
        syscall
        nop

# Cache
        li	$3, 100
        move	$4,$0
        syscall
        nop

# Jump main
        ei

        # Check if we got args from system
        lui     $2, %hi(_args)
        addiu   $2, %lo(_args)
        lw      $3, 0($2)
        bnez    $3,  _gotArgv   # Started w (Load)ExecPS2
        nop

        # Check args via $a0
        lui     $2,%hi(_args_ptr)
        addiu	$2,$2,%lo(_args_ptr)
        lw      $3,0($2)

        beqzl   $3, _goMain
        addu    $4, $0, $0
        
        addiu   $2, $3, 4
        b       _gotArgv
        nop

_gotArgv:
        lw      $4, ($2)
        addiu	$5, $2, 4
_goMain:
        jal     main
        nop

        # Check if we got our args in $a0 again
        la      $4, _args_ptr
        lw      $5, 0($4)
        beqz    $5,  _exit
        nop
_root:
        lw      $6, 0($5)
        sw      $0, 0($6)
        # Call ExitDeleteThread()
        addiu   $3, $0, 36
        syscall
        nop
_exit:
# Exit
        # Should call Exit(retval) if not called by pukklink
        addiu	$3,$0,35
        syscall
        move    $4, $2
        nop

        .bss
        .align	6
_args:  .space	256+16*4+4

        .data
_args_ptr:
        .space 4
