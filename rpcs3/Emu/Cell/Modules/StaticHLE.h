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

	statichle_handler(const statichle_handler&) = delete;
	statichle_handler& operator=(const statichle_handler&) = delete;

	bool load_patterns();
	bool check_against_patterns(vm::cptr<u8>& data, u32 size, u32 addr);

protected:
	static u16 gen_CRC16(const u8* data_p, usz length);

	std::vector<shle_pattern> hle_patterns;
};
