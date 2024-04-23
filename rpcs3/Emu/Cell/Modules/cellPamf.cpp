#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"

#include <bitset>
#include "cellPamf.h"

const std::function<bool()> SQUEUE_ALWAYS_EXIT = []() { return true; };
const std::function<bool()> SQUEUE_NEVER_EXIT = []() { return false; };

bool squeue_test_exit()
{
	return Emu.IsStopped();
}

LOG_CHANNEL(cellPamf);

template<>
void fmt_class_string<CellPamfError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_PAMF_ERROR_STREAM_NOT_FOUND);
			STR_CASE(CELL_PAMF_ERROR_INVALID_PAMF);
			STR_CASE(CELL_PAMF_ERROR_INVALID_ARG);
			STR_CASE(CELL_PAMF_ERROR_UNKNOWN_TYPE);
			STR_CASE(CELL_PAMF_ERROR_UNSUPPORTED_VERSION);
			STR_CASE(CELL_PAMF_ERROR_UNKNOWN_STREAM);
			STR_CASE(CELL_PAMF_ERROR_EP_NOT_FOUND);
			STR_CASE(CELL_PAMF_ERROR_NOT_AVAILABLE);
		}

		return unknown;
	});
}

error_code pamfVerifyMagicAndVersion(vm::cptr<PamfHeader> pAddr, vm::ptr<CellPamfReader> pSelf)
{
	ensure(!!pAddr); // Not checked on LLE

	if (pAddr->magic != std::bit_cast<be_t<u32>>("PAMF"_u32))
	{
		return CELL_PAMF_ERROR_UNKNOWN_TYPE;
	}

	if (pSelf)
	{
		pSelf->isPsmf = false;
	}

	be_t<u16> version;

	if (pAddr->version == std::bit_cast<be_t<u32>>("0040"_u32))
	{
		version = 40;
	}
	else if (pAddr->version == std::bit_cast<be_t<u32>>("0041"_u32))
	{
		version = 41;
	}
	else
	{
		return CELL_PAMF_ERROR_UNSUPPORTED_VERSION;
	}

	if (pSelf)
	{
		pSelf->version = version;
	}

	return CELL_OK;
}

