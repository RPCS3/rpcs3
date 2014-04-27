/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, David Ryan (Oobles@hotmail.com)
  ------------------------------------------------------------------------
  iop_sysclib.a		   C Library Functions.
			   taken from .irx files with symbol table.
			   Additions from Herben's IRX Tool imports.txt
*/

	.text
	.set	noreorder


/* ############################### SYSCLIB STUB ####### */
/* # Added by Oobles, 5th March 2002                  # */

	.local	sysclib_stub
sysclib_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"sysclib\0"
	.align	2

	.globl	setjmp				# 0x04
setjmp:
	j	$31
	li	$0, 0x04

	.globl	longjmp				# 0x05
longjmp:
	j	$31
	li	$0, 0x05

	.globl	toupper				# 0x06
toupper:
	j	$31
	li	$0, 0x06

	.globl	tolower				# 0x07
tolower:
	j	$31
	li	$0, 0x07

	.globl	look_ctype_table		# 0x08
look_ctype_table:
	j	$31
	li	$0, 0x08

	.globl	get_ctype_table			# 0x09
get_ctype_table:				
	j	$31
	li	$0, 0x09

	.globl	memchr				# 0x0A
memchr:
	j	$31
	li	$0, 0x0A

	.globl	memcmp				# 0x0B
memcmp:
	j	$31
	li	$0, 0x0B

	.globl	memcpy				# 0x0C
memcpy:
	j	$31
	li	$0, 0x0C

	.globl	memmove				# 0x0D
memmove:
	j	$31
	li	$0, 0x0D

	.globl	memset				# 0x0E
memset:
	j	$31
	li	$0, 0x0E

	.globl	bcmp				# 0x0F
bcmp:
	j	$31
	li	$0, 0x0F

	.globl	bcopy				# 0x10
bcopy:
	j	$31
	li	$0, 0x10

	.globl	bzero				# 0x11
bzero:
	j	$31
	li	$0, 0x11

	.globl	prnt				# 0x12
prnt:
	j	$31
	li	$0, 0x12

	.globl	sprintf				# 0x013
sprintf:
	j	$31
	li	$0, 0x13

	.globl	strcat				# 0x14
strcat:
	j	$31
	li	$0, 0x14

	.globl	strchr				# 0x15
strchr:
	j	$31
	li	$0, 0x15

	.globl	strcmp				# 0x16
strcmp:
	j	$31
	li	$0, 0x16

	.globl	strcpy				# 0x17
strcpy:
	j	$31
	li	$0, 0x17

	.globl	strcspn				# 0x18
strcspn:
	j	$31
	li	$0, 0x18

	.globl	index				# 0x19
index:
	j	$31
	li	$0, 0x19

	.globl	rindex				# 0x1A
rindex:
	j	$31
	li	$0, 0x1A

	.globl	strlen				# 0x1b
strlen:
	j	$31
	li	$0, 0x1b

	.globl	strncat				# 0x1c
strncat:
	j	$31
	li	$0, 0x1C

	.globl	strncmp				# 0x1d
strncmp:
	j	$31
	li	$0, 0x1d

	.globl	strncpy				# 0x1E
strncpy:
	j	$31
	li	$0, 0x1E

	.globl	strpbrk				# 0x1F
strpbrk:
	j	$31
	li	$0, 0x1F

	.globl	strrchr				# 0x20
strrchr:
	j	$31
	li	$0, 0x20

	.globl	strspn				# 0x21
strspn:
	j	$31
	li	$0, 0x21

	.globl	strstr				# 0x22
strstr:
	j	$31
	li	$0, 0x22

	.globl	strtok				# 0x23
strtok:
	j	$31
	li	$0, 0x23

	.globl	strtol				# 0x24
strtol:
	j	$31
	li	$0, 0x24

	.globl	atob				# 0x25
atob:
	j	$31
	li	$0, 0x25

	.globl	strtoul				# 0x26
strtoul:
	j	$31
	li	$0, 0x26

	.globl	wmemcopy			# 0x28
wmemcopy:
	j	$31
	li	$0, 0x28

	.globl	wmemset				# 0x29
wmemset:
	j	$31
	li	$0, 0x29

	.globl	vsprintf			# 0x2A
vsprintf:
	j	$31
	li	$0, 0x2A

	.word	0
	.word	0


