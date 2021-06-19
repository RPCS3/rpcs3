#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/RSX/Overlays/overlay_media_list_dialog.h"
#include "Emu/VFS.h"
#include "cellMusic.h"
#include "cellSearch.h"
#include "cellSpurs.h"
#include "cellSysutil.h"
#include "util/media_utils.h"

#include <deque>


LOG_CHANNEL(cellMusicDecode);

// Return Codes (CELL_MUSIC_DECODE2 codes are omitted if they are identical)
enum CellMusicDecodeError : u32
{
	CELL_MUSIC_DECODE_CANCELED                = 1,
	CELL_MUSIC_DECODE_DECODE_FINISHED         = 0x8002C101,
	CELL_MUSIC_DECODE_ERROR_PARAM             = 0x8002C102,
	CELL_MUSIC_DECODE_ERROR_BUSY              = 0x8002C103,
	CELL_MUSIC_DECODE_ERROR_NO_ACTIVE_CONTENT = 0x8002C104,
	CELL_MUSIC_DECODE_ERROR_NO_MATCH_FOUND    = 0x8002C105,
	CELL_MUSIC_DECODE_ERROR_INVALID_CONTEXT   = 0x8002C106,
	CELL_MUSIC_DECODE_ERROR_DECODE_FAILURE    = 0x8002C107,
	CELL_MUSIC_DECODE_ERROR_NO_MORE_CONTENT   = 0x8002C108,
	CELL_MUSIC_DECODE_DIALOG_OPEN             = 0x8002C109,
	CELL_MUSIC_DECODE_DIALOG_CLOSE            = 0x8002C10A,
	CELL_MUSIC_DECODE_ERROR_NO_LPCM_DATA      = 0x8002C10B,
	CELL_MUSIC_DECODE_NEXT_CONTENTS_READY     = 0x8002C10C,
	CELL_MUSIC_DECODE_ERROR_GENERIC           = 0x8002C1FF,
};

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

// Constants (CELL_MUSIC_DECODE2 codes are omitted if they are identical)
enum
{
	CELL_MUSIC_DECODE_EVENT_STATUS_NOTIFICATION = 0,
	CELL_MUSIC_DECODE_EVENT_INITIALIZE_RESULT = 1,
	CELL_MUSIC_DECODE_EVENT_FINALIZE_RESULT = 2,
	CELL_MUSIC_DECODE_EVENT_SELECT_CONTENTS_RESULT = 3,
	CELL_MUSIC_DECODE_EVENT_SET_DECODE_COMMAND_RESULT = 4,
	CELL_MUSIC_DECODE_EVENT_SET_SELECTION_CONTEXT_RESULT = 5,
	CELL_MUSIC_DECODE_EVENT_UI_NOTIFICATION = 6,
	CELL_MUSIC_DECODE_EVENT_NEXT_CONTENTS_READY_RESULT = 7,

	CELL_MUSIC_DECODE_MODE_NORMAL = 0,

	CELL_MUSIC_DECODE_CMD_STOP  = 0,
	CELL_MUSIC_DECODE_CMD_START = 1,
	CELL_MUSIC_DECODE_CMD_NEXT  = 2,
	CELL_MUSIC_DECODE_CMD_PREV  = 3,

	CELL_MUSIC_DECODE_STATUS_DORMANT  = 0,
	CELL_MUSIC_DECODE_STATUS_DECODING = 1,

	CELL_MUSIC_DECODE_POSITION_NONE         = 0,
	CELL_MUSIC_DECODE_POSITION_START        = 1,
	CELL_MUSIC_DECODE_POSITION_MID          = 2,
	CELL_MUSIC_DECODE_POSITION_END          = 3,
	CELL_MUSIC_DECODE_POSITION_END_LIST_END = 4,

	CELL_MUSIC_DECODE2_SPEED_MAX = 0,
	CELL_MUSIC_DECODE2_SPEED_2   = 2,

	CELL_SYSUTIL_MUSIC_DECODE2_INITIALIZING_FINISHED = 1,
	CELL_SYSUTIL_MUSIC_DECODE2_SHUTDOWN_FINISHED     = 4, // 3(SDK103) -> 4(SDK110)
	CELL_SYSUTIL_MUSIC_DECODE2_LOADING_FINISHED      = 5,
	CELL_SYSUTIL_MUSIC_DECODE2_UNLOADING_FINISHED    = 7,
	CELL_SYSUTIL_MUSIC_DECODE2_RELEASED              = 9,
	CELL_SYSUTIL_MUSIC_DECODE2_GRABBED               = 11,

	CELL_MUSIC_DECODE2_MIN_BUFFER_SIZE = 448 * 1024,
	CELL_MUSIC_DECODE2_MANAGEMENT_SIZE = 64 * 1024,
	CELL_MUSIC_DECODE2_PAGESIZE_64K    = 64 * 1024,
	CELL_MUSIC_DECODE2_PAGESIZE_1M     = 1 * 1024 * 1024,
};

