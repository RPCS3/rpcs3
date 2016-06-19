#include "stdafx.h"
#include "ps3emu_api_enums.h"
#include "ps3emu_api_structs.h"
#include "rpcs3_version.h"
#include "Emu/System.h"

#ifdef _MSC_VER
	#define UTILS_DLL_C_EXPORT extern "C" __declspec(dllexport)
#else
	#define UTILS_DLL_C_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static bool g_is_initialized = false;

UTILS_DLL_C_EXPORT unsigned int ps3emu_api_get_api_version()
{
	return ps3emu_api_version;
}

UTILS_DLL_C_EXPORT ps3emu_api_error_code ps3emu_api_initialize(const ps3emu_api_initialize_callbacks *callbacks)
{
	if (g_is_initialized)
	{
		return ps3emu_api_already_initialized;
	}

	if (!callbacks)
	{
		return ps3emu_api_bad_argument;
	}

	g_is_initialized = true;

	//TODO
	return ps3emu_api_ok;
}

UTILS_DLL_C_EXPORT ps3emu_api_error_code ps3emu_api_destroy()
{
	if (!g_is_initialized)
	{
		return ps3emu_api_not_initialized;
	}

	g_is_initialized = false;

	//TODO
	return ps3emu_api_ok;
}

UTILS_DLL_C_EXPORT ps3emu_api_error_code ps3emu_api_get_version_string(char *dest_buffer, int dest_buffer_size)
{
	if (!g_is_initialized)
	{
		return ps3emu_api_not_initialized;
	}

	if (!dest_buffer || dest_buffer_size <= 0)
	{
		return ps3emu_api_bad_argument;
	}

	if (dest_buffer_size > ps3emu_api_max_version_length)
	{
		dest_buffer_size = ps3emu_api_max_version_length;
	}

	const std::string version_string = rpcs3::version.to_string();

	if (dest_buffer_size > version_string.length())
	{
		dest_buffer_size = version_string.length();
	}

	std::memcpy(dest_buffer, version_string.c_str(), dest_buffer_size - 1);
	dest_buffer[dest_buffer_size - 1] = '\0';
	return ps3emu_api_ok;
}

UTILS_DLL_C_EXPORT ps3emu_api_error_code ps3emu_api_get_version_number(int *version_number)
{
	if (!g_is_initialized)
	{
		return ps3emu_api_not_initialized;
	}

	if (!version_number)
	{
		return ps3emu_api_bad_argument;
	}

	*version_number = rpcs3::version.to_hex();
	return ps3emu_api_ok;
}

UTILS_DLL_C_EXPORT ps3emu_api_error_code ps3emu_api_get_name_string(char *dest_buffer, int dest_buffer_size)
{
	if (!g_is_initialized)
	{
		return ps3emu_api_not_initialized;
	}

	if (!dest_buffer || dest_buffer_size <= 0)
	{
		return ps3emu_api_bad_argument;
	}

	if (dest_buffer_size > ps3emu_api_max_name_length)
	{
		dest_buffer_size = ps3emu_api_max_name_length;
	}

	const std::string name_string = "RPCS3";

	if (dest_buffer_size > name_string.length())
	{
		dest_buffer_size = name_string.length();
	}

	std::memcpy(dest_buffer, name_string.c_str(), dest_buffer_size - 1);
	dest_buffer[dest_buffer_size - 1] = '\0';

	return ps3emu_api_ok;
}

UTILS_DLL_C_EXPORT ps3emu_api_error_code ps3emu_api_load_elf(const char *path)
{
	if (!g_is_initialized)
	{
		return ps3emu_api_not_initialized;
	}

	if (!path)
	{
		return ps3emu_api_bad_argument;
	}

	if (!fs::is_file(path))
	{
		return ps3emu_api_not_found;
	}

	Emu.SetPath(path);
	Emu.Load();

	return ps3emu_api_ok;
}

UTILS_DLL_C_EXPORT ps3emu_api_error_code ps3emu_api_set_state(ps3emu_api_state state)
{
	if (!g_is_initialized)
	{
		return ps3emu_api_not_initialized;
	}

	//TODO state machine
	switch (state)
	{
	case ps3emu_api_state_stoping:
		Emu.Stop();
		break;

	case ps3emu_api_state_pausing:
		Emu.Pause();
		break;

	case ps3emu_api_state_starting:
		if (Emu.IsPaused())
		{
			Emu.Resume();
		}
		else
		{
			Emu.Run();
		}
		break;

	default:
		return ps3emu_api_bad_argument;
	}

	return ps3emu_api_ok;
}

UTILS_DLL_C_EXPORT ps3emu_api_error_code ps3emu_api_get_state(ps3emu_api_state *state)
{
	if (!g_is_initialized)
	{
		return ps3emu_api_not_initialized;
	}

	if (!state)
	{
		return ps3emu_api_bad_argument;
	}

	if (Emu.IsRunning())
	{
		*state = ps3emu_api_state_started;
	}
	else if (Emu.IsPaused())
	{
		*state = ps3emu_api_state_paused;
	}
	else if (Emu.IsStopped())
	{
		*state = ps3emu_api_state_stopped;
	}
	else if (Emu.IsReady())
	{
		*state = ps3emu_api_state_idle;
	}
	else
	{
		return ps3emu_api_internal_error;
	}

	return ps3emu_api_ok;
}
