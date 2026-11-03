#pragma once

#include "util/types.hpp"
#include "util/endian.hpp"
#include "nfc_reader.h"

// USIO USB endpoints
enum class usio_endpoint : u8
{
	command_out  = 0x01, // Bulk OUT: commands and write data
	response_in  = 0x82, // Bulk IN: read responses
	interrupt_in = 0x83, // Interrupt IN: declared, but never used
};

// USIO command operations (high nibble of the first command byte)
// Note: write contains the read bit, so write has to be checked before read
enum usio_operation : u8
{
	USIO_COMMAND_READ  = 0x10,
	USIO_COMMAND_WRITE = 0x90,
	USIO_COMMAND_INIT  = 0xA0,
};

// USIO channels
enum class usio_channel : u8
{
	registers = 0,
	firmware  = 1, // Firmware update, not used in emulation
	sram_base = 2, // Channel 2 + n is SRAM page n
};

// USIO channel 0 registers
enum class usio_reg : u16
{
	get_buffer           = 0x0000,
	set_system_error     = 0x0002,
	reset                = 0x0008, // Init
	clear_sram           = 0x000A, // Init, or write with usio_clear_sram_key
	set_expansion_mode   = 0x0028,
	set_hopper_request_0 = 0x0048, // + n * usio_hopper_stride for hopper n
	set_hopper_request_1 = 0x0058,
	set_hopper_request_2 = 0x0068,
	set_hopper_request_3 = 0x0078,
	set_hopper_limit_0   = 0x004A, // + n * usio_hopper_stride for hopper n
	set_hopper_limit_1   = 0x005A,
	set_hopper_limit_2   = 0x006A,
	set_hopper_limit_3   = 0x007A,
	card_reader_0_status = 0x0080, // Size of the pending card reader response
	card_reader_0_unk    = 0x008D, // Written, but ignored
	card_reader_1_status = 0x0090,
	card_reader_1_unk    = 0x009D,
	input_tekken         = 0x1000, // Possibly the input block of I/O board 0 rather than game specific
	input_taiko          = 0x1080, // Possibly the input block of I/O board 1 rather than game specific
	firmware_info_0      = 0x1800, // Reads start at (reg - firmware_info_0) into the info block
	firmware_info_1      = 0x1880,
	card_reader_0_read   = 0x7000, // Read the pending card reader response
	card_reader_0_write  = 0x7400, // Send a frame to the card reader
	card_reader_1_read   = 0x7800,
	card_reader_1_write  = 0x7C00,
};

constexpr u16 usio_hopper_stride = 0x10;
constexpr u16 usio_clear_sram_key = 0x6666;

// USIO command sent on the command endpoint
struct usio_command
{
	u8 op_channel = 0;     // High nibble: operation, low nibble: channel
	u8 reg_check = 0;      // Write only: ~(reg >> 8) & 0xF0
	le_t<u16, 1> reg = 0;
	le_t<u16, 1> length = 0;
};

static_assert(sizeof(usio_command) == 6);

struct usio_io_status
{
	bool test_on = false;
	bool test_key_pressed = false;
	bool coin_key_pressed = false;
	bool card_tapped = false;
	le_t<u16> coin_counter = 0;
	usz card_index = 0;
	s8 vital_sensors[2] = {0, 0};
	s8 wheel_rotation = 0;
	std::array<bool, 4> taiko_hit_already = {}; // Status of taiko_hit_side_left, taiko_hit_center_right, taiko_hit_side_right, and taiko_hit_center_left from last polling
	nfc_reader card_reader{};
};
