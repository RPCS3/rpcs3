#pragma once

#include <Utilities/dynamic_library.h>
#include <string>
#include "ps3emu_api_enums.h"
#include "ps3emu_api_structs.h"

class ps3emu_api
{
	utils::dynamic_library m_library;
	
public:
	ps3emu_api() = default;
	ps3emu_api(const std::string &path);

	unsigned int(*get_api_version)() = nullptr;
	ps3emu_api_error_code(*initialize)(const ps3emu_api_initialize_callbacks *callbacks) = nullptr;
	ps3emu_api_error_code(*destroy)() = nullptr;

	ps3emu_api_error_code(*get_version_string)(char *dest_buffer, int dest_buffer_size) = nullptr;
	ps3emu_api_error_code(*get_version_number)(int *version_number) = nullptr;
	ps3emu_api_error_code(*get_name_string)(char *dest_buffer, int dest_buffer_size) = nullptr;

	ps3emu_api_error_code(*load_elf)(const char *path) = nullptr;

	ps3emu_api_error_code(*set_state)(ps3emu_api_state state) = nullptr;
	ps3emu_api_error_code(*get_state)(ps3emu_api_state *state) = nullptr;

	bool load(const std::string &path);
	bool loaded() const;
	void close();
	
	explicit operator bool() const;
};

