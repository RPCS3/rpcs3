#pragma once

#include "util/types.hpp"
#include "util/endian.hpp"
#include "mifare_types.h"

#include <algorithm>
#include <array>
#include <span>
#include <vector>

struct usio_io_status;

// NFC controller host protocol
class nfc_reader
{
public:
	// Writes a response to out
	static void write_response(std::vector<u8>& buf, std::vector<u8>& response, const std::array<u8, 0x40>& card, const usio_io_status& status);

private:
	// Frames are wrapped in a preamble and a postamble byte, which are optional when receiving
	static constexpr u8 preamble = 0x00;
	static constexpr u8 postamble = 0x00;

	// Start code of every frame
	static constexpr std::array<u8, 2> start_code = {0x00, 0xFF};

	// ACK frame (without preamble and postamble)
	static constexpr std::array<u8, 4> ack_frame = {0x00, 0xFF, 0x00, 0xFF};

	// Error frame (application level error, e.g. syntax error) (without preamble and postamble)
	static constexpr std::array<u8, 6> error_frame = {0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x81};

	// Frame identifier (direction of an information frame)
	enum class frame_identifier : u8
	{
		to_device = 0xD4,
		to_host = 0xD5,
	};

	// Command codes (the response code is always the command code + 1)
	enum class command_code : u8
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

	// Status byte of InDataExchange, InDeselect, InRelease, InSelect and PowerDown responses
	enum class status : u8
	{
		success           = 0x00,
		mifare_auth_error = 0x14,
	};

	static constexpr u8 response_code(command_code command)
	{
		return static_cast<u8>(static_cast<u8>(command) + 1);
	}

	// Normal information frame header
	struct frame_header
	{
		std::array<u8, 2> start = start_code;
		u8 len = 0; // Length of TFI + data
		u8 lcs = 0; // Length checksum (len + lcs == 0)
		frame_identifier tfi = frame_identifier::to_host;
	};

	// Extended information frame header (len and lcs are 0xFF in the normal header position)
	struct extended_frame_header
	{
		std::array<u8, 2> start = start_code;
		std::array<u8, 2> marker = {0xFF, 0xFF};
		be_t<u16, 1> len = 0; // Length of TFI + data
		u8 lcs = 0;           // Length checksum (len_m + len_l + lcs == 0)
		frame_identifier tfi = frame_identifier::to_host;
	};

	// Frame footer
	struct frame_footer
	{
		u8 dcs = 0; // Data checksum (tfi + data + dcs == 0)
		u8 postamble = nfc_reader::postamble;
	};

	// ReadRegister request (only the first register address is inspected)
	struct read_register_request
	{
		command_code cmd = command_code::read_register;
		be_t<u16, 1> address = 0;
	};

	// InDataExchange request carrying a MIFARE command
	struct in_data_exchange_request
	{
		command_code cmd = command_code::in_data_exchange;
		u8 tg = 0;
		mifare_cmd mifare = mifare_cmd::read;
		u8 block = 0;
	};

	// ReadGPIO response
	struct read_gpio_response
	{
		u8 cmd = response_code(command_code::read_gpio);
		u8 p3 = 0x00;
		u8 p7 = 0x06;
		u8 i0i1 = 0x00;
	};

	// InDataExchange response (MIFARE READ of one 16-byte block)
	struct in_data_exchange_response
	{
		u8 cmd = response_code(command_code::in_data_exchange);
		u8 status = 0x00;
		std::array<u8, mifare_block_size> block{};
	};

	// InListPassiveTarget response (one ISO/IEC 14443 Type A target)
	struct in_list_passive_target_response
	{
		u8 cmd = response_code(command_code::in_list_passive_target);
		u8 nb_tg = 0x01;
		u8 tg = 0x01;
		be_t<u16, 1> sens_res = 0x0004; // ATQA
		u8 sel_res = 0x08;              // SAK: MIFARE Classic 1K
		u8 nfcid_length = mifare_uid_size;
		std::array<u8, mifare_uid_size> nfcid{};
	};

	// Skip the preamble (any bytes before the start code) and return the frame starting at the start code
	// Returns an empty span if there is no start code
	static inline std::span<const u8> find_frame(const std::vector<u8>& in)
	{
		const auto it = std::search(in.begin(), in.end(), start_code.begin(), start_code.end());
		return {it, in.end()};
	}

	// Requires at least sizeof(frame_header) bytes
	static inline bool is_extended_frame(std::span<const u8> frame)
	{
		const auto header = read_from_ptr<frame_header>(frame);
		return header.len == 0xFF && header.lcs == 0xFF;
	}

	// Writes an ACK frame (with preamble and postamble, unless removed) to the out buffer
	static inline void append_ack(std::vector<u8>& out, bool remove_pre_and_postamble = false)
	{
		if (!remove_pre_and_postamble)
			out.push_back(preamble);

		out.insert(out.end(), ack_frame.begin(), ack_frame.end());

		if (!remove_pre_and_postamble)
			out.push_back(postamble);
	}

	// Expects a frame starting at the start code (see find_frame), the postamble is optional
	static bool verify_request(std::span<const u8> in);

	// Appends to out, which already contains the ACK frame
	// The preamble and postamble can be removed with the fRemovePrePost flag of SetParameters
	static void build_response(std::vector<u8>& out, const std::vector<u8>& in = {}, bool remove_pre_and_postamble = false);

	static_assert(sizeof(frame_header) == 5);
	static_assert(sizeof(extended_frame_header) == 8);
	static_assert(sizeof(frame_footer) == 2);
	static_assert(sizeof(read_register_request) == 3);
	static_assert(sizeof(in_data_exchange_request) == 4);
	static_assert(sizeof(read_gpio_response) == 4);
	static_assert(sizeof(in_data_exchange_response) == 18);
	static_assert(sizeof(in_list_passive_target_response) == 11);
};
