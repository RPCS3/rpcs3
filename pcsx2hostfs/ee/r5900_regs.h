//------------------------------------------------------------------------
// File:	regs.h
// Author:	Tony Saveski, t_saveski@yahoo.com
// Notes:	Playstation 2 Register Definitions
//------------------------------------------------------------------------
#ifndef R5900_REGS_H
#define R5900_REGS_H

// MIPS CPU Registsers
#define zero	$0		// Always 0
#define at		$1		// Assembler temporary
#define v0		$2		// Function return
#define v1		$3		//
#define a0		$4		// Function arguments
#define a1		$5
#define a2		$6
#define a3		$7
#define t0		$8		// Temporaries. No need
#define t1		$9		// to preserve in your
#define t2		$10		// functions.
#define t3		$11
#define t4		$12
#define t5		$13
#define t6		$14
#define t7		$15
#define s0		$16		// Saved Temporaries.
#define s1		$17		// Make sure to restore
#define s2		$18		// to original value
#define s3		$19		// if your function
#define s4		$20		// changes their value.
#define s5		$21
#define s6		$22
#define s7		$23
#define t8		$24		// More Temporaries.
#define t9		$25
#define k0		$26		// Reserved for Kernel
#define k1		$27
#define gp		$28		// Global Pointer
#define sp		$29		// Stack Pointer
#define fp		$30		// Frame Pointer
#define ra		$31		// Function Return Address

// COP0
#define Index    $0  // Index into the TLB array 
#define Random   $1  // Randomly generated index into the TLB array 
#define EntryLo0 $2  // Low-order portion of the TLB entry for..
#define EntryLo1 $3  // Low-order portion of the TLB entry for
#define Context  $4  // Pointer to page table entry in memory
#define PageMask $5  // Control for variable page size in TLB entries 
#define Wired    $6  // Controls the number of fixed ("wired") TLB entries
#define BadVAddr $8  // Address for the most recent address-related exception
#define Count    $9  // Processor cycle count
#define EntryHi  $10 // High-order portion of the TLB entry
#define Compare  $11 // Timer interrupt control
#define Status   $12 // Processor status and control
#define Cause    $13 // Cause of last general exception
#define EPC      $14 // Program counter at last exception
#define PRId     $15 // Processor identification and revision
#define Config   $16 // Configuration register
#define LLAddr   $17 // Load linked address
#define WatchLo  $18 // Watchpoint address Section 6.25 on
#define WatchHi  $19 // Watchpoint control
#define Debug    $23 // EJTAG Debug register
#define DEPC     $24 // Program counter at last EJTAG debug exception
#define PerfCnt  $25 // Performance counter interface
#define ErrCtl   $26 // Parity/ECC error control and status
#define CacheErr $27 // Cache parity error control and status
#define TagLo    $28 // Low-order portion of cache tag interface
#define TagHi    $29 // High-order portion of cache tag interface
#define ErrorPC  $30 // Program counter at last error
#define DEASVE   $31 // EJTAG debug exception save register

#endif // R5900_REGS_H
