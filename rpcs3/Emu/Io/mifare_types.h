#pragma once

#include "util/types.hpp"

// MIFARE Classic card layout
constexpr usz mifare_block_size = 0x10;
constexpr usz mifare_uid_size = 4;
constexpr usz mifare_ultralight_uid_size = 7;

// MIFARE specific command codes
enum class mifare_cmd : u8
{
	auth_key_a     = 0x60,
	auth_key_b     = 0x61,
	read_16_bytes  = 0x30,
	write_16_bytes = 0xA0,
	write_4_bytes  = 0xA2,
	increment      = 0xC1,
	decrement      = 0xC0,
	transfer       = 0xB0,
	restore        = 0xC2,
};