error_code pamfGetHeaderAndDataSize(vm::cptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<u64> headerSize, vm::ptr<u64> dataSize)
{
	ensure(!!pAddr); // Not checked on LLE

	if (error_code ret = pamfVerifyMagicAndVersion(pAddr, vm::null); ret != CELL_OK)
	{
		return ret;
	}

	const u64 header_size = pAddr->header_size * 0x800ull;
	const u64 data_size = pAddr->data_size * 0x800ull;

	if (header_size == 0 || (fileSize != 0 && header_size + data_size != fileSize))
	{
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	if (headerSize)
	{
		*headerSize = header_size;
	}

	if (dataSize)
	{
		*dataSize = data_size;
	}

	return CELL_OK;
}

error_code pamfTypeChannelToStream(u8 type, u8 ch, u8* stream_coding_type, u8* stream_id, u8* private_stream_id)
{
	// This function breaks if ch is greater than 15, LLE doesn't check for this
	ensure(ch < 16);

	u8 _stream_coding_type;
	u8 _stream_id;
	u8 _private_stream_id;

	switch (type)
	{
	case CELL_PAMF_STREAM_TYPE_AVC:
	{
		_stream_coding_type = PAMF_STREAM_CODING_TYPE_AVC;
		_stream_id = 0xe0 | ch;
		_private_stream_id = 0;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_M2V:
	{
		_stream_coding_type = PAMF_STREAM_CODING_TYPE_M2V;
		_stream_id = 0xe0 | ch;
		_private_stream_id = 0;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_ATRAC3PLUS:
	{
		_stream_coding_type = PAMF_STREAM_CODING_TYPE_ATRAC3PLUS;
		_stream_id = 0xbd;
		_private_stream_id = ch;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_PAMF_LPCM:
	{
		_stream_coding_type = PAMF_STREAM_CODING_TYPE_PAMF_LPCM;
		_stream_id = 0xbd;
		_private_stream_id = 0x40 | ch;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_AC3:
	{
		_stream_coding_type = PAMF_STREAM_CODING_TYPE_AC3;
		_stream_id = 0xbd;
		_private_stream_id = 0x30 | ch;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_USER_DATA:
	{
		_stream_coding_type = PAMF_STREAM_CODING_TYPE_USER_DATA;
		_stream_id = 0xbd;
		_private_stream_id = 0x20 | ch;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_PSMF_AVC:
	{
		_stream_coding_type = PAMF_STREAM_CODING_TYPE_PSMF;
		_stream_id = 0xe0 | ch;
		_private_stream_id = 0;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_PSMF_ATRAC3PLUS:
	{
		_stream_coding_type = PAMF_STREAM_CODING_TYPE_PSMF;
		_stream_id = 0xbd;
		_private_stream_id = ch;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_PSMF_LPCM:
	{
		_stream_coding_type = PAMF_STREAM_CODING_TYPE_PSMF;
		_stream_id = 0xbd;
		_private_stream_id = 0x10 | ch;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_PSMF_USER_DATA:
	{
		_stream_coding_type = PAMF_STREAM_CODING_TYPE_PSMF;
		_stream_id = 0xbd;
		_private_stream_id = 0x20 | ch;
		break;
	}

	default:
	{
		cellPamf.error("pamfTypeChannelToStream(): unknown type %d", type);
		return CELL_PAMF_ERROR_INVALID_ARG;
	}
	}

	if (stream_coding_type)
	{
		*stream_coding_type = _stream_coding_type;
	}

	if (stream_id)
	{
		*stream_id = _stream_id;
	}

	if (private_stream_id)
	{
		*private_stream_id = _private_stream_id;
	}

	return CELL_OK;
}

error_code pamfStreamToTypeChannel(u8 stream_coding_type, u8 stream_id, u8 private_stream_id, vm::ptr<u8> type, vm::ptr<u8> ch)
{
	u8 _type;
	u8 _ch;

	switch (stream_coding_type)
	{
	case PAMF_STREAM_CODING_TYPE_AVC:
		_type = CELL_PAMF_STREAM_TYPE_AVC;
		_ch = stream_id & 0x0f;
		break;

	case PAMF_STREAM_CODING_TYPE_M2V:
		_type = CELL_PAMF_STREAM_TYPE_M2V;
		_ch = stream_id & 0x0f;
		break;

	case PAMF_STREAM_CODING_TYPE_ATRAC3PLUS:
		_type = CELL_PAMF_STREAM_TYPE_ATRAC3PLUS;
		_ch = private_stream_id & 0x0f;
		break;

	case PAMF_STREAM_CODING_TYPE_PAMF_LPCM:
		_type = CELL_PAMF_STREAM_TYPE_PAMF_LPCM;
		_ch = private_stream_id & 0x0f;
		break;

	case PAMF_STREAM_CODING_TYPE_AC3:
		_type = CELL_PAMF_STREAM_TYPE_AC3;
		_ch = private_stream_id & 0x0f;
		break;

	case PAMF_STREAM_CODING_TYPE_USER_DATA:
		_type = CELL_PAMF_STREAM_TYPE_USER_DATA;
		_ch = private_stream_id & 0x0f;
		break;

	case PAMF_STREAM_CODING_TYPE_PSMF:
		if ((stream_id & 0xf0) == 0xe0)
		{
			_type = CELL_PAMF_STREAM_TYPE_PSMF_AVC;
			_ch = stream_id & 0x0f;

			if (private_stream_id != 0)
			{
				return CELL_PAMF_ERROR_STREAM_NOT_FOUND;
			}
		}
		else if (stream_id == 0xbd)
		{
			_ch = private_stream_id & 0x0f;

			switch (private_stream_id & 0xf0)
			{
			case 0x00: _type = CELL_PAMF_STREAM_TYPE_PSMF_ATRAC3PLUS; break;
			case 0x10: _type = CELL_PAMF_STREAM_TYPE_PSMF_LPCM; break;
			case 0x20: _type = CELL_PAMF_STREAM_TYPE_PSMF_LPCM; break; // LLE doesn't use CELL_PAMF_STREAM_TYPE_PSMF_USER_DATA for some reason
			default: return CELL_PAMF_ERROR_STREAM_NOT_FOUND;
			}
		}
		else
		{
			return CELL_PAMF_ERROR_STREAM_NOT_FOUND;
		}
		break;

	default:
		cellPamf.error("pamfStreamToTypeChannel(): unknown stream_coding_type 0x%02x", stream_coding_type);
		return CELL_PAMF_ERROR_STREAM_NOT_FOUND;
	}

	if (type)
	{
		*type = _type;
	}

	if (ch)
	{
		*ch = _ch;
	}

	return CELL_OK;
}

void pamfEpUnpack(vm::cptr<PamfEpHeader> ep_packed, vm::ptr<CellPamfEp> ep)
{
	ensure(!!ep_packed && !!ep); // Not checked on LLE

	ep->indexN = (ep_packed->value0 >> 14) + 1;
	ep->nThRefPictureOffset = ((ep_packed->value0 & 0x1fff) * 0x800) + 0x800;
	ep->pts.upper = ep_packed->pts_high;
	ep->pts.lower = ep_packed->pts_low;
	ep->rpnOffset = ep_packed->rpnOffset * 0x800ull;
}

void psmfEpUnpack(vm::cptr<PsmfEpHeader> ep_packed, vm::ptr<CellPamfEp> ep)
{
	ensure(!!ep_packed && !!ep); // Not checked on LLE

	ep->indexN = (ep_packed->value0 >> 14) + 1;
	ep->nThRefPictureOffset = ((ep_packed->value0 & 0xffe) * 0x400) + 0x800;
	ep->pts.upper = ep_packed->value0 & 1;
	ep->pts.lower = ep_packed->pts_low;
	ep->rpnOffset = ep_packed->rpnOffset * 0x800ull;
}

bool pamfIsSameStreamType(u8 type, u8 requested_type)
{
	switch (requested_type)
	{
	case CELL_PAMF_STREAM_TYPE_VIDEO: return type == CELL_PAMF_STREAM_TYPE_AVC || type == CELL_PAMF_STREAM_TYPE_M2V;
	case CELL_PAMF_STREAM_TYPE_AUDIO: return type == CELL_PAMF_STREAM_TYPE_ATRAC3PLUS || type == CELL_PAMF_STREAM_TYPE_AC3 || type == CELL_PAMF_STREAM_TYPE_PAMF_LPCM;
	case CELL_PAMF_STREAM_TYPE_UNK:   return type == CELL_PAMF_STREAM_TYPE_PAMF_LPCM || type == CELL_PAMF_STREAM_TYPE_PSMF_ATRAC3PLUS; // ??? no idea what this is for
	default:                          return requested_type == type;
	}
}

error_code pamfVerify(vm::cptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<CellPamfReader> pSelf, u32 attribute)
{
	ensure(!!pAddr); // Not checked on LLE

	if (error_code ret = pamfVerifyMagicAndVersion(pAddr, pSelf); ret != CELL_OK)
	{
		return ret;
	}

	const u64 header_size = pAddr->header_size * 0x800ull;
	const u64 data_size = pAddr->data_size * 0x800ull;

	// Header size
	if (header_size == 0)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid header_size" };
	}

	if (pSelf)
	{
		pSelf->headerSize = header_size;
		pSelf->dataSize = data_size;
	}

	// Data size
	if (fileSize != 0 && header_size + data_size != fileSize)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: fileSize isn't equal header_size + data_size" };
	}

	const u32 psmf_marks_offset = pAddr->psmf_marks_offset;
	const u32 psmf_marks_size = pAddr->psmf_marks_size;
	const u32 unk_offset = pAddr->unk_offset;
	const u32 unk_size = pAddr->unk_size;

	// PsmfMarks
	if (psmf_marks_offset == 0)
	{
		if (psmf_marks_size != 0)
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: psmf_marks_offset is zero but psmf_marks_size is not zero" };
		}
	}
	else
	{
		if (psmf_marks_size == 0)
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: psmf_marks_offset is set but psmf_marks_size is zero" };
		}

		if (header_size < static_cast<u64>(psmf_marks_offset) + psmf_marks_size)
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: header_size is less than psmf_marks_offset + psmf_marks_size" };
		}
	}

	if (unk_offset == 0)
	{
		if (unk_size != 0)
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: unk_offset is zero but unk_size is not zero" };
		}
	}
	else
	{
		if (unk_size == 0)
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: unk_offset is set but unk_size is zero" };
		}

		if (header_size < static_cast<u64>(unk_offset) + unk_size)
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: header_size is less than unk_offset + unk_size" };
		}
	}

	if (unk_offset < static_cast<u64>(psmf_marks_offset) + psmf_marks_size)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: unk_offset is less than psmf_marks_offset + psmf_marks_size" };
	}


	// Sequence Info

	const u32 seq_info_size = pAddr->seq_info.size;

	// Sequence info size
	if (offsetof(PamfHeader, seq_info) + sizeof(u32) + seq_info_size > header_size)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid seq_info_size" };
	}

	const u64 start_pts = static_cast<u64>(pAddr->seq_info.start_pts_high) << 32 | pAddr->seq_info.start_pts_low;
	const u64 end_pts = static_cast<u64>(pAddr->seq_info.end_pts_high) << 32 | pAddr->seq_info.end_pts_low;

	// Start and end presentation time stamps
	if (end_pts > CODEC_TS_INVALID)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid end_pts" };
	}

	if (start_pts >= end_pts)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid start_pts" };
	}

	// Grouping period count
	if (pAddr->seq_info.grouping_period_num != 1)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid grouping_period_num" };
	}


	// Grouping Period

	const u32 grouping_period_size = pAddr->seq_info.grouping_periods.size;

	// Grouping period size
	if (offsetof(PamfHeader, seq_info.grouping_periods) + sizeof(u32) + grouping_period_size > header_size)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid grouping_period_size" };
	}

	const u64 grp_period_start_pts = static_cast<u64>(pAddr->seq_info.grouping_periods.start_pts_high) << 32 | pAddr->seq_info.grouping_periods.start_pts_low;
	const u64 grp_period_end_pts = static_cast<u64>(pAddr->seq_info.grouping_periods.start_pts_high) << 32 | pAddr->seq_info.grouping_periods.end_pts_low; // LLE uses start_pts_high due to a bug

	// Start and end presentation time stamps
	if (grp_period_end_pts > CODEC_TS_INVALID)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid grp_period_end_pts" };
	}

	if (grp_period_start_pts >= grp_period_end_pts)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid grp_period_start_pts" };
	}

	if (grp_period_start_pts != start_pts)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: grp_period_start_pts not equal start_pts" };
	}

	// Group count
	if (pAddr->seq_info.grouping_periods.group_num != 1)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid group_num" };
	}


	// Group

	const u32 group_size = pAddr->seq_info.grouping_periods.groups.size;

	// StreamGroup size
	if (offsetof(PamfHeader, seq_info.grouping_periods.groups) + sizeof(u32) + group_size > header_size)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid group_size" };
	}

	const u8 stream_num = pAddr->seq_info.grouping_periods.groups.stream_num;

	// Stream count
	if (stream_num == 0)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid stream_num" };
	}


	// Streams

	const auto streams = &pAddr->seq_info.grouping_periods.groups.streams;

	std::bitset<16> channels_used[6]{};

	u32 end_of_streams_addr = 0;
	u32 next_ep_table_addr = 0;

	for (u8 stream_idx = 0; stream_idx < stream_num; stream_idx++)
	{
		vm::var<u8> type;
		vm::var<u8> ch;

		// Stream coding type and IDs
		if (pamfStreamToTypeChannel(streams[stream_idx].stream_coding_type, streams[stream_idx].stream_id, streams[stream_idx].private_stream_id, type, ch) != CELL_OK)
		{
			return CELL_PAMF_ERROR_UNKNOWN_STREAM;
		}

		// Every channel may only be used once per type
		if (channels_used[*type].test(*ch))
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid channel" };
		}

		// Mark channel as used
		channels_used[*type].set(*ch);

		const u32 ep_offset = streams[stream_idx].ep_offset;
		const u32 ep_num = streams[stream_idx].ep_num;

		// Entry point offset and number
		if (ep_num == 0)
		{
			if (ep_offset != 0)
			{
				return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: ep_num is zero but ep_offset is not zero" };
			}
		}
		else
		{
			if (ep_offset == 0)
			{
				return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid ep_offset" };
			}

			if (ep_offset + ep_num * sizeof(PamfEpHeader) > header_size)
			{
				return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid ep_num" };
			}
		}


		// Entry points

		// Skip if there are no entry points or if the minimum header attribute is set
		if (ep_offset == 0 || attribute & CELL_PAMF_ATTRIBUTE_MINIMUM_HEADER)
		{
			continue;
		}

		const auto eps = vm::cptr<PamfEpHeader>::make(pAddr.addr() + ep_offset);

		// Entry point tables must be sorted by the stream index to which they belong
		// and there mustn't be any gaps between them
		if (end_of_streams_addr == 0)
		{
			end_of_streams_addr = eps.addr();
			next_ep_table_addr = end_of_streams_addr;
		}
		else if (next_ep_table_addr != eps.addr())
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid ep table address" };
		}

		u64 previous_rpn_offset = 0;
		for (u32 ep_idx = 0; ep_idx < ep_num; ep_idx++)
		{
			const u64 pts = static_cast<u64>(eps[ep_idx].pts_high) << 32 | eps[ep_idx].pts_low;

			// Entry point time stamp
			if (pts > CODEC_TS_INVALID)
			{
				return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid ep pts" };
			}

			const u64 rpn_offset = eps[ep_idx].rpnOffset * 0x800ull;

			// Entry point rpnOffset
			if (rpn_offset > data_size || rpn_offset < previous_rpn_offset)
			{
				return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid rpn_offset" };
			}

			previous_rpn_offset = rpn_offset;
		}

		next_ep_table_addr += ep_num * sizeof(PamfEpHeader);
	}

	// This can overflow on LLE, the +4 is necessary on both sides and the left operand needs to be u32
	if (group_size + 4 > grouping_period_size - offsetof(PamfGroupingPeriod, groups) + sizeof(u32)
		|| grouping_period_size + 4 > seq_info_size - offsetof(PamfSequenceInfo, grouping_periods) + sizeof(u32))
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: size mismatch" };
	}

	// Since multiple grouping periods/groups was never implemented, number of streams in SequenceInfo must be equal stream_num in Group
	if (pAddr->seq_info.total_stream_num != stream_num)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: number of streams mismatch" };
	}


	// PsmfMarks
	// This is probably useless since the official PAMF tools don't support PsmfMarks

	if (end_of_streams_addr == 0)
	{
		if (psmf_marks_offset != 0)
		{
			end_of_streams_addr = pAddr.addr() + psmf_marks_offset;
		}
		else if (unk_offset != 0)
		{
			end_of_streams_addr = pAddr.addr() + unk_offset;
		}
	}

	if (end_of_streams_addr != 0 && pAddr.addr() + offsetof(PamfHeader, seq_info.grouping_periods) + sizeof(u32) + grouping_period_size != end_of_streams_addr)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid offset of ep tables or psmf marks" };
	}

	if (next_ep_table_addr != 0 && psmf_marks_offset == 0)
	{
		if (unk_offset != 0 && pAddr.addr() + unk_offset != next_ep_table_addr)
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid unk_offset" };
		}
	}
	else if (next_ep_table_addr != 0 && pAddr.addr() + psmf_marks_offset != next_ep_table_addr)
	{
		return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid psmf_marks_offset" };
	}
	else if (psmf_marks_offset != 0 && !(attribute & CELL_PAMF_ATTRIBUTE_MINIMUM_HEADER))
	{
		const u32 size = vm::read32(pAddr.addr() + psmf_marks_offset);

		if (size + sizeof(u32) != psmf_marks_size)
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid psmf_marks_size" };
		}

		const u16 marks_num = vm::read16(pAddr.addr() + psmf_marks_offset + 6); // LLE uses the wrong offset (6 instead of 4)

		if (sizeof(u16) + marks_num * 0x28 /*sizeof PsmfMark*/ != size)
		{
			return { CELL_PAMF_ERROR_INVALID_PAMF, "pamfVerify() failed: invalid marks_num" };
		}

		// There are more checks in LLE but due to the bug above these would never be executed
	}

	return CELL_OK;
}

