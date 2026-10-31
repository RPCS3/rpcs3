#pragma once

#include "util/types.hpp"

// MIFARE Classic card layout
constexpr usz mifare_block_size = 0x10;
constexpr usz mifare_uid_size = 4;

// MIFARE Classic command codes
enum class mifare_cmd : u8
{
	auth_key_a = 0x60,
	auth_key_b = 0x61,
	read       = 0x30,
	write      = 0xA0,
};
