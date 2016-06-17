#ifndef _PS3EMU_API_STRUCTS
#define _PS3EMU_API_STRUCTS

#include "ps3emu_api_enums.h"

#ifdef __cplusplus
extern"C"
{
#endif /* __cplusplus */
	typedef struct ps3emu_api_window_handle_s * ps3emu_api_window;

	typedef struct
	{
		ps3emu_api_error_code(*create_window)(ps3emu_api_window *window, ps3emu_api_window_type type, unsigned int version);
		ps3emu_api_error_code(*destroy_window)(ps3emu_api_window window);
		ps3emu_api_error_code(*flip)(ps3emu_api_window window);
	} ps3emu_api_initialize_callbacks;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PS3EMU_API_STRUCTS */
