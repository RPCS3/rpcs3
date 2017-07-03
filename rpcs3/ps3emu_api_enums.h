#pragma once

#ifndef _PS3EMU_API_ENUMS
#define _PS3EMU_API_ENUMS

#ifdef __cplusplus
extern"C"
{
#endif /* __cplusplus */

	typedef enum
	{
		ps3emu_api_ok,
		ps3emu_api_bad_argument,
		ps3emu_api_not_found,
		ps3emu_api_internal_error,
		ps3emu_api_not_initialized,
		ps3emu_api_already_initialized
	} ps3emu_api_error_code;

	enum
	{
		ps3emu_api_version = 1,
		ps3emu_api_max_name_length = 16,
		ps3emu_api_max_version_length = 64
	};

	typedef enum
	{
		ps3emu_api_state_idle,
		ps3emu_api_state_stoping,
		ps3emu_api_state_stopped,
		ps3emu_api_state_pausing,
		ps3emu_api_state_paused,
		ps3emu_api_state_starting,
		ps3emu_api_state_started
	} ps3emu_api_state;

	typedef enum
	{
		ps3emu_api_window_null,
		ps3emu_api_window_opengl,
		ps3emu_api_window_vulkan
		/* ps3emu_api_window_direct3d */
	} ps3emu_api_window_type;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PS3EMU_API_ENUMS */
