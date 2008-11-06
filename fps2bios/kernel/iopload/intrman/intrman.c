
#define _GNU_SOURCE

#include "kloadcore.h"
#include "kexcepman.h"
#include "kintrman.h"
#include "err.h"

#define ICFG       (*(volatile int*)0xbf801450)
#define IREG       (*(volatile int*)0xbf801070)
#define IMASK      (*(volatile int*)0xbf801074)
#define ICTRL      (*(volatile int*)0xbf801078)
#define DMA_ICR    (*(volatile int*)0xbf8010F4)
#define DMA_ICR2   (*(volatile int*)0xbf801574)

#define EXCEP_CAUSE (*(volatile int*)0x40C)

int debug=1;

#define _dprintf(fmt, args...) \
	if (debug > 0) __printf("intrman: " fmt, ## args)

struct intrHandler {
	u32  handler;
	void *arg;
};

func context_switch_required_handler;
func context_switch_handler;

// Additional interrupt mask applied after IMASK
u32 soft_hw_intr_mask;

int unknown2[4];  //0x1280 up

#define INTR_STACK_SIZE 0x800
unsigned char tempstack[INTR_STACK_SIZE];

u32 *extable;
struct intrHandler *intrtable;

extern struct export export_stub;

int  _start();
int IntrHandler();
void intrman_retonly();

/////////////////////////////syscalls//////////////////////////////////
__inline void syscall_0();
void syscall_1_CpuDisableIntr();
void syscall_2_CpuEnableIntr();
void syscall_3_intrman_call14();
void syscall_4_intrman_call17_19();
void syscall_5_intrman_call18_20();
void goto_EXCEP_Sys_handler();
void syscall_8_threadman();

func syscall[] = {
    (func)syscall_0,
/*    (func)syscall_1_CpuDisableIntr,
    (func)syscall_2_CpuEnableIntr,
    (func)syscall_3_intrman_call14,
    (func)syscall_4_intrman_call17_19,
    (func)syscall_5_intrman_call18_20,
    (func)goto_EXCEP_Sys_handler,
    (func)goto_EXCEP_Sys_handler,
    (func)syscall_8_threadman,
    (func)goto_EXCEP_Sys_handler,
    (func)goto_EXCEP_Sys_handler,
    (func)goto_EXCEP_Sys_handler,
    (func)goto_EXCEP_Sys_handler,
    (func)goto_EXCEP_Sys_handler,
    (func)goto_EXCEP_Sys_handler,
    (func)goto_EXCEP_Sys_handler*/
};

///////////////////////////////////////////////////////////////////////
int intrmanDeinit() {
	return 0;
}

///////////////////////////////////////////////////////////////////////
int intrman_call2()
{
	IMASK    = 0x00;
	DMA_ICR  = 0x00;
	DMA_ICR2 = 0x00;
	return 0;
}

///////////////////////////////////////////////////////////////////////
int intrman_call3(){
	return (int)&unknown2;
}

///////////////////////////////////////////////////////////////////////
//
// interrupt values 0x2E - 0x3F are equivalent to 0x1E - 0x2F with a mode set
// interrupt values 0x20 - 0x2D have their mode forced to zero
//
// mode specifies what registers to save around the call
//    0 = don't save anything, handler will not change anything
//    1 = save t0 - t9, gp and fp
//    2 = save t0 - t9, gp, fp and s0 - s7
//
// arg is the value passed to the interrupt handler when it is called
//
int RegisterIntrHandler(int interrupt, int mode, intrh_func handler, void *arg)
{
	u32 ictrl;

	_dprintf("%s interrupt=%x, mode=%x\n", __FUNCTION__, interrupt, mode);
	if (QueryIntrContext()){
		// cannot be called from within an interrupt
		_dprintf("%s ERROR_INTR_CONTEXT\n", __FUNCTION__);
		return ERROR_INTR_CONTEXT;
	 }
	CpuSuspendIntr(&ictrl);

	if (interrupt < 0 || interrupt > 0x3F) {
		CpuResumeIntr(ictrl);
		_dprintf("%s ERROR_ILLEGAL_INTRCODE\n", __FUNCTION__);
		return ERROR_ILLEGAL_INTRCODE;
	}
	int real_interrupt = interrupt;
	int real_mode = mode;
	
	if (interrupt >= 0x2E){
		real_interrupt -= 0x10;	
	} else if (interrupt >= 0x20){
		real_mode = 0;	
	}
	
	if (intrtable[real_interrupt].handler){
		CpuResumeIntr(ictrl);
		_dprintf("%s ERROR_DOES_EXIST\n", __FUNCTION__);
		return ERROR_DOES_EXIST;
	}
	intrtable[real_interrupt].handler = (mode & 0x03) | (u32)handler;
	intrtable[real_interrupt].arg     = arg;
	CpuResumeIntr(ictrl);
	return 0;
}

///////////////////////////////////////////////////////////////////////
int ReleaseIntrHandler(int interrupt)
{
	u32 ictrl;

	if (QueryIntrContext()){
		// Cannot be called from within an interrupt
		return ERROR_INTR_CONTEXT;
	 }
	CpuSuspendIntr(&ictrl);

	if (interrupt < 0 || interrupt >= 0x40) {
		CpuResumeIntr(ictrl);
		return ERROR_ILLEGAL_INTRCODE;
	}
	if (interrupt >= 0x2E){
		interrupt -= 0x10;	
	}
	
	if (intrtable[interrupt].handler){
		intrtable[interrupt].handler = 0;
		CpuResumeIntr(ictrl);
		return 0;		
	}
	CpuResumeIntr(ictrl);
	return ERROR_DOES_EXIST;
}

///////////////////////////////////////////////////////////////////////
int CpuSuspendIntr(u32 *ictrl)
{
    u32 rval = ICTRL;
	if (ictrl) *ictrl = rval;
	if (rval == 0) return ERROR_CPUDI;
	return 0;
}

///////////////////////////////////////////////////////////////////////
int CpuResumeIntr(u32 ictrl)
{
	ICTRL = ictrl;
	return 0;
}

///////////////////////////////////////////////////////////////////////
int CpuDisableIntr()
{
	if (ICTRL) return 0;
	return ERROR_CPUDI;
}

///////////////////////////////////////////////////////////////////////
int CpuEnableIntr()
{
	//_dprintf("%s\n", __FUNCTION__);
//	intrman_syscall_08();
//	_dprintf("syscall ok\n");
	CpuEnableICTRL();
	return 0;
}

///////////////////////////////////////////////////////////////////////
int CpuGetICTRL()
{
	return ICTRL;
}

///////////////////////////////////////////////////////////////////////
int CpuEnableICTRL()
{
	ICTRL = 1;
	return 1;
}

///////////////////////////////////////////////////////////////////////
void intrman_syscall_04()
{
    __asm__ (
        "li	 $3, 4\n"
        "syscall\n"
    );
}

///////////////////////////////////////////////////////////////////////
void intrman_syscall_08()
{
    __asm__ (
        "li	 $3, 8\n"
        "syscall\n"
    );
}

///////////////////////////////////////////////////////////////////////
void intrman_syscall_10()
{
    __asm__ (
        "li	 $3, 16\n"
        "syscall\n"
    );
}

///////////////////////////////////////////////////////////////////////
void intrman_syscall_14()
{
    __asm__ (
        "li	 $3, 20\n"
        "syscall\n"
    );
}

///////////////////////////////////////////////////////////////////////
void intrman_syscall_0C()
{
    __asm__ (
        "li	 $3, 12\n"
        "syscall\n"
    );
}

///////////////////////////////////////////////////////////////////////
//
// Interrupt:
//       0 - 0x1F  normal interrupt
//    0x20 - 0x26  
//    0x27 - 0x2D
//
// Return 0 for success, -101 for invalid interrupt number
//
#define ERROR_BAD_NUMBER -101
#define INUM_DMA_0 0x20
#define INUM_DMA_BERR 0x27
#define INUM_DMA_7 0x28
#define IMODE_DMA_IRM 0x100
#define IMODE_DMA_IQE 0x200
#define INUM_DMA 3

int EnableIntr(int interrupt)
{
    int retval = 0;
    u32 low_irq  = interrupt & 0xFF;
    u32 high_irq = interrupt & 0xFFFFFF00;
    
    u32 ictrl;
    CpuSuspendIntr(&ictrl);

    if( interrupt < 0 ) {
        retval = ERROR_BAD_NUMBER;
    }
    else if (low_irq < INUM_DMA_0){
        IMASK |= (1 << low_irq);
    } else if (low_irq < INUM_DMA_BERR){
        DMA_ICR = (DMA_ICR & ((~(1<<(low_irq-INUM_DMA_0)))&0x00FFFFFF))
            | (1 << (INUM_DMA_BERR-INUM_DMA_0 + 16))
            | (1<<(low_irq-INUM_DMA_0+0x10))
            | ((high_irq & IMODE_DMA_IRM) ? (1<<(low_irq-INUM_DMA_0)) : 0);
            
        DMA_ICR2 = (DMA_ICR2 & ((~(1<<(low_irq-INUM_DMA_0)))&0x00FFFFFF))
            | ((high_irq & IMODE_DMA_IQE) ? (1<<(low_irq-0x10)) : 0);
        
        IMASK |= 1<<INUM_DMA;
    } else if ( (low_irq-INUM_DMA_7)<6){ // low_irq = 0x27 isn't handled?

        u32 extra = 0;
        if (high_irq & IMODE_DMA_IQE)
            extra = (1<<(low_irq-INUM_DMA_7 + (INUM_DMA_BERR-INUM_DMA_0)));

        DMA_ICR2=(DMA_ICR2&(~(1<<(low_irq-INUM_DMA_7 + (INUM_DMA_BERR-INUM_DMA_0))) & 0x00FFFFFF)) | (1<<(low_irq-INUM_DMA_7+0x10)) | extra;
        DMA_ICR |= (DMA_ICR&0x00FFFFFF)| (1 << (INUM_DMA_BERR-INUM_DMA_0 + 16));
        
        IMASK |= 1<<INUM_DMA;
    } else {
        retval = ERROR_BAD_NUMBER;
    }
    
    CpuResumeIntr(ictrl);
    return retval;
}

///////////////////////////////////////////////////////////////////////
//not checked
int DisableIntr(int interrupt, int *oldstat){
 u32 ictrl;
 u32 old_IMASK;
 u32 low_irq;
 u32 stat;
 u32 ret;

 low_irq = interrupt & 0xFF;

 CpuSuspendIntr(&ictrl);

 ret = -101;
 stat = -103;

 if (low_irq<0) goto end;

 ret = 0;

 if (low_irq<0x20)
 {
	old_IMASK=IMASK;
	IMASK&=~(1<<low_irq);

	if (old_IMASK&(1<<low_irq))
	{
		stat = low_irq;
		goto end;
	}

	ret = -103;
	goto end;
 }

 if (low_irq<0x27)
 {

	if (DMA_ICR & 0xFFFFFF & (1<<(low_irq-0x10)))
	{
		if (((DMA_ICR & 0xFFFFFF)>>(low_irq-0x20)) & 0x01) stat = low_irq | 0x100;

		if (DMA_ICR2 & (1<<(low_irq-0x20))) stat |= 0x200;

		DMA_ICR2 &= ~(1<<(low_irq-0x20)) & 0xFFFFFF;

		goto end;
	}

	ret = -103;
	goto end;
 }

 if (low_irq<0x2E)
 {
	if (DMA_ICR2 & 0xFFFFFF & (1<<(low_irq-0x18)))
	{
		stat = low_irq;

		if (((DMA_ICR2 & 0xFFFFFF)>>(low_irq-0x21))&0x01) stat |= 0x200;

		DMA_ICR2 &= ~(1<<(low_irq-0x18)) & 0xFFFFFF;

		goto end;
	}

	ret = -103;
	goto end;
 }

end:
 if (oldstat) *(volatile int*)oldstat = stat;
 CpuResumeIntr(ictrl);
 return ret;
}

///////////////////////////////////////////////////////////////////////
// Enable
void intrman_call16(int interrupt)
{
    u32 ictrl;
    CpuSuspendIntr(&ictrl);
    
    interrupt &= 0xFF;
    
    if (interrupt < 0x20){
        soft_hw_intr_mask |= (1 << interrupt);
    } else if (interrupt < 0x28){
        unknown2[1] |= 1 << (interrupt-0x08);
    } else if (interrupt < 0x2E){
        unknown2[2] |= 1 << (interrupt-0x10);
    }
    
    CpuResumeIntr(ictrl);
}

///////////////////////////////////////////////////////////////////////
// Disable
void intrman_call15(int interrupt)
{
    u32 ictrl;
    CpuSuspendIntr(&ictrl);
    
    interrupt &= 0xFF;
    
    if (interrupt < 0x20){
        soft_hw_intr_mask &= ~(1 << interrupt);
    } else if (interrupt < 0x28){
        unknown2[1] &= ~(1 << (interrupt-0x08));
    } else if (interrupt < 0x2E){
        unknown2[2] &= ~(1 << (interrupt-0x10));
    }
    
    CpuResumeIntr(ictrl);
}

///////////////////////////////////////////////////////////////////////
void intrman_call27(int arg0)
{
	unknown2[3]=arg0;
}

///////////////////////////////////////////////////////////////////////
//
int IntrHandler()
{
	u32 masked_icr = DMA_ICR & unknown2[1];

	_dprintf("%s\n", __FUNCTION__);


	while (masked_icr & 0x7F008000){
		int i;		
		if (masked_icr & 0x00008000){
			// Int 0x25
			func int_func = (func)(intrtable[0x25].handler & 0xFFFFFFFC);
			DMA_ICR &= 0x00FF7FFF;
			if (int_func){
				int_func(intrtable[0x25].arg);
			}
		}
		// Check each DMA interrupt
		// The bits 24 - 30 are set for DMA channels 0 - 6 (ints 0x20 - 0x26 respectively)
        // The bits 16 - 23 are the corresponding mask bits - the interrupt is only generated if the mask bit is set. 
		for (i=0; i < 7; ++i){
			u32 ibit = 0x01000000 << i;
			if (masked_icr & ibit){
				func int_func = (func)(intrtable[0x20+i].handler & 0xFFFFFFFC);
				DMA_ICR &= (ibit | 0x00FFFFFF);
				if (int_func){
					if (!int_func(intrtable[0x20+i].arg)){
						// Disable / mask the interrupt if it was not handled
						DMA_ICR &= (~(0x00010000 << i) & 0x00FFFFFF);
					}
				}
			}
		}
	}
	DMA_ICR &= 0x007FFFFF;
	while (DMA_ICR & 0x00800000){
		// do nothing
	}

	{
		u32 temp_icr = DMA_ICR;
		temp_icr &= 0x007FFFFF;
		temp_icr |= 0x00800000;
		DMA_ICR = temp_icr;
	}

	return 1;
}

///////////////////////////////////////////////////////////////////////
//
// Default context switch handler - does not switch context.
//
int default_ctx_switch_handler(u32 sp_in)
{
	return sp_in;
}

///////////////////////////////////////////////////////////////////////
//
// Default routine to request a context switch. Does not request one.
//
int default_ctx_switch_required_handler()
{
	return 0;
}


void SetCtxSwitchHandler(func handler){
	context_switch_handler = handler;
}

void* ResetCtxSwitchHandler()
{
	context_switch_handler = default_ctx_switch_handler;
	return &context_switch_handler;
}

void SetCtxSwitchReqHandler(func handler)
{
	context_switch_required_handler = handler;
}

int ResetCtxSwitchReqHandler()
{
	context_switch_required_handler = default_ctx_switch_required_handler;
}

///////////////////////////////////////////////////////////////////////
// Return non-zero if sp is in an interrupt context
int QueryIntrStack(unsigned char* sp)
{
	int retval = 0;
	if ( (sp < tempstack + INTR_STACK_SIZE) && (sp > tempstack) ){
		retval = 1;
	}
	return retval;
}

///////////////////////////////////////////////////////////////////////
// Return non-zero is we're currently in a interrupt context
int QueryIntrContext()
{
	unsigned char* sp;
	__asm__ ("move   %0, $29\n" : "=r"(sp) :);
 
	return QueryIntrStack(sp);
}

///////////////////////////////////////////////////////////////////////
//
int iCatchMultiIntr()
{
	unsigned char* sp;
	__asm__ ("move   %0, $29\n" : "=r"(sp) :);

	if (QueryIntrStack(sp)){
		if (sp >= tempstack + 0x160){
			u32 SR;
			__asm__ ("mfc0 %0, $12\n" : "=r"(SR));
			if (SR & 1 == 0){
				u32 set_SR = SR | 1;
			    __asm__ volatile (
				     "mtc0 %0, $12\n"
			         "nop\n"
			         "nop\n"
				     "mtc0 %1, $12\n" : : "r"(set_SR), "r" (SR)
			    );
			}
			return SR;
		}
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////
// called by EXCEP_Sys_handler
void EXCEP_Sys_8() {
	_dprintf("%s\n", __FUNCTION__);
}
///////////////////////////////////////////////////////////////////////
//not finished
void syscall_3_intrman_call14(){
}

///////////////////////////////////////////////////////////////////////
//not finished
void syscall_4_intrman_call17_19(){
}

///////////////////////////////////////////////////////////////////////
__inline void syscall_0(){
    func funct;
    funct=(func)((*(volatile int*)(0x404))+4);
    __asm__ (
        "jr %0\n"
        "rfe\n"
        : "=r" (funct)
    );
}

///////////////////////////////////////////////////////////////////////
void retonly(){}

u32 callContextSwitchRequiredHandler()
{
	if (NULL != context_switch_required_handler){
		return context_switch_required_handler();
	} else {
		return 0;
	}
}

//////////////////////////////entrypoint///////////////////////////////
struct export export_stub={
	0x41C00000,
	0,
	VER(1, 2),	// 1.2 => 0x102
	0,
	"intrman",
	(func)_start,	// entrypoint
    (func)intrmanDeinit,
    (func)intrman_call2,
    (func)intrman_call3,
    (func)RegisterIntrHandler,
    (func)ReleaseIntrHandler,
    (func)EnableIntr,
    (func)DisableIntr,
    (func)CpuDisableIntr,
    (func)CpuEnableIntr,
    (func)intrman_syscall_04,		// 0x0A
    (func)intrman_syscall_08,
    (func)CpuGetICTRL,
    (func)CpuEnableICTRL,
    (func)intrman_syscall_0C,		// 0x0E
    (func)intrman_call15,
    (func)intrman_call16,			// 0x10
    (func)CpuSuspendIntr,
    (func)CpuResumeIntr,
    (func)CpuSuspendIntr,
    (func)CpuResumeIntr,
    (func)intrman_syscall_10,
    (func)intrman_syscall_14,
    (func)QueryIntrContext,			// 0x17
    (func)QueryIntrStack,
    (func)iCatchMultiIntr,
    (func)retonly,
    (func)intrman_call27,
    (func)SetCtxSwitchHandler,		// 0x1C (called by threadman)
    (func)ResetCtxSwitchHandler,
    (func)SetCtxSwitchReqHandler,			// 0x1E (called by threadman)
    (func)ResetCtxSwitchReqHandler,
	0
};

extern struct exHandler EXCEP_Int_priority_exception_handler;
extern struct exHandler EXCEP_Int_handler;
extern struct exHandler EXCEP_Sys_handler;

//////////////////////////////entrypoint///////////////////////////////
int _start(){
	int i;

	intrtable = (struct intrHandler*)(GetExHandlersTable() + 0x40);

	IMASK    = 0x00;
	DMA_ICR  = 0x00;
	DMA_ICR2 = 0x00;

	for (i=0; i < 0x2F; i++) {
		intrtable[i].handler = 0;
		intrtable[i].arg = 0;
	}
 
	soft_hw_intr_mask  = 0xFFFFFFFF;
	unknown2[1] = -1;
	unknown2[2] = -1;
	unknown2[0] = (int)intrtable;

	RegisterExceptionHandler(0, &EXCEP_Int_handler);
	RegisterPriorityExceptionHandler(0, 3, &EXCEP_Int_priority_exception_handler);
	RegisterExceptionHandler(8, &EXCEP_Sys_handler);
	RegisterIntrHandler(3, 1, &IntrHandler, 0);
	RegisterLibraryEntries(&export_stub);

	return 0;
}

void ijb_ping1()
{
	_dprintf("%s\n", __FUNCTION__);
	unsigned char* x = tempstack;
}
void ijb_trace3(u32 a0, u32 a1, u32 a2, u32 a3)
{
	__printf("trace3: 0x%x, 0x%x, 0x%x\n", a0, a1, a2);
while (1){}
}
