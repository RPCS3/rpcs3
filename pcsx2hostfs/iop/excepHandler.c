/*********************************************************************
 * Copyright (C) 2004 Lukasz Bruun (mail@lukasz.dk)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#include <tamtypes.h>
#include "irx_imports.h"
#include "hostlink.h"

#define BUFFER_SIZE		sizeof(exception_frame_t)+4+4+128

u32 excep_buffer[BUFFER_SIZE/4] __attribute__ ((aligned(16)));

extern unsigned int pcsx2fioSendSifCmd(unsigned int cmd, void *src, unsigned int len); // lazy fix :)
extern int excepscrdump;

#define PKO_RPC_IOPEXCEP	12

// taken from smod by mrbrown, only use one function, didn't wanna include another irx.

/* Module info entry.  */
typedef struct _smod_mod_info {
	struct _smod_mod_info *next;
	u8	*name;
	u16	version;
	u16	newflags;	/* For modload shipped with games.  */
	u16	id;
	u16	flags;		/* I believe this is where flags are kept for BIOS versions.  */
	u32	entry;		/* _start */
	u32	gp;
	u32	text_start;
	u32	text_size;
	u32	data_size;
	u32	bss_size;
	u32	unused1;
	u32	unused2;
} smod_mod_info_t;


smod_mod_info_t *smod_get_next_mod(smod_mod_info_t *cur_mod)
{
  /* If cur_mod is 0, return the head of the list (IOP address 0x800).  */
  if (!cur_mod) {
    return (smod_mod_info_t *)0x800;
  } else {
    if (!cur_mod->next)
      return 0;
    else
      return cur_mod->next;
  }
  return 0;
}

char* ExceptionGetModuleName(u32 epc, u32* r_epc)
{
	smod_mod_info_t *mod_info = 0;

	while((mod_info = smod_get_next_mod(mod_info)) != 0)
	{
		if((epc >= mod_info->text_start) && (epc <= (mod_info->text_start+mod_info->text_size)))
		{
			if(r_epc)
				*r_epc = epc -  mod_info->text_start;
			
			return mod_info->name;
		}	
	}
	
	return 0;
}

static void excep_handler2(exception_frame_t *frame)
{
	u32 r_epc; // relative epc
	char *module_name;
	u32 len;
	u32* buffer = excep_buffer;

	module_name = ExceptionGetModuleName(frame->epc, &r_epc);

	len = strlen(module_name);
	

	buffer[0] = 0x0d02beba; // reverse engineering..
	buffer++;
	
	memcpy(buffer, frame, BUFFER_SIZE);
	buffer+= (sizeof(exception_frame_t)/4);
	
	buffer[0] = r_epc;
	buffer[1] = len;
	buffer+=2;
	memcpy(buffer, module_name, len+1);

	//pcsx2fioSendSifCmd(PKO_RPC_IOPEXCEP, excep_buffer, BUFFER_SIZE);
}


static void excep_handler(exception_type_t type, exception_frame_t *frame)
{
	excep_handler2(frame); // Don't know why this works, but keeps iop alive.
}


////////////////////////////////////////////////////////////////////////
// Installs iop exception handlers for the 'usual' exceptions..
void installExceptionHandlers(void)
{
	s32 i;

	for(i=1; i < 8; i++)
		set_exception_handler(i, excep_handler);
	
	for(i=10; i < 13; i++)
		set_exception_handler(i, excep_handler);

}