error_code cellPamfGetStreamOffsetAndSize(vm::ptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<u64> pOffset, vm::ptr<u64> pSize);

error_code cellPamfGetHeaderSize(vm::ptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<u64> pSize)
{
	cellPamf.notice("cellPamfGetHeaderSize(pAddr=*0x%x, fileSize=0x%llx, pSize=*0x%x)", pAddr, fileSize, pSize);

	return cellPamfGetStreamOffsetAndSize(pAddr, fileSize, pSize, vm::null);
}

error_code cellPamfGetHeaderSize2(vm::ptr<PamfHeader> pAddr, u64 fileSize, u32 attribute, vm::ptr<u64> pSize)
{
	cellPamf.notice("cellPamfGetHeaderSize2(pAddr=*0x%x, fileSize=0x%llx, attribute=0x%x, pSize=*0x%x)", pAddr, fileSize, attribute, pSize);

	const vm::var<u64> header_size;

	if (error_code ret = cellPamfGetStreamOffsetAndSize(pAddr, fileSize, header_size, vm::null); ret != CELL_OK)
	{
		return ret;
	}

	if (attribute & CELL_PAMF_ATTRIBUTE_MINIMUM_HEADER)
	{
		if (pAddr->magic != std::bit_cast<be_t<u32>>("PAMF"_u32)) // Already checked in cellPamfGetStreamOffsetAndSize(), should always evaluate to false at this point
		{
			return CELL_PAMF_ERROR_UNKNOWN_TYPE;
		}

		const u64 min_header_size = offsetof(PamfHeader, seq_info) + sizeof(u32) + pAddr->seq_info.size; // Size without EP tables

		if (min_header_size > *header_size)
		{
			return CELL_PAMF_ERROR_INVALID_PAMF;
		}

		*header_size = min_header_size;
	}

	if (pSize)
	{
		*pSize = *header_size;
	}

	return CELL_OK;
}

