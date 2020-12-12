#pragma once

#include "util/types.hpp"
#include "Emu/Memory/vm_ptr.h"
#include <vector>

struct shle_pattern
{
	u16 start_pattern[32];
	u8 crc16_length;
	u16 crc16;
	u16 total_length;
	std::string _module;
	std::string name;

	u32 fnid;
};

class statichle_handler
{
public:
	statichle_handler(int);
	~statichle_handler();

	bool load_patterns();
	bool check_against_patterns(vm::cptr<u8>& data, u32 size, u32 addr);

protected:
	uint16_t gen_CRC16(const uint8_t* data_p, size_t length);

	std::vector<shle_pattern> hle_patterns;
};
