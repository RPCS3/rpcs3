#include "stdafx.h"
#include "localized_string.h"
#include "emu_callbacks.h"

std::string get_localized_string(localized_string_id id, const char* args)
{
	return g_emu_callbacks.get_localized_string(id, args);
}

std::u32string get_localized_u32string(localized_string_id id, const char* args)
{
	return g_emu_callbacks.get_localized_u32string(id, args);
}
