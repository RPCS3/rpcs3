// microVU.cpp assembly routines
// arcum42(@gmail.com)
.intel_syntax

.extern mVUexecuteVU0
.extern mVUexecuteVU1
.extern g_sseVUMXCSR
.extern g_sseMXCSR

//------------------------------------------------------------------
// Dispatcher Functions
//------------------------------------------------------------------

// Runs VU0 for number of cycles
// __fastcall = The first two DWORD or smaller arguments are passed in ECX and EDX registers; all other arguments are passed right to left.
//void __fastcall startVU0(u32 startPC, u32 cycles)
.globl startVU0
startVU0:
		call mVUexecuteVU0

		// backup cpu state
		push %ebx
		push %ebp
		push %esi
		push %edi

		ldmxcsr g_sseVUMXCSR
		// Should set xmmZ?
		jmp %eax

// Runs VU1 for number of cycles
// void __fastcall startVU1(u32 startPC, u32 cycles) 
.globl startVU1
startVU01:
		call mVUexecuteVU1

		// backup cpu state
		push %ebx
		push %ebp
		push %esi
		push %edi

		ldmxcsr g_sseVUMXCSR

		jmp %eax

// Exit point
// void __fastcall endVU0(u32 startPC, u32 cycles)
.globl endVU0
endVU0:
		//call mVUcleanUpVU0

		/*restore cpu state*/
		pop %edi;
		pop %esi;
		pop %ebp;
		pop %ebx;
		
		ldmxcsr g_sseMXCSR

		ret
		