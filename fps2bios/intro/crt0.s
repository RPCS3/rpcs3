.set noat
.set noreorder
.global _start

.global FlushCache
.global Exit
.global SignalSema
.global _start
.global	_exit

.text
	nop
	nop
FlushCache:
	li	$3,100
	syscall
	jr	$31
	nop
Exit:
	li	$3,4
	syscall
	jr	$31
	nop
SignalSema:
	li	$3,66
	syscall
	jr	$31
	nop

	nop
	nop
_start:
   	lui	$2,%hi(_args_ptr)
   	addiu	$2,$2, %lo(_args_ptr)
    	sw	$4,($2)
# Clear bss
zerobss:
	lui	$2,%hi(_fbss)
	lui	$3,%hi(_end)
	addiu	$2,$2,%lo(_fbss)
	addiu	$3,$3,%lo(_end)
loop:
	nop
	nop
	sq	$0,($2)
	sltu	$1,$2,$3
	bne	$1,$0,loop
	addiu	$2,$2,16

# Thread
	lui	$4,%hi(_gp)
	addiu	$4,$4,%lo(_gp)
	lui	$5,%hi(_stack)
	addiu	$5,$5,%lo(_stack)
	lui	$6,%hi(_stack_size)
	addiu	$6,$6,%lo(_stack_size)
	lui	$7,%hi(_args)
	addiu	$7,$7,%lo(_args)
	lui	$8,%hi(_root)
	addiu	$8,$8,%lo(_root)
	move	$28,$4
	addiu	$3,$0,60
	syscall
	move	$29, $2

# Heap
	addiu	$3,$0,61
	lui	$4,%hi(_end)
	addiu	$4,$4,%lo(_end)
	lui	$5,%hi(_heap_size)
	addiu	$5,$5,%lo(_heap_size)
	syscall
	nop

# Cache
	jal	FlushCache
	move	$4,$0

# Jump main
	ei

 	lui	$2,%hi(_args_ptr)
 	addiu	$2,$2,%lo(_args_ptr)
 	lw	$3,($2)
	lui	$2,%hi(_args)
	addiu	$2,$2,%lo(_args)

	lw	$4, ($2)
	jal	main
	addiu	$5, $2, 4
_root:
_exit:
# ???
#	lui	$2,%hi(_args_ptr)
#	addiu	$2,$2,%lo(_args_ptr)
#	lw	$3,($2)
#	jal	SignalSema
#	lw	$4,($3)
# Exit
#	addiu	$3,$0,35
#	syscall
	jr  $31
	nop

	.bss
	.align	6
_args: .space	256+16*4+4

	.data
_args_ptr:
	.space 4
