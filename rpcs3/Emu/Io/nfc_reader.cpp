#include "stdafx.h"
#include "nfc_reader.h"

LOG_CHANNEL(usio_log, "USIO");

usz nfc_reader::get_buffer_size() const
{
	usz size = 0;

	if (m_send_ack)
		size += ack_frame.size() + (m_remove_pre_and_postamble ? 0 : 2);

	if (m_send_response)
		size += m_response.size();

	return size;
}

void nfc_reader::write_buffer(std::vector<u8>& out)
{
	if (m_send_ack)
	{
		write_ack(out);
		m_send_ack = false;
	}

	if (m_send_response)
	{
		out.insert(out.end(), m_response.begin(), m_response.end());
		m_send_response = false;
	}
}

bool nfc_reader::verify_request(std::span<const u8> in)
{
	if (in.size() < ack_frame.size())
		return false;

	if (!std::equal(start_code.begin(), start_code.end(), in.begin()))
		return false;

	if (std::equal(ack_frame.begin(), ack_frame.end(), in.begin()) ||
		std::equal(nack_frame.begin(), nack_frame.end(), in.begin())) // Further checks are not applicable for ACK and NACK, so simply return true
		return true;

	if (in.size() < sizeof(frame_header))
		return false;

	usz tfi_offset = 0; // TFI is the last byte of either header
	usz len = 0;
	u8 len_checksum = 0;
	frame_identifier tfi{};

	if (is_extended_frame(in)) // Extended Information Frame
	{
		if (in.size() < sizeof(extended_frame_header))
			return false;

		const auto header = read_from_ptr<extended_frame_header>(in);
		tfi_offset = offsetof(extended_frame_header, tfi);
		len = header.len;
		len_checksum = static_cast<u8>((len >> 8) + (len & 0xFF) + header.lcs);
		tfi = header.tfi;
	}
	else // Normal Information Frame
	{
		const auto header = read_from_ptr<frame_header>(in);
		tfi_offset = offsetof(frame_header, tfi);
		len = header.len;
		len_checksum = static_cast<u8>(header.len + header.lcs);
		tfi = header.tfi;
	}

	// len covers TFI + data, followed by DCS
	if (len < 2 || len_checksum != 0x00 || tfi != frame_identifier::to_device || in.size() < tfi_offset + len + sizeof(frame_footer::dcs))
		return false;

	// TFI + data + DCS
	u8 data_checksum = 0;
	for (usz i = tfi_offset; i < tfi_offset + len + 1; i++)
		data_checksum += in[i];

	return data_checksum == 0x00;
}

void nfc_reader::build_response(const std::vector<u8>& in)
{
	auto& out = m_response;

	if (!m_remove_pre_and_postamble)
		out.push_back(preamble);

	if (in.empty())
	{
		out.insert(out.end(), error_frame.begin(), error_frame.end());

		if (!m_remove_pre_and_postamble)
			out.push_back(postamble);

		return;
	}

	const auto append = [&out](const auto& value)
	{
		const auto bytes = to_bytes(value);
		out.insert(out.end(), bytes.begin(), bytes.end());
	};

	const usz total_len = in.size() + 1; // Including TFI

	if (total_len > 0xFF) // Extended Information Frame
	{
		extended_frame_header header{};
		header.len = static_cast<u16>(total_len);
		header.lcs = static_cast<u8>(0x00 - ((total_len >> 8) + (total_len & 0xFF)));
		append(header);
	}
	else // Normal Information Frame
	{
		frame_header header{};
		header.len = static_cast<u8>(total_len);
		header.lcs = static_cast<u8>(0x00 - total_len);
		append(header);
	}

	frame_footer footer{};
	footer.dcs = static_cast<u8>(0x00 - static_cast<u8>(frame_identifier::to_host));

	for (const auto& data : in)
	{
		out.push_back(data);
		footer.dcs -= data;
	}

	append(footer);

	if (!m_remove_pre_and_postamble)
		out.push_back(postamble);
}

