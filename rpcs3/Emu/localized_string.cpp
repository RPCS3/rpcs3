#include "stdafx.h"
#include "localized_string.h"
#include "System.h"

std::string get_localized_string(localized_string_id id, const char* args)
{
	return Emu.GetCallbacks().get_localized_string(id, args);
}

std::u32string get_localized_u32string(localized_string_id id, const char* args)
{
	return Emu.GetCallbacks().get_localized_u32string(id, args);
}
