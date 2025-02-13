#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/RSX/Overlays/overlay_media_list_dialog.h"
#include "Emu/VFS.h"
#include "cellMusicDecode.h"
#include "cellMusic.h"
#include "cellSearch.h"
#include "cellSpurs.h"
#include "cellSysutil.h"
#include "util/media_utils.h"

LOG_CHANNEL(cellMusicDecode);

template<>
void fmt_class_string<CellMusicDecodeError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MUSIC_DECODE_CANCELED);
			STR_CASE(CELL_MUSIC_DECODE_DECODE_FINISHED);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_PARAM);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_BUSY);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_NO_ACTIVE_CONTENT);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_NO_MATCH_FOUND);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_INVALID_CONTEXT);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_DECODE_FAILURE);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_NO_MORE_CONTENT);
			STR_CASE(CELL_MUSIC_DECODE_DIALOG_OPEN);
			STR_CASE(CELL_MUSIC_DECODE_DIALOG_CLOSE);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_NO_LPCM_DATA);
			STR_CASE(CELL_MUSIC_DECODE_NEXT_CONTENTS_READY);
			STR_CASE(CELL_MUSIC_DECODE_ERROR_GENERIC);
		}

		return unknown;
	});
}

struct music_decode
{
	vm::ptr<CellMusicDecodeCallback> func{};
	vm::ptr<void> userData{};
	music_selection_context current_selection_context{};
	s32 decode_status = CELL_MUSIC_DECODE_STATUS_DORMANT;
	s32 decode_command = CELL_MUSIC_DECODE_CMD_STOP;
	u64 read_pos = 0;
	utils::audio_decoder decoder{};

	shared_mutex mutex;

	error_code set_decode_command(s32 command)
	{
		decode_command = command;

		switch (command)
		{
		case CELL_MUSIC_DECODE_CMD_STOP:
		{
			decoder.stop();
			decode_status = CELL_MUSIC_DECODE_STATUS_DORMANT;
			break;
		}
		case CELL_MUSIC_DECODE_CMD_START:
		{
			decode_status = CELL_MUSIC_DECODE_STATUS_DECODING;
			read_pos = 0;

			// Decode data. The format of the decoded data is 48kHz, float 32bit, 2ch LPCM data interleaved in order from left to right.
			cellMusicDecode.notice("set_decode_command(START): context: %s", current_selection_context.to_string());

			music_selection_context context = current_selection_context;

			for (usz i = 0; i < context.playlist.size(); i++)
			{
				context.playlist[i] = vfs::get(context.playlist[i]);
			}

			// TODO: set speed if small-memory decoding is used (music_decode2)
			decoder.set_context(std::move(context));
			decoder.set_swap_endianness(true);
			decoder.decode();
			break;
		}
		case CELL_MUSIC_DECODE_CMD_NEXT:
		case CELL_MUSIC_DECODE_CMD_PREV:
		{
			decoder.stop();

			if (decoder.set_next_index(command == CELL_MUSIC_DECODE_CMD_NEXT) == umax)
			{
				decode_status = CELL_MUSIC_DECODE_STATUS_DORMANT;
				return CELL_MUSIC_DECODE_ERROR_NO_MORE_CONTENT;
			}

			decoder.decode();
			break;
		}
		default:
		{
			fmt::throw_exception("Unknown decode command %d", command);
		}
		}
		return CELL_OK;
	}

	error_code finalize()
	{
		decoder.stop();
		decode_status = CELL_MUSIC_DECODE_STATUS_DORMANT;
		decode_command = CELL_MUSIC_DECODE_CMD_STOP;
		read_pos = 0;
		return CELL_OK;
	}
};

struct music_decode2 : music_decode
{
	s32 speed = CELL_MUSIC_DECODE2_SPEED_MAX;
};

