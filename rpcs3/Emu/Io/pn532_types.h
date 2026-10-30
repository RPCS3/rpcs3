#pragma once

#include "util/types.hpp"
#include "util/endian.hpp"
#include "mifare_types.h"

#include <array>

// NXP PN532 NFC controller host protocol

// Preamble and start code of every PN532 frame
constexpr std::array<u8, 3> pn532_frame_start = {0x00, 0x00, 0xFF};

// PN532 ACK frame
constexpr std::array<u8, 6> pn532_ack_frame = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};

// PN532 frame identifier (direction of an information frame)
enum class pn532_tfi : u8
{
	syntax_error  = 0x7F,
	host_to_pn532 = 0xD4,
	pn532_to_host = 0xD5,
};

// PN532 command codes (the response code is always the command code + 1)
enum class pn532_cmd : u8
{
	get_firmware_version   = 0x02,
	read_register          = 0x06,
	write_register         = 0x08,
	read_gpio              = 0x0C,
	write_gpio             = 0x0E,
	set_parameters         = 0x12,
	sam_configuration      = 0x14,
	power_down             = 0x16,
	alpar_command_for_tda  = 0x18,
	rf_configuration       = 0x32,
	in_data_exchange       = 0x40,
	in_deselect            = 0x44,
	in_list_passive_target = 0x4A,
	in_release             = 0x52,
	in_select              = 0x54,
};

// PN532 status byte of InDataExchange, InDeselect, InRelease, InSelect and PowerDown responses
enum class pn532_status : u8
{
	success           = 0x00,
	mifare_auth_error = 0x14,
};

constexpr u8 pn532_response_code(pn532_cmd cmd)
{
	return static_cast<u8>(static_cast<u8>(cmd) + 1);
}

// PN532 normal information frame header
struct pn532_frame_header
{
	std::array<u8, 3> start = pn532_frame_start;
	u8 len = 0; // Length of TFI + data
	u8 lcs = 0; // Length checksum (len + lcs == 0)
	pn532_tfi tfi = pn532_tfi::pn532_to_host;
};

// PN532 extended information frame header (len and lcs are 0xFF in the normal header position)
struct pn532_extended_frame_header
{
	std::array<u8, 3> start = pn532_frame_start;
	std::array<u8, 2> marker = {0xFF, 0xFF};
	be_t<u16, 1> len = 0; // Length of TFI + data
	u8 lcs = 0;           // Length checksum (len_m + len_l + lcs == 0)
	pn532_tfi tfi = pn532_tfi::pn532_to_host;
};

// PN532 frame footer
struct pn532_frame_footer
{
	u8 dcs = 0; // Data checksum (tfi + data + dcs == 0)
	u8 postamble = 0x00;
};

// PN532 ReadRegister request (only the first register address is inspected)
struct pn532_read_register_request
{
	pn532_cmd cmd = pn532_cmd::read_register;
	be_t<u16, 1> address = 0;
};

// PN532 ReadGPIO response
struct pn532_read_gpio_response
{
	u8 cmd = pn532_response_code(pn532_cmd::read_gpio);
	u8 p3 = 0x00;
	u8 p7 = 0x06;
	u8 i0i1 = 0x00;
};

// PN532 InListPassiveTarget response (one ISO/IEC 14443 Type A target)
struct pn532_in_list_passive_target_response
{
	u8 cmd = pn532_response_code(pn532_cmd::in_list_passive_target);
	u8 nb_tg = 0x01;
	u8 tg = 0x01;
	be_t<u16, 1> sens_res = 0x0004; // ATQA
	u8 sel_res = 0x08;              // SAK: MIFARE Classic 1K
	u8 nfcid_length = mifare_uid_size;
	std::array<u8, mifare_uid_size> nfcid{};
};

// Requires at least sizeof(pn532_frame_header) bytes
inline bool pn532_is_extended_frame(const std::vector<u8>& in)
{
	const auto header = read_from_ptr<pn532_frame_header>(in);
	return header.len == 0xFF && header.lcs == 0xFF;
};

static_assert(sizeof(pn532_frame_header) == 6);
static_assert(sizeof(pn532_extended_frame_header) == 9);
static_assert(sizeof(pn532_frame_footer) == 2);
static_assert(sizeof(pn532_read_register_request) == 3);
static_assert(sizeof(pn532_read_gpio_response) == 4);
static_assert(sizeof(pn532_in_list_passive_target_response) == 11);
