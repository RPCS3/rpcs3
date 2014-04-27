/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002,  David Ryan (Oobles@hotmail.com)
  ------------------------------------------------------------------------
  iop_thbase.s		   Base Kernel Functions.
			   taken from .irx files with symbol table
*/

	.text
	.set	noreorder


/* ############################### THBASE STUB ######## */
/* # Added by Oobles, 5th March 2002                  # */

	.local	thbase_stub
thbase_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"thbase\0\0"
	.align	2

	.globl	CreateThread			# 004
CreateThread:
	j	$31
	li	$0, 4

	.globl	DeleteThread			# 005
DeleteThread:
	j	$31
	li	$0, 5

	.globl	StartThread			# 006
StartThread:
	j	$31
	li	$0, 6

	.globl	StartThreadArgs			# 007
StartThreadArgs:
	j	$31
	li	$0, 0x07

	.globl	ExitThread			# 008
ExitThread:
	j	$31
	li	$0, 0x08

	.globl	ExitDeleteThread		# 009
ExitDeleteThread:
	j	$31
	li	$0, 0x09

	.globl	TerminateThread			# 010
TerminateThread:
	j	$31
	li	$0, 0x0A

	.globl	iTerminateThread		# 011
iTerminateThread:
	j	$31
	li	$0, 0x0B

	.globl	DisableDispatchThread		# 012
DisableDispatchThread:
	j	$31
	li	$0, 0x0C

	.globl	EnableDispatchThread		# 013
EnableDispatchThread:
	j	$31
	li	$0, 0x0D


	.globl	ChangeThreadPriority		# 014
ChangeThreadPriority:
	j	$31
	li	$0, 0x0E

	.globl	iChangeThreadPriority		# 015
iChangeThreadPriority:
	j	$31
	li	$0, 0x0F


	.globl	ReleaseWaitThread		# 018
ReleaseWaitThread:
	j	$31
	li	$0, 18

	.globl	iReleaseWaitThread		# 019
iReleaseWaitThread:
	j	$31
	li	$0, 19

	.globl	GetThreadId			# 0x14
GetThreadId:
	j	$31
	li	$0, 0x14

	.globl	SleepThread			# 0x18
SleepThread:
	j	$31
	li	$0, 0x18

	.globl	WakeupThread			# 0x19
WakeupThread:
	j	$31
	li	$0, 0x19

	.globl	iWakeupThread			# 0x1A
iWakeupThread:
	j	$31
	li	$0, 0x1A

	.globl	DelayThread			# 0x21
DelayThread:
	j	$31
	li	$0, 0x21

	.globl	GetSystemTime			# 0x22
GetSystemTime:
	j	$31
	li	$0, 0x22


	.globl	SetAlarm			# 0x23
SetAlarm:
	j	$31
	li	$0, 0x23

	.globl	iSetAlarm			# 0x24
iSetAlarm:
	j	$31
	li	$0, 0x24

	.globl	CancelAlarm			# 0x25
CancelAlarm:
	j	$31
	li	$0, 0x25

	.globl	iCancelAlarm			# 0x26
iCancelAlarm:
	j	$31
	li	$0, 0x26

	.globl	USec2SysClock			# 0x27
USec2SysClock:
	j	$31
	li	$0, 0x27

	.globl	SysClock2USec			# 0x28
SysClock2USec:
	j	$31
	li	$0, 0x28

	.globl	GetSystemStatusFlag		# 0x29
GetSystemStatusFlag:
	j	$31
	li	$0, 0x29


	.word	0
	.word	0


