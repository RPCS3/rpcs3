/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, Florin Sasu (florinsasu@yahoo.com)
  ------------------------------------------------------------------------
  iop_thevent.s		   Event Function Imports.
*/

	.text
	.set	noreorder


	.local	thevent_stub
thevent_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000101
	.ascii	"thevent\0"
	.align	2

	.globl	CreateEventFlag			# 004
CreateEventFlag:
	j	$31
	li	$0, 0x04

        .globl  DeleteEventFlag
DeleteEventFlag:
        j       $31
        li      $0, 0x05

	.globl	SetEventFlag			# 006
SetEventFlag:
	j	$31
	li	$0, 0x06

	.globl	iSetEventFlag			# 007
iSetEventFlag:
	j	$31
	li	$0, 0x07

	.globl	ClearEventFlag			# 008
ClearEventFlag:
	j	$31
	li	$0, 0x08

	.globl	iClearEventFlag			# 009
iClearEventFlag:
	j	$31
	li	$0, 0x09

	.globl	WaitEventFlag			# 00A
WaitEventFlag:
	j	$31
	li	$0, 0x0A

	.word	0
	.word	0

	.globl	ReferEventFlagStatus		# 00D
ReferEventFlagStatus:
	j	$31
	li	$0, 0x0D

	.globl	iReferEventFlagStatus		# 00E
iReferEventFlagStatus:
	j	$31
	li	$0, 0x0E