void nfc_reader::process_request(const std::vector<u8>& in, const std::array<u8, 0x40>& card, bool card_tapped)
{
	const std::span<const u8> frame = find_frame(in);

	if (!verify_request(frame))
		return;

	// In case NACK is received, the pending response should be retransmitted.
	// So only clear the pending response buffer when a new legitimate request or ACK is received.
	if (std::equal(nack_frame.begin(), nack_frame.end(), frame.begin()))
	{
		m_send_response = true; // Only retransmit the actual response; do not retransmit ACK
		return;
	}

	m_response.clear();
	m_send_ack = false;
	m_send_response = false;

	// Do not respond to ACK
	if (std::equal(ack_frame.begin(), ack_frame.end(), frame.begin()))
		return;

	const bool is_extended = is_extended_frame(frame);
	const usz data_offset = is_extended ? sizeof(extended_frame_header) : sizeof(frame_header);
	const usz data_len = (is_extended ? static_cast<usz>(read_from_ptr<extended_frame_header>(frame).len) : static_cast<usz>(read_from_ptr<frame_header>(frame).len)) - 1; // Excluding TFI
	const std::span<const u8> payload = frame.subspan(data_offset, data_len); // data_offset + data_len boundary check is done in verify_request()

	const auto command = static_cast<command_code>(::at32(payload, 0));
	const u8 res = response_code(command);

	switch (command)
	{
	case command_code::read_register:
	{
		if (payload.size() >= sizeof(read_register_request))
		{
			const auto req = read_from_ptr<read_register_request>(payload);

			if ((static_cast<u16>(req.address) & 0xFF) == 0x1C)
			{
				build_response({res, 0xFF, 0x3F, 0x0E, 0xF1, 0xFF, 0x3F, 0x0E, 0xF1});
			}
			else
			{
				build_response({res, 0xDC, 0xF4, 0x3F, 0x11, 0x4D, 0x85, 0x61, 0xF1, 0x26, 0x6A, 0x87});
			}
		}
		break;
	}
	case command_code::write_register:
	{
		build_response({res, 0x00});
		break;
	}
	case command_code::read_gpio:
	{
		build_response(to_bytes(read_gpio_response{}));
		break;
	}
	case command_code::write_gpio:
	{
		build_response({res});
		break;
	}
	case command_code::set_parameters:
	{
		build_response({res});
		break;
	}
	case command_code::alpar_command_for_tda:
	{
		build_response({res});
		break;
	}
	case command_code::rf_configuration:
	{
		build_response({res});
		break;
	}
	case command_code::in_data_exchange:
	{
		if (payload.size() < sizeof(in_data_exchange_request) || !card_tapped)
		{
			build_response({res, 0x01});
			break;
		}

		const auto req = read_from_ptr<in_data_exchange_request>(payload);

		if (req.block < (card.size() / mifare_block_size))
		{
			in_data_exchange_response resp{};
			std::memcpy(resp.block.data(), card.data() + req.block * mifare_block_size, mifare_block_size);
			build_response(to_bytes(resp));
		}
		else
		{
			build_response({res, 0x01});
		}
		break;
	}
	case command_code::in_list_passive_target:
	{
		if (card_tapped)
		{
			in_list_passive_target_response resp{};
			std::memcpy(resp.nfcid.data(), card.data(), mifare_uid_size);
			build_response(to_bytes(resp));
		}
		else
		{
			build_response({res, 0x00}); // No targets found
		}
		break;
	}
	case command_code::in_release:
	{
		build_response({res, 0x01, 0x00});
		break;
	}
	default:
	{
		usio_log.warning("Unhandled NFC reader command: 0x%02X", static_cast<u8>(command));
		build_response(); // Respond with an error frame
		break;
	}
	}

	m_send_ack = true;
	m_send_response = true;
}