using CellMusicDecodeCallback = void(u32, vm::ptr<void> param, vm::ptr<void> userData);
using CellMusicDecode2Callback = void(u32, vm::ptr<void> param, vm::ptr<void> userData);

struct music_decode
{
	vm::ptr<CellMusicDecodeCallback> func{};
	vm::ptr<void> userData{};
	music_selection_context current_selection_context{};
	s32 decode_status = CELL_MUSIC_DECODE_STATUS_DORMANT;
	s32 decode_command = CELL_MUSIC_DECODE_CMD_STOP;
	u64 readPos = 0;
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
			readPos = 0;

			// Decode data. The format of the decoded data is 48kHz, float 32bit, 2ch LPCM data interleaved in order from left to right.
			const std::string path = vfs::get(current_selection_context.content_path);
			cellMusicDecode.notice("set_decode_command(START): Setting vfs path: '%s' (unresolved='%s')", path, current_selection_context.content_path);

			decoder.set_path(path);
			decoder.set_swap_endianness(true);
			decoder.decode();
			break;
		}
		case CELL_MUSIC_DECODE_CMD_NEXT: // TODO: set path of next file if possible
		case CELL_MUSIC_DECODE_CMD_PREV: // TODO: set path of prev file if possible
		{
			return CELL_MUSIC_DECODE_ERROR_NO_MORE_CONTENT;
		}
		}
		return CELL_OK;
	}

	error_code finalize()
	{
		decoder.stop();
		decode_status = CELL_MUSIC_DECODE_STATUS_DORMANT;
		decode_command = CELL_MUSIC_DECODE_CMD_STOP;
		readPos = 0;
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

	const std::string dir_path = "/dev_hdd0/music";
	const std::string vfs_dir_path = vfs::get("/dev_hdd0/music");
	const std::string title = get_localized_string(localized_string_id::RSX_OVERLAYS_MEDIA_DIALOG_EMPTY);

	error_code error = rsx::overlays::show_media_list_dialog(rsx::overlays::media_list_dialog::media_type::audio, vfs_dir_path, title,
		[&dec, dir_path, vfs_dir_path](s32 status, utils::media_info info)
		{
			sysutil_register_cb([&dec, dir_path, vfs_dir_path, info, status](ppu_thread& ppu) -> s32
			{
				std::lock_guard lock(dec.mutex);
				const u32 result = status >= 0 ? u32{CELL_OK} : u32{CELL_MUSIC_DECODE_CANCELED};
				if (result == CELL_OK)
				{
					music_selection_context context;
					context.content_path = dir_path + info.path.substr(vfs_dir_path.length()); // We need the non-vfs path here
					context.content_type = fs::is_dir(info.path) ? CELL_SEARCH_CONTENTTYPE_MUSICLIST : CELL_SEARCH_CONTENTTYPE_MUSIC;
					// TODO: context.repeat_mode = CELL_SEARCH_REPEATMODE_NONE;
					// TODO: context.context_option = CELL_SEARCH_CONTEXTOPTION_NONE;
					dec.current_selection_context = context;
					cellMusicDecode.success("Media list dialog: selected entry '%s'", context.content_path);
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
	if (!buf || !startTime || !position || !readSize)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

	auto& dec = g_fxo->get<Music_Decode>();
	std::lock_guard lock(dec.mutex);
	std::scoped_lock slock(dec.decoder.m_mtx);

	if (dec.decoder.has_error)
		return CELL_MUSIC_DECODE_ERROR_DECODE_FAILURE;

	if (dec.decoder.m_size == 0)
	{
		*position = CELL_MUSIC_DECODE_POSITION_NONE;
		*readSize = 0;
		return CELL_MUSIC_DECODE_ERROR_NO_LPCM_DATA;
	}

	if (dec.readPos == 0)
	{
		*position = CELL_MUSIC_DECODE_POSITION_START;
	}
	else if ((dec.readPos + reqSize) >= dec.decoder.m_size)
	{
		*position = CELL_MUSIC_DECODE_POSITION_END_LIST_END;
	}
	else
	{
		*position = CELL_MUSIC_DECODE_POSITION_MID;
	}

	const u64 size_to_read = (dec.readPos + reqSize) <= dec.decoder.m_size ? reqSize : dec.decoder.m_size - dec.readPos;
	std::memcpy(buf.get_ptr(), &dec.decoder.data[dec.readPos], size_to_read);

	dec.readPos += size_to_read;
	*readSize = size_to_read;

	s64 start_time_ms = 0;

	if (!dec.decoder.timestamps_ms.empty())
	{
		start_time_ms = dec.decoder.timestamps_ms.front().second;

		while (dec.decoder.timestamps_ms.size() > 1 && dec.readPos >= dec.decoder.timestamps_ms.at(1).first)
		{
			dec.decoder.timestamps_ms.pop_front();
		}
	}

	*startTime = static_cast<u32>(start_time_ms); // startTime is milliseconds

	cellMusicDecode.trace("cell_music_decode_read(size_to_read=%d, samples=%d, start_time_ms=%d)", size_to_read, size_to_read / sizeof(u64), start_time_ms);

	return CELL_OK;
}

error_code cellMusicDecodeInitialize(s32 mode, u32 container, s32 spuPriority, vm::ptr<CellMusicDecodeCallback> func, vm::ptr<void> userData)
{
	cellMusicDecode.todo("cellMusicDecodeInitialize(mode=0x%x, container=0x%x, spuPriority=0x%x, func=*0x%x, userData=*0x%x)", mode, container, spuPriority, func, userData);

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
	cellMusicDecode.todo("cellMusicDecodeInitializeSystemWorkload(mode=0x%x, container=0x%x, func=*0x%x, userData=*0x%x, spuUsageRate=0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, container, func, userData, spuUsageRate, spurs, priority, attr);

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
	cellMusicDecode.todo("cellMusicDecodeSetDecodeCommand(command=0x%x)", command);

	auto& dec = g_fxo->get<music_decode>();
	std::lock_guard lock(dec.mutex);

	if (!dec.func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	const error_code result = dec.set_decode_command(command);

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

	return CELL_OK;
}

error_code cellMusicDecodeRead(vm::ptr<void> buf, vm::ptr<u32> startTime, u64 reqSize, vm::ptr<u64> readSize, vm::ptr<s32> position)
{
	cellMusicDecode.notice("cellMusicDecodeRead(buf=*0x%x, startTime=*0x%x, reqSize=0x%llx, readSize=*0x%x, position=*0x%x)", buf, startTime, reqSize, readSize, position);

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
	else cellMusicDecode.error("cellMusicDecodeSetSelectionContext: failed. context = '%s'", context->data);

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
		return CELL_MUSIC_ERROR_PARAM;

	// HACKY
	auto& dec = g_fxo->get<music_decode>();
	std::lock_guard lock(dec.mutex);
	return dec.current_selection_context.find_content_id(contents_id);
}

error_code cellMusicDecodeInitialize2(s32 mode, u32 container, s32 spuPriority, vm::ptr<CellMusicDecode2Callback> func, vm::ptr<void> userData, s32 speed, s32 bufSize)
{
	cellMusicDecode.todo("cellMusicDecodeInitialize2(mode=0x%x, container=0x%x, spuPriority=0x%x, func=*0x%x, userData=*0x%x, speed=0x%x, bufSize=0x%x)", mode, container, spuPriority, func, userData, speed, bufSize);

	if (bufSize < CELL_MUSIC_DECODE2_MIN_BUFFER_SIZE)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

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
	cellMusicDecode.todo("cellMusicDecodeInitialize2SystemWorkload(mode=0x%x, container=0x%x, func=*0x%x, userData=*0x%x, spuUsageRate=0x%x, bufSize=0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, container, func, userData, spuUsageRate, bufSize, spurs, priority, attr);

	if (bufSize < CELL_MUSIC_DECODE2_MIN_BUFFER_SIZE)
		return CELL_MUSIC_DECODE_ERROR_PARAM;

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
	cellMusicDecode.todo("cellMusicDecodeSetDecodeCommand2(command=0x%x)", command);

	auto& dec = g_fxo->get<music_decode2>();
	std::lock_guard lock(dec.mutex);

	if (!dec.func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	const error_code result = dec.set_decode_command(command);

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

	return CELL_OK;
}

error_code cellMusicDecodeRead2(vm::ptr<void> buf, vm::ptr<u32> startTime, u64 reqSize, vm::ptr<u64> readSize, vm::ptr<s32> position)
{
	cellMusicDecode.notice("cellMusicDecodeRead2(buf=*0x%x, startTime=*0x%x, reqSize=0x%llx, readSize=*0x%x, position=*0x%x)", buf, startTime, reqSize, readSize, position);

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

	auto& dec = g_fxo->get<music_decode2>();
	std::lock_guard lock(dec.mutex);

	if (!dec.func)
		return CELL_MUSIC_DECODE_ERROR_GENERIC;

	const bool result = dec.current_selection_context.set(*context);
	if (result) cellMusicDecode.warning("cellMusicDecodeSetSelectionContext2: new selection_context = %s", dec.current_selection_context.to_string());
	else cellMusicDecode.error("cellMusicDecodeSetSelectionContext2: failed. context = '%s'", context->data);

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
		return CELL_MUSIC2_ERROR_PARAM;

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