template <typename Music_Decode>
error_code cell_music_decode_select_contents()
{
	auto& dec = g_fxo->get<Music_Decode>();

	if (!dec.func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	const std::string vfs_dir_path = vfs::get("/dev_hdd0/music");
	const std::string title = get_localized_string(localized_string_id::RSX_OVERLAYS_MEDIA_DIALOG_TITLE);

	error_code error = rsx::overlays::show_media_list_dialog(rsx::overlays::media_list_dialog::media_type::audio, vfs_dir_path, title,
		[&dec](s32 status, utils::media_info info)
		{
			sysutil_register_cb([&dec, info = std::move(info), status](ppu_thread& ppu) -> s32
			{
				std::lock_guard lock(dec.mutex);
				const u32 result = status >= 0 ? u32{CELL_OK} : u32{CELL_MUSIC_DECODE_CANCELED};
				if (result == CELL_OK)
				{
					// Let's always choose the whole directory for now
					std::string track;
					std::string dir = info.path;
					if (fs::is_file(info.path))
					{
						track = std::move(dir);
						dir = fs::get_parent_dir(track);
					}

					music_selection_context context{};
					context.set_playlist(dir);
					context.set_track(track);
					// TODO: context.repeat_mode = CELL_SEARCH_REPEATMODE_NONE;
					// TODO: context.context_option = CELL_SEARCH_CONTEXTOPTION_NONE;
					dec.current_selection_context = std::move(context);
					dec.current_selection_context.create_playlist(music_selection_context::get_next_hash());
					cellMusicDecode.success("Media list dialog: selected entry '%s'", dec.current_selection_context.playlist.front());
				}
				else
				{
					cellMusicDecode.warning("Media list dialog was canceled");
				}
				dec.func(ppu, CELL_MUSIC_DECODE_EVENT_SELECT_CONTENTS_RESULT, vm::addr_t(result), dec.userData);
				return CELL_OK;
			});
		});
	return error;
}

template <typename Music_Decode>
error_code cell_music_decode_read(vm::ptr<void> buf, vm::ptr<u32> startTime, u64 reqSize, vm::ptr<u64> readSize, vm::ptr<s32> position)
{
	if (!buf || !startTime || !position || !reqSize || !readSize)
	{
		return CELL_MUSIC_DECODE_ERROR_PARAM;
	}

	*position = CELL_MUSIC_DECODE_POSITION_NONE;
	*readSize = 0;
	*startTime = 0;

	auto& dec = g_fxo->get<Music_Decode>();
	std::lock_guard lock(dec.mutex);
	std::scoped_lock slock(dec.decoder.m_mtx);

	if (dec.decoder.has_error)
	{
		return CELL_MUSIC_DECODE_ERROR_DECODE_FAILURE;
	}

	if (dec.decoder.m_size == 0)
	{
		return CELL_MUSIC_DECODE_ERROR_NO_LPCM_DATA;
	}

	const u64 size_left = dec.decoder.m_size - dec.read_pos;

	if (dec.read_pos == 0)
	{
		cellMusicDecode.trace("cell_music_decode_read: position=CELL_MUSIC_DECODE_POSITION_START, read_pos=%d, reqSize=%d, m_size=%d", dec.read_pos, reqSize, dec.decoder.m_size.load());
		*position = CELL_MUSIC_DECODE_POSITION_START;
	}
	else if (!dec.decoder.track_fully_decoded || size_left > reqSize) // track_fully_decoded is not guarded by a mutex, but since it is set to true after the decode, it should be fine.
	{
		cellMusicDecode.trace("cell_music_decode_read: position=CELL_MUSIC_DECODE_POSITION_MID, read_pos=%d, reqSize=%d, m_size=%d", dec.read_pos, reqSize, dec.decoder.m_size.load());
		*position = CELL_MUSIC_DECODE_POSITION_MID;
	}
	else
	{
		if (dec.decoder.set_next_index(true) == umax)
		{
			cellMusicDecode.trace("cell_music_decode_read: position=CELL_MUSIC_DECODE_POSITION_END_LIST_END, read_pos=%d, reqSize=%d, m_size=%d", dec.read_pos, reqSize, dec.decoder.m_size.load());
			*position = CELL_MUSIC_DECODE_POSITION_END_LIST_END;
		}
		else
		{
			cellMusicDecode.trace("cell_music_decode_read: position=CELL_MUSIC_DECODE_POSITION_END, read_pos=%d, reqSize=%d, m_size=%d", dec.read_pos, reqSize, dec.decoder.m_size.load());
			*position = CELL_MUSIC_DECODE_POSITION_END;
		}
	}

	const u64 size_to_read = std::min(reqSize, size_left);
	*readSize = size_to_read;

	if (size_to_read == 0)
	{
		return CELL_MUSIC_DECODE_ERROR_NO_LPCM_DATA; // TODO: speculative
	}

	std::memcpy(buf.get_ptr(), &dec.decoder.data[dec.read_pos], size_to_read);

	if (size_to_read < reqSize)
	{
		// Set the rest of the buffer to zero to prevent loud pops at the end of the stream if the game ignores the readSize.
		std::memset(vm::static_ptr_cast<u8>(buf).get_ptr() + size_to_read, 0, reqSize - size_to_read);
	}

	dec.read_pos += size_to_read;

	s64 start_time_ms = 0;

	if (!dec.decoder.timestamps_ms.empty())
	{
		start_time_ms = dec.decoder.timestamps_ms.front().second;

		while (dec.decoder.timestamps_ms.size() > 1 && dec.read_pos >= ::at32(dec.decoder.timestamps_ms, 1).first)
		{
			dec.decoder.timestamps_ms.pop_front();
		}
	}

	*startTime = static_cast<u32>(start_time_ms); // startTime is milliseconds

	switch (*position)
	{
	case CELL_MUSIC_DECODE_POSITION_END_LIST_END:
	{
		// Reset the decoder and the decode status
		ensure(dec.set_decode_command(CELL_MUSIC_DECODE_CMD_STOP) == CELL_OK);
		dec.read_pos = 0;
		break;
	}
	case CELL_MUSIC_DECODE_POSITION_END:
	{
		dec.read_pos = 0;
		dec.decoder.clear();
		dec.decoder.track_fully_consumed = 1;
		dec.decoder.track_fully_consumed.notify_one();
		break;
	}
	default:
	{
		break;
	}
	}

	cellMusicDecode.trace("cell_music_decode_read(size_to_read=%d, samples=%d, start_time_ms=%d)", size_to_read, size_to_read / sizeof(u64), start_time_ms);

	return CELL_OK;
}

error_code cellMusicDecodeInitialize(s32 mode, u32 container, s32 spuPriority, vm::ptr<CellMusicDecodeCallback> func, vm::ptr<void> userData)
{
	cellMusicDecode.warning("cellMusicDecodeInitialize(mode=0x%x, container=0x%x, spuPriority=0x%x, func=*0x%x, userData=*0x%x)", mode, container, spuPriority, func, userData);

	if (mode != CELL_MUSIC_DECODE2_MODE_NORMAL || (spuPriority - 0x10U > 0xef) || !func)
	{
		return CELL_MUSIC_DECODE_ERROR_PARAM;
	}

	auto& dec = g_fxo->get<music_decode>();
	std::lock_guard lock(dec.mutex);
	dec.func = func;
	dec.userData = userData;

	sysutil_register_cb([&dec](ppu_thread& ppu) -> s32
	{
		dec.func(ppu, CELL_MUSIC_DECODE_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), dec.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeInitializeSystemWorkload(s32 mode, u32 container, vm::ptr<CellMusicDecodeCallback> func, vm::ptr<void> userData, s32 spuUsageRate, vm::ptr<CellSpurs> spurs, vm::cptr<u8> priority, vm::cptr<struct CellSpursSystemWorkloadAttribute> attr)
{
	cellMusicDecode.warning("cellMusicDecodeInitializeSystemWorkload(mode=0x%x, container=0x%x, func=*0x%x, userData=*0x%x, spuUsageRate=0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, container, func, userData, spuUsageRate, spurs, priority, attr);

	if (mode != CELL_MUSIC_DECODE2_MODE_NORMAL || !func || (spuUsageRate - 1U > 99) || !spurs || !priority)
	{
		return CELL_MUSIC_DECODE_ERROR_PARAM;
	}

	auto& dec = g_fxo->get<music_decode>();
	std::lock_guard lock(dec.mutex);
	dec.func = func;
	dec.userData = userData;

	sysutil_register_cb([&dec](ppu_thread& ppu) -> s32
	{
		dec.func(ppu, CELL_MUSIC_DECODE_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), dec.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeFinalize()
{
	cellMusicDecode.todo("cellMusicDecodeFinalize()");

	auto& dec = g_fxo->get<music_decode>();
	std::lock_guard lock(dec.mutex);
	dec.finalize();

	if (dec.func)
	{
		sysutil_register_cb([&dec](ppu_thread& ppu) -> s32
		{
			dec.func(ppu, CELL_MUSIC_DECODE_EVENT_FINALIZE_RESULT, vm::addr_t(CELL_OK), dec.userData);
			return CELL_OK;
		});
	}

	return CELL_OK;
}

error_code cellMusicDecodeSelectContents()
{
	cellMusicDecode.todo("cellMusicDecodeSelectContents()");

	return cell_music_decode_select_contents<music_decode>();
}

error_code cellMusicDecodeSetDecodeCommand(s32 command)
{
	cellMusicDecode.warning("cellMusicDecodeSetDecodeCommand(command=0x%x)", command);

	if (command < CELL_MUSIC_DECODE_CMD_STOP || command > CELL_MUSIC_DECODE_CMD_PREV)
	{
		return CELL_MUSIC_DECODE_ERROR_PARAM;
	}

	auto& dec = g_fxo->get<music_decode>();
	std::lock_guard lock(dec.mutex);

	if (!dec.func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	error_code result = CELL_OK;
	{
		std::scoped_lock slock(dec.decoder.m_mtx);
		result = dec.set_decode_command(command);
	}

	sysutil_register_cb([&dec, result](ppu_thread& ppu) -> s32
	{
		dec.func(ppu, CELL_MUSIC_DECODE_EVENT_SET_DECODE_COMMAND_RESULT, vm::addr_t(s32{result}), dec.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeGetDecodeStatus(vm::ptr<s32> status)
{
	cellMusicDecode.todo("cellMusicDecodeGetDecodeStatus(status=*0x%x)", status);

	if (!status)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

	auto& dec = g_fxo->get<music_decode>();
	std::lock_guard lock(dec.mutex);
	*status = dec.decode_status;

	cellMusicDecode.notice("cellMusicDecodeGetDecodeStatus: status=%d", *status);
	return CELL_OK;
}

error_code cellMusicDecodeRead(vm::ptr<void> buf, vm::ptr<u32> startTime, u64 reqSize, vm::ptr<u64> readSize, vm::ptr<s32> position)
{
	cellMusicDecode.trace("cellMusicDecodeRead(buf=*0x%x, startTime=*0x%x, reqSize=0x%llx, readSize=*0x%x, position=*0x%x)", buf, startTime, reqSize, readSize, position);

	return cell_music_decode_read<music_decode>(buf, startTime, reqSize, readSize, position);
}

error_code cellMusicDecodeGetSelectionContext(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusicDecode.todo("cellMusicDecodeGetSelectionContext(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

	auto& dec = g_fxo->get<music_decode>();
	std::lock_guard lock(dec.mutex);

	*context = dec.current_selection_context.get();
	cellMusicDecode.warning("cellMusicDecodeGetSelectionContext: selection_context = %s", dec.current_selection_context.to_string());

	return CELL_OK;
}

error_code cellMusicDecodeSetSelectionContext(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusicDecode.todo("cellMusicDecodeSetSelectionContext(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

	auto& dec = g_fxo->get<music_decode>();
	std::lock_guard lock(dec.mutex);

	if (!dec.func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	const bool result = dec.current_selection_context.set(*context);
	if (result) cellMusicDecode.warning("cellMusicDecodeSetSelectionContext: new selection_context = %s", dec.current_selection_context.to_string());
	else cellMusicDecode.error("cellMusicDecodeSetSelectionContext: failed. context = '%s'", music_selection_context::context_to_hex(*context));

	sysutil_register_cb([&dec, result](ppu_thread& ppu) -> s32
	{
		const u32 status = result ? u32{CELL_OK} : u32{CELL_MUSIC_DECODE_ERROR_INVALID_CONTEXT};
		dec.func(ppu, CELL_MUSIC_DECODE_EVENT_SET_SELECTION_CONTEXT_RESULT, vm::addr_t(status), dec.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeGetContentsId(vm::ptr<CellSearchContentId> contents_id)
{
	cellMusicDecode.todo("cellMusicDecodeGetContentsId(contents_id=*0x%x)", contents_id);

	if (!contents_id)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

	// HACKY
	auto& dec = g_fxo->get<music_decode>();
	std::lock_guard lock(dec.mutex);
	return dec.current_selection_context.find_content_id(contents_id);
}

error_code cellMusicDecodeInitialize2(s32 mode, u32 container, s32 spuPriority, vm::ptr<CellMusicDecode2Callback> func, vm::ptr<void> userData, s32 speed, s32 bufSize)
{
	cellMusicDecode.warning("cellMusicDecodeInitialize2(mode=0x%x, container=0x%x, spuPriority=0x%x, func=*0x%x, userData=*0x%x, speed=0x%x, bufSize=0x%x)", mode, container, spuPriority, func, userData, speed, bufSize);

	if (mode != CELL_MUSIC_DECODE2_MODE_NORMAL || (spuPriority - 0x10U > 0xef) ||
		bufSize < CELL_MUSIC_DECODE2_MIN_BUFFER_SIZE || !func ||
		(speed != CELL_MUSIC_DECODE2_SPEED_MAX && speed != CELL_MUSIC_DECODE2_SPEED_2))
	{
		return CELL_MUSIC_DECODE_ERROR_PARAM;
	}

	auto& dec = g_fxo->get<music_decode2>();
	std::lock_guard lock(dec.mutex);
	dec.func = func;
	dec.userData = userData;
	dec.speed = speed;

	sysutil_register_cb([userData, &dec](ppu_thread& ppu) -> s32
	{
		dec.func(ppu, CELL_MUSIC_DECODE_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeInitialize2SystemWorkload(s32 mode, u32 container, vm::ptr<CellMusicDecode2Callback> func, vm::ptr<void> userData, s32 spuUsageRate, s32 bufSize, vm::ptr<CellSpurs> spurs, vm::cptr<u8> priority, vm::cptr<CellSpursSystemWorkloadAttribute> attr)
{
	cellMusicDecode.warning("cellMusicDecodeInitialize2SystemWorkload(mode=0x%x, container=0x%x, func=*0x%x, userData=*0x%x, spuUsageRate=0x%x, bufSize=0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, container, func, userData, spuUsageRate, bufSize, spurs, priority, attr);

	if (mode != CELL_MUSIC_DECODE2_MODE_NORMAL || !func || (spuUsageRate - 1U > 99) ||
		bufSize < CELL_MUSIC_DECODE2_MIN_BUFFER_SIZE || !spurs || !priority)
	{
		return CELL_MUSIC_DECODE_ERROR_PARAM;
	}

	auto& dec = g_fxo->get<music_decode2>();
	std::lock_guard lock(dec.mutex);
	dec.func = func;
	dec.userData = userData;

	sysutil_register_cb([&dec](ppu_thread& ppu) -> s32
	{
		dec.func(ppu, CELL_MUSIC_DECODE_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), dec.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeFinalize2()
{
	cellMusicDecode.todo("cellMusicDecodeFinalize2()");

	auto& dec = g_fxo->get<music_decode2>();
	std::lock_guard lock(dec.mutex);
	dec.finalize();

	if (dec.func)
	{
		sysutil_register_cb([&dec](ppu_thread& ppu) -> s32
		{
			dec.func(ppu, CELL_MUSIC_DECODE_EVENT_FINALIZE_RESULT, vm::addr_t(CELL_OK), dec.userData);
			return CELL_OK;
		});
	}

	return CELL_OK;
}

error_code cellMusicDecodeSelectContents2()
{
	cellMusicDecode.todo("cellMusicDecodeSelectContents2()");

	return cell_music_decode_select_contents<music_decode2>();
}

error_code cellMusicDecodeSetDecodeCommand2(s32 command)
{
	cellMusicDecode.warning("cellMusicDecodeSetDecodeCommand2(command=0x%x)", command);

	if (command < CELL_MUSIC_DECODE_CMD_STOP || command > CELL_MUSIC_DECODE_CMD_PREV)
	{
		return CELL_MUSIC_DECODE_ERROR_PARAM;
	}

	auto& dec = g_fxo->get<music_decode2>();
	std::lock_guard lock(dec.mutex);

	if (!dec.func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	error_code result = CELL_OK;
	{
		std::scoped_lock slock(dec.decoder.m_mtx);
		result = dec.set_decode_command(command);
	}

	sysutil_register_cb([&dec, result](ppu_thread& ppu) -> s32
	{
		dec.func(ppu, CELL_MUSIC_DECODE_EVENT_SET_DECODE_COMMAND_RESULT, vm::addr_t(s32{result}), dec.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeGetDecodeStatus2(vm::ptr<s32> status)
{
	cellMusicDecode.todo("cellMusicDecodeGetDecodeStatus2(status=*0x%x)", status);

	if (!status)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

	auto& dec = g_fxo->get<music_decode2>();
	std::lock_guard lock(dec.mutex);
	*status = dec.decode_status;

	cellMusicDecode.notice("cellMusicDecodeGetDecodeStatus2: status=%d", *status);
	return CELL_OK;
}

error_code cellMusicDecodeRead2(vm::ptr<void> buf, vm::ptr<u32> startTime, u64 reqSize, vm::ptr<u64> readSize, vm::ptr<s32> position)
{
	cellMusicDecode.trace("cellMusicDecodeRead2(buf=*0x%x, startTime=*0x%x, reqSize=0x%llx, readSize=*0x%x, position=*0x%x)", buf, startTime, reqSize, readSize, position);

	return cell_music_decode_read<music_decode2>(buf, startTime, reqSize, readSize, position);
}

error_code cellMusicDecodeGetSelectionContext2(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusicDecode.todo("cellMusicDecodeGetSelectionContext2(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

	auto& dec = g_fxo->get<music_decode2>();
	std::lock_guard lock(dec.mutex);

	*context = dec.current_selection_context.get();
	cellMusicDecode.warning("cellMusicDecodeGetSelectionContext2: selection context = %s)", dec.current_selection_context.to_string());

	return CELL_OK;
}

error_code cellMusicDecodeSetSelectionContext2(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusicDecode.todo("cellMusicDecodeSetSelectionContext2(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

	auto& dec = g_fxo->get<music_decode2>();
	std::lock_guard lock(dec.mutex);

	if (!dec.func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	const bool result = dec.current_selection_context.set(*context);
	if (result) cellMusicDecode.warning("cellMusicDecodeSetSelectionContext2: new selection_context = %s", dec.current_selection_context.to_string());
	else cellMusicDecode.error("cellMusicDecodeSetSelectionContext2: failed. context = '%s'", music_selection_context::context_to_hex(*context));

	sysutil_register_cb([&dec, result](ppu_thread& ppu) -> s32
	{
		const u32 status = result ? u32{CELL_OK} : u32{CELL_MUSIC_DECODE_ERROR_INVALID_CONTEXT};
		dec.func(ppu, CELL_MUSIC_DECODE_EVENT_SET_SELECTION_CONTEXT_RESULT, vm::addr_t(status), dec.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicDecodeGetContentsId2(vm::ptr<CellSearchContentId> contents_id)
{
	cellMusicDecode.todo("cellMusicDecodeGetContentsId2(contents_id=*0x%x)", contents_id);

	if (!contents_id)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

	// HACKY
	auto& dec = g_fxo->get<music_decode2>();
	std::lock_guard lock(dec.mutex);
	return dec.current_selection_context.find_content_id(contents_id);
}


DECLARE(ppu_module_manager::cellMusicDecode)("cellMusicDecodeUtility", []()
{
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeInitialize);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeInitializeSystemWorkload);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeFinalize);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSelectContents);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSetDecodeCommand);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetDecodeStatus);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeRead);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetSelectionContext);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSetSelectionContext);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetContentsId);

	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeInitialize2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeInitialize2SystemWorkload);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeFinalize2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSelectContents2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSetDecodeCommand2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetDecodeStatus2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeRead2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetSelectionContext2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeSetSelectionContext2);
	REG_FUNC(cellMusicDecodeUtility, cellMusicDecodeGetContentsId2);
});
