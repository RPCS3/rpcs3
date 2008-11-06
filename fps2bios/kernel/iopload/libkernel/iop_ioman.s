/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, Gustavo Scott(gustavo@scotti.com)
  ------------------------------------------------------------------------
  iop_iooman.s
			IOP Basic libraries. 
			took from .irx files with symbol table / string table
*/

	.text
	.set	noreorder


/* ############################### IOMAN STUB ######## */
	.local	ioman_stub
ioman_stub:
	.word	0x41e00000
	.word	0
	.word	0x00000102
	.ascii	"ioman\0\0\0"
	.align	2

	.globl	open				# 004
open:
	j	$31
	li	$0, 4

	.globl	close				# 005
close:
	j	$31
	li	$0, 5

	.globl	read				# 006
read:
	j	$31
	li	$0, 6

	.globl	write				# 007
write:
	j	$31
	li	$0, 7

	.globl	lseek				# 008
lseek:
	j	$31
	li	$0, 8

	.globl	ioctl
ioctl:
	j	$31
	li	$0, 9

	.globl	remove
remove:
	j	$31
	li	$0, 10

	.globl	mkdir
mkdir:
	j	$31
	li	$0, 11

	.globl	rmdir
rmdir:
	j	$31
	li	$0, 12

	.globl	dopen
dopen:
	j	$31
	li	$0, 13

	.globl	dclose
dclose:
	j	$31
	li	$0, 14

	.globl	dread
dread:
	j	$31
	li	$0, 15

	.globl	getstat
getstat:
	j	$31
	li	$0, 16

	.globl	chstat
chstat:
	j	$31
	li	$0, 17

	.globl	format
format:
	j	$31
	li	$0, 18

	.globl	FILEIO_add			# 020
	.globl	AddDrv
FILEIO_add:
AddDrv:
	j	$31
	li	$0, 20

	.globl	FILEIO_del			# 021
	.globl	DelDrv
FILEIO_del:
DelDrv:
	j	$31
	li	$0, 21

	.word	0
	.word	0