error_code cellPamfGetStreamOffsetAndSize(vm::ptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<u64> pOffset, vm::ptr<u64> pSize)
{
	cellPamf.notice("cellPamfGetStreamOffsetAndSize(pAddr=*0x%x, fileSize=0x%llx, pOffset=*0x%x, pSize=*0x%x)", pAddr, fileSize, pOffset, pSize);

	return pamfGetHeaderAndDataSize(pAddr, fileSize, pOffset, pSize);
}

error_code cellPamfVerify(vm::cptr<PamfHeader> pAddr, u64 fileSize)
{
	cellPamf.notice("cellPamfVerify(pAddr=*0x%x, fileSize=0x%llx)", pAddr, fileSize);

	return pamfVerify(pAddr, fileSize, vm::null, CELL_PAMF_ATTRIBUTE_VERIFY_ON);
}

error_code cellPamfReaderSetStreamWithIndex(vm::ptr<CellPamfReader> pSelf, u8 streamIndex);

error_code cellPamfReaderInitialize(vm::ptr<CellPamfReader> pSelf, vm::cptr<PamfHeader> pAddr, u64 fileSize, u32 attribute)
{
	cellPamf.notice("cellPamfReaderInitialize(pSelf=*0x%x, pAddr=*0x%x, fileSize=0x%llx, attribute=0x%x)", pSelf, pAddr, fileSize, attribute);

	ensure(!!pSelf); // Not checked on LLE

	std::memset(pSelf.get_ptr(), 0, sizeof(CellPamfReader));

	pSelf->attribute = attribute;

	if (attribute & CELL_PAMF_ATTRIBUTE_VERIFY_ON)
	{
		if (error_code ret = pamfVerify(pAddr, fileSize, pSelf, attribute); ret != CELL_OK)
		{
			return ret;
		}
	}

	pSelf->pamf.header = pAddr;
	pSelf->pamf.sequenceInfo = pAddr.ptr(&PamfHeader::seq_info);

	pSelf->currentGroupingPeriodIndex = -1;
	pSelf->currentGroupIndex = -1;
	pSelf->currentStreamIndex = -1;

	if (pAddr->seq_info.grouping_period_num == 0)
	{
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	pSelf->currentGroupingPeriodIndex = 0;
	pSelf->pamf.currentGroupingPeriod = pSelf->pamf.sequenceInfo.ptr(&PamfSequenceInfo::grouping_periods);

	if (pAddr->seq_info.grouping_periods.group_num != 0)
	{
		pSelf->currentGroupIndex = 0;
		pSelf->pamf.currentGroup = pSelf->pamf.currentGroupingPeriod.ptr(&PamfGroupingPeriod::groups);

		cellPamfReaderSetStreamWithIndex(pSelf, 0);
	}

	return CELL_OK;
}

error_code cellPamfReaderGetPresentationStartTime(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp)
{
	cellPamf.notice("cellPamfReaderGetPresentationStartTime(pSelf=*0x%x, pTimeStamp=*0x%x)", pSelf, pTimeStamp);

	// always returns CELL_OK

	ensure(!!pSelf && !!pTimeStamp); // Not checked on LLE

	if (pSelf->isPsmf)
	{
		pTimeStamp->upper = pSelf->psmf.sequenceInfo->start_pts_high;
		pTimeStamp->lower = pSelf->psmf.sequenceInfo->start_pts_low;
	}
	else
	{
		pTimeStamp->upper = pSelf->pamf.sequenceInfo->start_pts_high;
		pTimeStamp->lower = pSelf->pamf.sequenceInfo->start_pts_low;
	}

	return CELL_OK;
}

error_code cellPamfReaderGetPresentationEndTime(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp)
{
	cellPamf.notice("cellPamfReaderGetPresentationEndTime(pSelf=*0x%x, pTimeStamp=*0x%x)", pSelf, pTimeStamp);

	// always returns CELL_OK

	ensure(!!pSelf && !!pTimeStamp); // Not checked on LLE

	if (pSelf->isPsmf)
	{
		pTimeStamp->upper = pSelf->psmf.sequenceInfo->end_pts_high;
		pTimeStamp->lower = pSelf->psmf.sequenceInfo->end_pts_low;
	}
	else
	{
		pTimeStamp->upper = pSelf->pamf.sequenceInfo->end_pts_high;
		pTimeStamp->lower = pSelf->pamf.sequenceInfo->end_pts_low;
	}

	return CELL_OK;
}

u32 cellPamfReaderGetMuxRateBound(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf.notice("cellPamfReaderGetMuxRateBound(pSelf=*0x%x)", pSelf);

	ensure(!!pSelf); // Not checked on LLE

	if (pSelf->isPsmf)
	{
		return pSelf->psmf.sequenceInfo->mux_rate_bound & 0x003fffff;
	}

	return pSelf->pamf.sequenceInfo->mux_rate_bound & 0x003fffff;
}

u8 cellPamfReaderGetNumberOfStreams(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf.notice("cellPamfReaderGetNumberOfStreams(pSelf=*0x%x)", pSelf);

	ensure(!!pSelf); // Not checked on LLE

	return pSelf->pamf.currentGroup->stream_num;
}

u8 cellPamfReaderGetNumberOfSpecificStreams(vm::ptr<CellPamfReader> pSelf, u8 streamType)
{
	cellPamf.notice("cellPamfReaderGetNumberOfSpecificStreams(pSelf=*0x%x, streamType=%d)", pSelf, streamType);

	ensure(!!pSelf); // Not checked on LLE

	const vm::var<u8> type;
	u8 found = 0;

	if (pSelf->isPsmf)
	{
		const auto streams = pSelf->psmf.currentGroup.ptr(&PsmfGroup::streams);

		for (u8 i = 0; i < pSelf->psmf.currentGroup->stream_num; i++)
		{
			if (pamfStreamToTypeChannel(PAMF_STREAM_CODING_TYPE_PSMF, streams[i].stream_id, streams[i].private_stream_id, type, vm::null) == CELL_OK)
			{
				found += pamfIsSameStreamType(*type, streamType);
			}
		}
	}
	else
	{
		const auto streams = pSelf->pamf.currentGroup.ptr(&PamfGroup::streams);

		for (u8 i = 0; i < pSelf->pamf.currentGroup->stream_num; i++)
		{
			if (pamfStreamToTypeChannel(streams[i].stream_coding_type, streams[i].stream_id, streams[i].private_stream_id, type, vm::null) == CELL_OK)
			{
				found += pamfIsSameStreamType(*type, streamType);
			}
		}
	}

	return found;
}

error_code cellPamfReaderSetStreamWithIndex(vm::ptr<CellPamfReader> pSelf, u8 streamIndex)
{
	cellPamf.notice("cellPamfReaderSetStreamWithIndex(pSelf=*0x%x, streamIndex=%d)", pSelf, streamIndex);

	ensure(!!pSelf); // Not checked on LLE

	if (streamIndex >= pSelf->pamf.currentGroup->stream_num)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	pSelf->currentStreamIndex = streamIndex;

	if (pSelf->isPsmf)
	{
		pSelf->psmf.currentStream = pSelf->psmf.currentGroup.ptr(&PsmfGroup::streams) + streamIndex;
	}
	else
	{
		pSelf->pamf.currentStream = pSelf->pamf.currentGroup.ptr(&PamfGroup::streams) + streamIndex;
	}

	return CELL_OK;
}

error_code cellPamfReaderSetStreamWithTypeAndChannel(vm::ptr<CellPamfReader> pSelf, u8 streamType, u8 ch)
{
	cellPamf.notice("cellPamfReaderSetStreamWithTypeAndChannel(pSelf=*0x%x, streamType=%d, ch=%d)", pSelf, streamType, ch);

	// This function is broken on LLE

	ensure(!!pSelf); // Not checked on LLE

	u8 stream_coding_type;
	u8 stream_id;
	u8 private_stream_id;

	if (pamfTypeChannelToStream(streamType, ch, &stream_coding_type, &stream_id, &private_stream_id) != CELL_OK)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	const u8 stream_num = pSelf->pamf.currentGroup->stream_num;
	u32 i = 0;

	if (pSelf->isPsmf)
	{
		const auto streams = pSelf->psmf.currentGroup.ptr(&PsmfGroup::streams);

		for (; i < stream_num; i++)
		{
			// LLE increments the index by 12 instead of 1
			if (stream_coding_type == PAMF_STREAM_CODING_TYPE_PSMF && streams[i * 12].stream_id == stream_id && streams[i * 12].private_stream_id == private_stream_id)
			{
				break;
			}
		}
	}
	else
	{
		const auto streams = pSelf->pamf.currentGroup.ptr(&PamfGroup::streams);

		for (; i < stream_num; i++)
		{
			// LLE increments the index by 0x10 instead of 1
			if (streams[i * 0x10].stream_coding_type == stream_coding_type && streams[i * 0x10].stream_id == stream_id && streams[i * 0x10].private_stream_id == private_stream_id)
			{
				break;
			}
		}
	}

	if (i == stream_num)
	{
		i = CELL_PAMF_ERROR_STREAM_NOT_FOUND; // LLE writes the error code to the index
	}

	if (pSelf->currentStreamIndex != i)
	{
		pSelf->pamf.currentStream = pSelf->pamf.currentGroup.ptr(&PamfGroup::streams); // LLE always sets this to the first stream
		pSelf->currentStreamIndex = i;
	}

	if (i == CELL_PAMF_ERROR_STREAM_NOT_FOUND)
	{
		return CELL_PAMF_ERROR_STREAM_NOT_FOUND;
	}

	return not_an_error(i);
}

error_code cellPamfReaderSetStreamWithTypeAndIndex(vm::ptr<CellPamfReader> pSelf, u8 streamType, u8 streamIndex)
{
	cellPamf.notice("cellPamfReaderSetStreamWithTypeAndIndex(pSelf=*0x%x, streamType=%d, streamIndex=%d)", pSelf, streamType, streamIndex);

	ensure(!!pSelf); // Not checked on LLE

	const u8 stream_num = pSelf->pamf.currentGroup->stream_num;

	if (streamIndex >= stream_num)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	const vm::var<u8> type;
	u32 found = 0;

	if (pSelf->isPsmf)
	{
		const auto streams = pSelf->psmf.currentGroup.ptr(&PsmfGroup::streams);

		for (u8 i = 0; i < stream_num; i++)
		{
			if (pamfStreamToTypeChannel(PAMF_STREAM_CODING_TYPE_PSMF, streams[i].stream_id, streams[i].private_stream_id, type, vm::null) != CELL_OK)
			{
				continue;
			}

			found += *type == streamType;

			if (found > streamIndex)
			{
				pSelf->currentStreamIndex = streamIndex; // LLE sets this to the index counting only streams of the requested type instead of the overall index
				pSelf->psmf.currentStream = streams; // LLE always sets this to the first stream
				return not_an_error(i);
			}
		}
	}
	else
	{
		const auto streams = pSelf->pamf.currentGroup.ptr(&PamfGroup::streams);

		for (u8 i = 0; i < stream_num; i++)
		{
			if (pamfStreamToTypeChannel(streams[i].stream_coding_type, streams[i].stream_id, streams[i].private_stream_id, type, vm::null) != CELL_OK)
			{
				continue;
			}

			found += pamfIsSameStreamType(*type, streamType);

			if (found > streamIndex)
			{
				pSelf->currentStreamIndex = i;
				pSelf->pamf.currentStream = streams + i;
				return not_an_error(i);
			}
		}
	}

	return CELL_PAMF_ERROR_STREAM_NOT_FOUND;
}

error_code cellPamfStreamTypeToEsFilterId(u8 type, u8 ch, vm::ptr<CellCodecEsFilterId> pEsFilterId)
{
	cellPamf.notice("cellPamfStreamTypeToEsFilterId(type=%d, ch=%d, pEsFilterId=*0x%x)", type, ch, pEsFilterId);

	if (!pEsFilterId)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	u8 stream_id = 0;
	u8 private_stream_id = 0;

	if (pamfTypeChannelToStream(type, ch, nullptr, &stream_id, &private_stream_id) != CELL_OK)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	pEsFilterId->filterIdMajor = stream_id;
	pEsFilterId->filterIdMinor = private_stream_id;
	pEsFilterId->supplementalInfo1 = type == CELL_PAMF_STREAM_TYPE_AVC;
	pEsFilterId->supplementalInfo2 = 0;

	return CELL_OK;
}

s32 cellPamfReaderGetStreamIndex(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf.notice("cellPamfReaderGetStreamIndex(pSelf=*0x%x)", pSelf);

	ensure(!!pSelf); // Not checked on LLE

	return pSelf->currentStreamIndex;
}

error_code cellPamfReaderGetStreamTypeAndChannel(vm::ptr<CellPamfReader> pSelf, vm::ptr<u8> pType, vm::ptr<u8> pCh)
{
	cellPamf.notice("cellPamfReaderGetStreamTypeAndChannel(pSelf=*0x%x, pType=*0x%x, pCh=*0x%x", pSelf, pType, pCh);

	ensure(!!pSelf); // Not checked on LLE

	if (pSelf->isPsmf)
	{
		const auto stream = pSelf->psmf.currentStream;
		return pamfStreamToTypeChannel(PAMF_STREAM_CODING_TYPE_PSMF, stream->stream_id, stream->private_stream_id, pType, pCh);
	}
	else
	{
		const auto stream = pSelf->pamf.currentStream;
		return pamfStreamToTypeChannel(stream->stream_coding_type, stream->stream_id, stream->private_stream_id, pType, pCh);
	}
}

error_code cellPamfReaderGetEsFilterId(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecEsFilterId> pEsFilterId)
{
	cellPamf.notice("cellPamfReaderGetEsFilterId(pSelf=*0x%x, pEsFilterId=*0x%x)", pSelf, pEsFilterId);

	// always returns CELL_OK

	ensure(!!pSelf && !!pEsFilterId); // Not checked on LLE

	if (pSelf->isPsmf)
	{
		pEsFilterId->filterIdMajor = pSelf->psmf.currentStream->stream_id;
		pEsFilterId->filterIdMinor = pSelf->psmf.currentStream->private_stream_id;
		pEsFilterId->supplementalInfo1 = 0;
	}
	else
	{
		pEsFilterId->filterIdMajor = pSelf->pamf.currentStream->stream_id;
		pEsFilterId->filterIdMinor = pSelf->pamf.currentStream->private_stream_id;
		pEsFilterId->supplementalInfo1 = pSelf->pamf.currentStream->stream_coding_type == PAMF_STREAM_CODING_TYPE_AVC;
	}

	pEsFilterId->supplementalInfo2 = 0;
	return CELL_OK;
}

error_code cellPamfReaderGetStreamInfo(vm::ptr<CellPamfReader> pSelf, vm::ptr<void> pInfo, u32 size)
{
	cellPamf.notice("cellPamfReaderGetStreamInfo(pSelf=*0x%x, pInfo=*0x%x, size=%d)", pSelf, pInfo, size);

	ensure(!!pSelf); // Not checked on LLE

	const auto& header = *pSelf->pamf.currentStream;
	const auto& psmf_header = *pSelf->psmf.currentStream;
	const vm::var<u8> type;
	error_code ret;

	if (pSelf->isPsmf)
	{
		ret = pamfStreamToTypeChannel(PAMF_STREAM_CODING_TYPE_PSMF, psmf_header.stream_id, psmf_header.private_stream_id, type, vm::null);
	}
	else
	{
		ret = pamfStreamToTypeChannel(header.stream_coding_type, header.stream_id, header.private_stream_id, type, vm::null);
	}

	if (ret != CELL_OK)
	{
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	switch (*type)
	{
	case CELL_PAMF_STREAM_TYPE_AVC:
	{
		if (size < sizeof(CellPamfAvcInfo))
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		auto info = vm::static_ptr_cast<CellPamfAvcInfo>(pInfo);

		info->profileIdc = header.AVC.profileIdc;
		info->levelIdc = header.AVC.levelIdc;
		info->frameMbsOnlyFlag = (header.AVC.x2 & 0x80) >> 7;
		info->videoSignalInfoFlag = (header.AVC.x2 & 0x40) >> 6;
		info->frameRateInfo = (header.AVC.x2 & 0x0f) - 1;
		info->aspectRatioIdc = header.AVC.aspectRatioIdc;

		if (header.AVC.aspectRatioIdc == 0xff)
		{
			info->sarWidth = header.AVC.sarWidth;
			info->sarHeight = header.AVC.sarHeight;
		}
		else
		{
			info->sarWidth = 0;
			info->sarHeight = 0;
		}

		info->horizontalSize = (header.AVC.horizontalSize & u8{0xff}) * 16;
		info->verticalSize = (header.AVC.verticalSize & u8{0xff}) * 16;
		info->frameCropLeftOffset = header.AVC.frameCropLeftOffset;
		info->frameCropRightOffset = header.AVC.frameCropRightOffset;
		info->frameCropTopOffset = header.AVC.frameCropTopOffset;
		info->frameCropBottomOffset = header.AVC.frameCropBottomOffset;

		if (info->videoSignalInfoFlag)
		{
			info->videoFormat = header.AVC.x14 >> 5;
			info->videoFullRangeFlag = (header.AVC.x14 & 0x10) >> 4;
			info->colourPrimaries = header.AVC.colourPrimaries;
			info->transferCharacteristics = header.AVC.transferCharacteristics;
			info->matrixCoefficients = header.AVC.matrixCoefficients;
		}
		else
		{
			info->videoFormat = 0;
			info->videoFullRangeFlag = 0;
			info->colourPrimaries = 0;
			info->transferCharacteristics = 0;
			info->matrixCoefficients = 0;
		}

		info->entropyCodingModeFlag = (header.AVC.x18 & 0x80) >> 7;
		info->deblockingFilterFlag = (header.AVC.x18 & 0x40) >> 6;
		info->minNumSlicePerPictureIdc = (header.AVC.x18 & 0x30) >> 4;
		info->nfwIdc = header.AVC.x18 & 0x03;
		info->maxMeanBitrate = header.AVC.maxMeanBitrate;

		cellPamf.notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_AVC");
		break;
	}

	case CELL_PAMF_STREAM_TYPE_M2V:
	{
		if (size < sizeof(CellPamfM2vInfo))
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		auto info = vm::static_ptr_cast<CellPamfM2vInfo>(pInfo);

		switch (header.M2V.x0)
		{
		case 0x44: info->profileAndLevelIndication = 3; break;
		case 0x48: info->profileAndLevelIndication = 1; break;
		default: info->profileAndLevelIndication = CELL_PAMF_M2V_UNKNOWN;
		}

		info->progressiveSequence = (header.M2V.x2 & 0x80) >> 7;
		info->videoSignalInfoFlag = (header.M2V.x2 & 0x40) >> 6;
		info->frameRateInfo = header.M2V.x2 & 0xf;
		info->aspectRatioIdc = header.M2V.aspectRatioIdc;

		if (header.M2V.aspectRatioIdc == 0xff)
		{
			info->sarWidth = header.M2V.sarWidth;
			info->sarHeight = header.M2V.sarHeight;
		}
		else
		{
			info->sarWidth = 0;
			info->sarHeight = 0;
		}

		info->horizontalSize = (header.M2V.horizontalSize & u8{0xff}) * 16;
		info->verticalSize = (header.M2V.verticalSize & u8{0xff}) * 16;
		info->horizontalSizeValue = header.M2V.horizontalSizeValue;
		info->verticalSizeValue = header.M2V.verticalSizeValue;

		if (info->videoSignalInfoFlag)
		{
			info->videoFormat = header.M2V.x14 >> 5;
			info->videoFullRangeFlag = (header.M2V.x14 & 0x10) >> 4;
			info->colourPrimaries = header.M2V.colourPrimaries;
			info->transferCharacteristics = header.M2V.transferCharacteristics;
			info->matrixCoefficients = header.M2V.matrixCoefficients;
		}
		else
		{
			info->videoFormat = 0;
			info->videoFullRangeFlag = 0;
			info->colourPrimaries = 0;
			info->transferCharacteristics = 0;
			info->matrixCoefficients = 0;
		}

		cellPamf.notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_M2V");
		break;
	}

	case CELL_PAMF_STREAM_TYPE_ATRAC3PLUS:
	{
		if (size < sizeof(CellPamfAtrac3plusInfo))
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		auto info = vm::static_ptr_cast<CellPamfAtrac3plusInfo>(pInfo);

		info->samplingFrequency = header.audio.freq & 0xf;
		info->numberOfChannels = header.audio.channels & 0xf;

		cellPamf.notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_ATRAC3PLUS");
		break;
	}

	case CELL_PAMF_STREAM_TYPE_PAMF_LPCM:
	{
		if (size < sizeof(CellPamfLpcmInfo))
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		auto info = vm::static_ptr_cast<CellPamfLpcmInfo>(pInfo);

		info->samplingFrequency = header.audio.freq & 0xf;
		info->numberOfChannels = header.audio.channels & 0xf;
		info->bitsPerSample = header.audio.bps >> 6;

		cellPamf.notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_PAMF_LPCM");
		break;
	}

	case CELL_PAMF_STREAM_TYPE_AC3:
	{
		if (size < sizeof(CellPamfAc3Info))
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		auto info = vm::static_ptr_cast<CellPamfAc3Info>(pInfo);

		info->samplingFrequency = header.audio.freq & 0xf;
		info->numberOfChannels = header.audio.channels & 0xf;

		cellPamf.notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_AC3");
		break;
	}

	case CELL_PAMF_STREAM_TYPE_USER_DATA:
	case CELL_PAMF_STREAM_TYPE_PSMF_USER_DATA:
	{
		cellPamf.error("cellPamfReaderGetStreamInfo(): invalid type CELL_PAMF_STREAM_TYPE_USER_DATA");
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	case CELL_PAMF_STREAM_TYPE_PSMF_AVC:
	{
		if (size < 4)
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		vm::static_ptr_cast<u16>(pInfo)[0] = psmf_header.video.horizontalSize * 0x10;
		vm::static_ptr_cast<u16>(pInfo)[1] = psmf_header.video.verticalSize * 0x10;

		cellPamf.notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_PSMF_AVC");
		break;
	}

	case CELL_PAMF_STREAM_TYPE_PSMF_ATRAC3PLUS:
	case CELL_PAMF_STREAM_TYPE_PSMF_LPCM:
	{
		if (size < 2)
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		vm::static_ptr_cast<u8>(pInfo)[0] = psmf_header.audio.channelConfiguration;
		vm::static_ptr_cast<u8>(pInfo)[1] = psmf_header.audio.samplingFrequency & 0x0f;

		cellPamf.notice("cellPamfReaderGetStreamInfo(): PSMF audio");
		break;
	}

	default:
	{
		// invalid type or getting type/ch failed
		cellPamf.error("cellPamfReaderGetStreamInfo(): invalid type %d", *type);
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}
	}

	return CELL_OK;
}

u32 cellPamfReaderGetNumberOfEp(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf.notice("cellPamfReaderGetNumberOfEp(pSelf=*0x%x)", pSelf);

	ensure(!!pSelf); // Not checked on LLE

	return pSelf->isPsmf ? pSelf->psmf.currentStream->ep_num : pSelf->pamf.currentStream->ep_num;
}

error_code cellPamfReaderGetEpIteratorWithIndex(vm::ptr<CellPamfReader> pSelf, u32 epIndex, vm::ptr<CellPamfEpIterator> pIt)
{
	cellPamf.notice("cellPamfReaderGetEpIteratorWithIndex(pSelf=*0x%x, epIndex=%d, pIt=*0x%x)", pSelf, epIndex, pIt);

	ensure(!!pSelf && !!pIt); // Not checked on LLE

	const u32 ep_num = cellPamfReaderGetNumberOfEp(pSelf);

	if (epIndex >= ep_num)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	if (pSelf->attribute & CELL_PAMF_ATTRIBUTE_MINIMUM_HEADER)
	{
		return CELL_PAMF_ERROR_NOT_AVAILABLE;
	}

	pIt->isPamf = !pSelf->isPsmf;
	pIt->index = epIndex;
	pIt->num = ep_num;
	pIt->pCur.set(pSelf->isPsmf ? pSelf->psmf.header.addr() + pSelf->psmf.currentStream->ep_offset + epIndex * sizeof(PsmfEpHeader)
		: pSelf->pamf.header.addr() + pSelf->pamf.currentStream->ep_offset + epIndex * sizeof(PamfEpHeader));

	return CELL_OK;
}

error_code cellPamfReaderGetEpIteratorWithTimeStamp(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp, vm::ptr<CellPamfEpIterator> pIt)
{
	cellPamf.notice("cellPamfReaderGetEpIteratorWithTimeStamp(pSelf=*0x%x, pTimeStamp=*0x%x, pIt=*0x%x)", pSelf, pTimeStamp, pIt);

	ensure(!!pSelf && !!pTimeStamp && !!pIt); // Not checked on LLE

	if (pSelf->attribute & CELL_PAMF_ATTRIBUTE_MINIMUM_HEADER)
	{
		return CELL_PAMF_ERROR_NOT_AVAILABLE;
	}

	const u32 ep_num = cellPamfReaderGetNumberOfEp(pSelf);

	pIt->num = ep_num;
	pIt->isPamf = !pSelf->isPsmf;

	if (ep_num == 0)
	{
		return CELL_PAMF_ERROR_EP_NOT_FOUND;
	}

	u32 i = ep_num - 1;
	const u64 requested_time_stamp = std::bit_cast<be_t<u64>>(*pTimeStamp);

	if (pSelf->isPsmf)
	{
		const auto eps = vm::cptr<PsmfEpHeader>::make(pSelf->psmf.header.addr() + pSelf->psmf.currentStream->ep_offset);

		for (; i >= 1; i--) // always output eps[0] if no other suitable ep is found
		{
			const u64 time_stamp = (static_cast<u64>(eps[i].value0 & 1) << 32) | eps[i].pts_low;

			if (time_stamp <= requested_time_stamp)
			{
				break;
			}
		}

		pIt->pCur = eps + i;
	}
	else
	{
		const auto eps = vm::cptr<PamfEpHeader>::make(pSelf->pamf.header.addr() + pSelf->pamf.currentStream->ep_offset);

		for (; i >= 1; i--) // always output eps[0] if no other suitable ep is found
		{
			const u64 time_stamp = (static_cast<u64>(eps[i].pts_high) << 32) | eps[i].pts_low;

			if (time_stamp <= requested_time_stamp)
			{
				break;
			}
		}

		pIt->pCur = eps + i;
	}

	pIt->index = i;

	return CELL_OK;
}

error_code cellPamfEpIteratorGetEp(vm::ptr<CellPamfEpIterator> pIt, vm::ptr<CellPamfEp> pEp)
{
	cellPamf.notice("cellPamfEpIteratorGetEp(pIt=*0x%x, pEp=*0x%x)", pIt, pEp);

	// always returns CELL_OK

	ensure(!!pIt); // Not checked on LLE

	if (pIt->isPamf)
	{
		pamfEpUnpack(vm::static_ptr_cast<const PamfEpHeader>(pIt->pCur), pEp);
	}
	else
	{
		psmfEpUnpack(vm::static_ptr_cast<const PsmfEpHeader>(pIt->pCur), pEp);
	}

	return CELL_OK;
}

s32 cellPamfEpIteratorMove(vm::ptr<CellPamfEpIterator> pIt, s32 steps, vm::ptr<CellPamfEp> pEp)
{
	cellPamf.notice("cellPamfEpIteratorMove(pIt=*0x%x, steps=%d, pEp=*0x%x)", pIt, steps, pEp);

	ensure(!!pIt); // Not checked on LLE

	u32 new_index = pIt->index + steps;

	if (static_cast<s32>(new_index) < 0)
	{
		steps = -static_cast<s32>(pIt->index);
		new_index = 0;
	}
	else if (new_index >= pIt->num)
	{
		steps = (pIt->num - pIt->index) - 1;
		new_index = pIt->index + steps;
	}

	pIt->index = new_index;

	if (pIt->isPamf)
	{
		pIt->pCur = vm::static_ptr_cast<const PamfEpHeader>(pIt->pCur) + steps;

		if (pEp)
		{
			pamfEpUnpack(vm::static_ptr_cast<const PamfEpHeader>(pIt->pCur), pEp);
		}
	}
	else
	{
		pIt->pCur = vm::static_ptr_cast<const PsmfEpHeader>(pIt->pCur) + steps;

		if (pEp)
		{
			psmfEpUnpack(vm::static_ptr_cast<const PsmfEpHeader>(pIt->pCur), pEp);
		}
	}

	return steps;
}

error_code cellPamfReaderGetEpWithTimeStamp(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp, vm::ptr<CellPamfEpUnk> pEp, u32 unk)
{
	cellPamf.notice("cellPamfReaderGetEpWithTimeStamp(pSelf=*0x%x, pTimeStamp=*0x%x, pEp=*0x%x, unk=0x%x)", pSelf, pTimeStamp, pEp, unk);

	// This function is broken on LLE

	ensure(!!pSelf && !!pTimeStamp && !!pEp); // Not checked on LLE

	if (pSelf->attribute & CELL_PAMF_ATTRIBUTE_MINIMUM_HEADER)
	{
		return CELL_PAMF_ERROR_NOT_AVAILABLE;
	}

	const u32 ep_num = cellPamfReaderGetNumberOfEp(pSelf);
	u64 next_rpn_offset = pSelf->dataSize;

	if (ep_num == 0)
	{
		return CELL_PAMF_ERROR_EP_NOT_FOUND;
	}

	u32 i = ep_num - 1;
	const u64 requested_time_stamp = std::bit_cast<be_t<u64>>(*pTimeStamp);

	if (pSelf->isPsmf)
	{
		const auto eps = vm::cptr<PsmfEpHeader>::make(pSelf->psmf.header.addr() + pSelf->psmf.currentStream->ep_offset);

		for (; i >= 1; i--) // always output eps[0] if no other suitable ep is found
		{
			const u64 time_stamp = (static_cast<u64>(eps[i].value0 & 1) << 32) | eps[i].pts_low;

			if (time_stamp <= requested_time_stamp)
			{
				break;
			}
		}

		// LLE doesn't write the result to pEp

		if (i < ep_num - 1)
		{
			next_rpn_offset = eps[i + 1].rpnOffset;
		}
	}
	else
	{
		const auto eps = vm::cptr<PamfEpHeader>::make(pSelf->pamf.header.addr() + pSelf->pamf.currentStream->ep_offset);

		for (; i >= 1; i--) // always output eps[0] if no other suitable ep is found
		{
			const u64 time_stamp = (static_cast<u64>(eps[i].pts_high) << 32) | eps[i].pts_low;

			if (time_stamp <= requested_time_stamp)
			{
				break;
			}
		}

		// LLE doesn't write the result to pEp

		if (i < ep_num - 1)
		{
			next_rpn_offset = eps[i + 1].rpnOffset;
		}
	}

	if (unk == sizeof(CellPamfEpUnk))
	{
		pEp->nextRpnOffset = next_rpn_offset;
	}

	return CELL_OK;
}

error_code cellPamfReaderGetEpWithIndex(vm::ptr<CellPamfReader> pSelf, u32 epIndex, vm::ptr<CellPamfEpUnk> pEp, u32 unk)
{
	cellPamf.notice("cellPamfReaderGetEpWithIndex(pSelf=*0x%x, epIndex=%d, pEp=*0x%x, unk=0x%x)", pSelf, epIndex, pEp, unk);

	ensure(!!pSelf && !!pEp); // Not checked on LLE

	const u32 ep_num = cellPamfReaderGetNumberOfEp(pSelf);
	u64 next_rpn_offset = pSelf->dataSize;

	if (epIndex >= ep_num)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	if (pSelf->attribute & CELL_PAMF_ATTRIBUTE_MINIMUM_HEADER)
	{
		return CELL_PAMF_ERROR_NOT_AVAILABLE;
	}

	if (pSelf->isPsmf)
	{
		const auto ep = vm::cptr<PsmfEpHeader>::make(pSelf->psmf.header.addr() + pSelf->psmf.currentStream->ep_offset + epIndex * sizeof(PsmfEpHeader));

		psmfEpUnpack(ep, pEp.ptr(&CellPamfEpUnk::ep));

		if (epIndex < ep_num - 1)
		{
			next_rpn_offset = ep[1].rpnOffset * 0x800ull;
		}
	}
	else
	{
		const auto ep = vm::cptr<PamfEpHeader>::make(pSelf->pamf.header.addr() + pSelf->pamf.currentStream->ep_offset + epIndex * sizeof(PamfEpHeader));

		pamfEpUnpack(ep, pEp.ptr(&CellPamfEpUnk::ep));

		if (epIndex < ep_num - 1)
		{
			next_rpn_offset = ep[1].rpnOffset * 0x800ull;
		}
	}

	if (unk == sizeof(CellPamfEpUnk))
	{
		pEp->nextRpnOffset = next_rpn_offset;
	}

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellPamf)("cellPamf", []()
{
	REG_FUNC(cellPamf, cellPamfGetHeaderSize);
	REG_FUNC(cellPamf, cellPamfGetHeaderSize2);
	REG_FUNC(cellPamf, cellPamfGetStreamOffsetAndSize);
	REG_FUNC(cellPamf, cellPamfVerify);
	REG_FUNC(cellPamf, cellPamfReaderInitialize);
	REG_FUNC(cellPamf, cellPamfReaderGetPresentationStartTime);
	REG_FUNC(cellPamf, cellPamfReaderGetPresentationEndTime);
	REG_FUNC(cellPamf, cellPamfReaderGetMuxRateBound);
	REG_FUNC(cellPamf, cellPamfReaderGetNumberOfStreams);
	REG_FUNC(cellPamf, cellPamfReaderGetNumberOfSpecificStreams);
	REG_FUNC(cellPamf, cellPamfReaderSetStreamWithIndex);
	REG_FUNC(cellPamf, cellPamfReaderSetStreamWithTypeAndChannel);
	REG_FUNC(cellPamf, cellPamfReaderSetStreamWithTypeAndIndex);
	REG_FUNC(cellPamf, cellPamfStreamTypeToEsFilterId);
	REG_FUNC(cellPamf, cellPamfReaderGetStreamIndex);
	REG_FUNC(cellPamf, cellPamfReaderGetStreamTypeAndChannel);
	REG_FUNC(cellPamf, cellPamfReaderGetEsFilterId);
	REG_FUNC(cellPamf, cellPamfReaderGetStreamInfo);
	REG_FUNC(cellPamf, cellPamfReaderGetNumberOfEp);
	REG_FUNC(cellPamf, cellPamfReaderGetEpIteratorWithIndex);
	REG_FUNC(cellPamf, cellPamfReaderGetEpIteratorWithTimeStamp);
	REG_FUNC(cellPamf, cellPamfEpIteratorGetEp);
	REG_FUNC(cellPamf, cellPamfEpIteratorMove);
	REG_FUNC(cellPamf, cellPamfReaderGetEpWithTimeStamp);
	REG_FUNC(cellPamf, cellPamfReaderGetEpWithIndex);
});
