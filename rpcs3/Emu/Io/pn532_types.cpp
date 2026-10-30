#include "stdafx.h"
#include "pn532_types.h"
#include "usio_types.h"

bool pn532_verify_request(std::span<const u8> in)
{
	if (in.size() < pn532_ack_frame.size())
		return false;

	if (!std::equal(pn532_start_code.begin(), pn532_start_code.end(), in.begin()))
		return false;

	if (std::equal(pn532_ack_frame.begin(), pn532_ack_frame.end(), in.begin())) // Further checks are not applicable for ACK, so simply return true if it is an ACK packet
		return true;

	if (in.size() < sizeof(pn532_frame_header))
		return false;

	usz tfi_offset = 0; // TFI is the last byte of either header
	usz len = 0;
	u8 len_checksum = 0;
	pn532_tfi tfi{};

	if (pn532_is_extended_frame(in)) // Extended Information Frame
	{
		if (in.size() < sizeof(pn532_extended_frame_header))
			return false;

		const auto header = read_from_ptr<pn532_extended_frame_header>(in);
		tfi_offset = offsetof(pn532_extended_frame_header, tfi);
		len = header.len;
		len_checksum = static_cast<u8>((len >> 8) + (len & 0xFF) + header.lcs);
		tfi = header.tfi;
	}
	else // Normal Information Frame
	{
		const auto header = read_from_ptr<pn532_frame_header>(in);
		tfi_offset = offsetof(pn532_frame_header, tfi);
		len = header.len;
		len_checksum = static_cast<u8>(header.len + header.lcs);
		tfi = header.tfi;
	}

	// len covers TFI + data, followed by DCS
	if (len < 2 || len_checksum != 0x00 || tfi != pn532_tfi::host_to_pn532 || in.size() < tfi_offset + len + sizeof(pn532_frame_footer::dcs))
		return false;

	// TFI + data + DCS
	u8 data_checksum = 0;
	for (usz i = tfi_offset; i < tfi_offset + len + 1; i++)
		data_checksum += in[i];

	return data_checksum == 0x00;
}

void pn532_build_response(std::vector<u8>& out, const std::vector<u8>& in)
{
	out.push_back(pn532_preamble);

	if (in.empty())
	{
		out.insert(out.end(), pn532_error_frame.begin(), pn532_error_frame.end());
		out.push_back(pn532_postamble);
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
		pn532_extended_frame_header header{};
		header.len = static_cast<u16>(total_len);
		header.lcs = static_cast<u8>(0x00 - ((total_len >> 8) + (total_len & 0xFF)));
		append(header);
	}
	else // Normal Information Frame
	{
		pn532_frame_header header{};
		header.len = static_cast<u8>(total_len);
		header.lcs = static_cast<u8>(0x00 - total_len);
		append(header);
	}

	pn532_frame_footer footer{};
	footer.dcs = static_cast<u8>(0x00 - static_cast<u8>(pn532_tfi::pn532_to_host));

	for (const auto& data : in)
	{
		out.push_back(data);
		footer.dcs -= data;
	}

	append(footer);
}
