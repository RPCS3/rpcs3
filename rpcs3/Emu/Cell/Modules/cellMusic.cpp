#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/Io/music_handler_base.h"
#include "Emu/System.h"
#include "Emu/VFS.h"
#include "Emu/RSX/Overlays/overlay_media_list_dialog.h"
#include "cellSearch.h"
#include "cellSpurs.h"
#include "cellSysutil.h"
#include "cellMusic.h"

LOG_CHANNEL(cellMusic);

template<>
void fmt_class_string<CellMusicError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MUSIC_CANCELED);
			STR_CASE(CELL_MUSIC_PLAYBACK_FINISHED);
			STR_CASE(CELL_MUSIC_ERROR_PARAM);
			STR_CASE(CELL_MUSIC_ERROR_BUSY);
			STR_CASE(CELL_MUSIC_ERROR_NO_ACTIVE_CONTENT);
			STR_CASE(CELL_MUSIC_ERROR_NO_MATCH_FOUND);
			STR_CASE(CELL_MUSIC_ERROR_INVALID_CONTEXT);
			STR_CASE(CELL_MUSIC_ERROR_PLAYBACK_FAILURE);
			STR_CASE(CELL_MUSIC_ERROR_NO_MORE_CONTENT);
			STR_CASE(CELL_MUSIC_DIALOG_OPEN);
			STR_CASE(CELL_MUSIC_DIALOG_CLOSE);
			STR_CASE(CELL_MUSIC_ERROR_GENERIC);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellMusic2Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MUSIC2_CANCELED);
			STR_CASE(CELL_MUSIC2_PLAYBACK_FINISHED);
			STR_CASE(CELL_MUSIC2_ERROR_PARAM);
			STR_CASE(CELL_MUSIC2_ERROR_BUSY);
			STR_CASE(CELL_MUSIC2_ERROR_NO_ACTIVE_CONTENT);
			STR_CASE(CELL_MUSIC2_ERROR_NO_MATCH_FOUND);
			STR_CASE(CELL_MUSIC2_ERROR_INVALID_CONTEXT);
			STR_CASE(CELL_MUSIC2_ERROR_PLAYBACK_FAILURE);
			STR_CASE(CELL_MUSIC2_ERROR_NO_MORE_CONTENT);
			STR_CASE(CELL_MUSIC2_DIALOG_OPEN);
			STR_CASE(CELL_MUSIC2_DIALOG_CLOSE);
			STR_CASE(CELL_MUSIC2_ERROR_GENERIC);
		}

		return unknown;
	});
}

struct music_state
{
	shared_mutex mutex;

	vm::ptr<void(u32 event, vm::ptr<void> param, vm::ptr<void> userData)> func{};
	vm::ptr<void> userData{};
	std::mutex mtx;
	std::shared_ptr<music_handler_base> handler;
	music_selection_context current_selection_context;

	music_state()
	{
		handler = Emu.GetCallbacks().get_music_handler();
	}
};

error_code cell_music_select_contents()
{
	auto& music = g_fxo->get<music_state>();

	if (!music.func)
		return CELL_MUSIC_ERROR_GENERIC;

	const std::string dir_path = "/dev_hdd0/music";
	const std::string vfs_dir_path = vfs::get("/dev_hdd0/music");
	const std::string title = get_localized_string(localized_string_id::RSX_OVERLAYS_MEDIA_DIALOG_EMPTY);

	error_code error = rsx::overlays::show_media_list_dialog(rsx::overlays::media_list_dialog::media_type::audio, vfs_dir_path, title,
		[&music, dir_path, vfs_dir_path](s32 status, utils::media_info info)
		{
			sysutil_register_cb([&music, dir_path, vfs_dir_path, info, status](ppu_thread& ppu) -> s32
			{
				const u32 result = status >= 0 ? u32{CELL_OK} : u32{CELL_MUSIC_CANCELED};
				if (result == CELL_OK)
				{
					music_selection_context context;
					context.content_path = info.path;
					context.content_path = dir_path + info.path.substr(vfs_dir_path.length()); // We need the non-vfs path here
					context.content_type = fs::is_dir(info.path) ? CELL_SEARCH_CONTENTTYPE_MUSICLIST : CELL_SEARCH_CONTENTTYPE_MUSIC;
					// TODO: context.repeat_mode = CELL_SEARCH_REPEATMODE_NONE;
					// TODO: context.context_option = CELL_SEARCH_CONTEXTOPTION_NONE;
					music.current_selection_context = context;
					cellMusic.success("Media list dialog: selected entry '%s'", context.content_path);
				}
				else
				{
					cellMusic.warning("Media list dialog was canceled");
				}
				music.func(ppu, CELL_MUSIC_EVENT_SELECT_CONTENTS_RESULT, vm::addr_t(result), music.userData);
				return CELL_OK;
			});
		});
	return error;
}

error_code cellMusicGetSelectionContext(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusic.todo("cellMusicGetSelectionContext(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC_ERROR_PARAM;
	
	auto& music = g_fxo->get<music_state>();
	std::lock_guard lock(music.mtx);

	*context = music.current_selection_context.get();
	cellMusic.success("cellMusicGetSelectionContext: selection context = %s", music.current_selection_context.to_string());

	return CELL_OK;
}

error_code cellMusicSetSelectionContext2(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusic.todo("cellMusicSetSelectionContext2(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC2_ERROR_PARAM;

	auto& music = g_fxo->get<music_state>();

	if (!music.func)
		return CELL_MUSIC2_ERROR_GENERIC;

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		bool result = false;
		{
			std::lock_guard lock(music.mtx);
			result = music.current_selection_context.set(*context);
		}
		const u32 status = result ? u32{CELL_OK} : u32{CELL_MUSIC2_ERROR_INVALID_CONTEXT};

		if (result) cellMusic.success("cellMusicSetSelectionContext2: new selection context = %s)", music.current_selection_context.to_string());
		else cellMusic.todo("cellMusicSetSelectionContext2: failed. context = %s)", context->data);

		music.func(ppu, CELL_MUSIC2_EVENT_SET_SELECTION_CONTEXT_RESULT, vm::addr_t(status), music.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSetVolume2(f32 level)
{
	cellMusic.todo("cellMusicSetVolume2(level=%f)", level);

	level = std::clamp(level, 0.0f, 1.0f);

	auto& music = g_fxo->get<music_state>();

	if (!music.func)
		return CELL_MUSIC2_ERROR_GENERIC;

	music.handler->set_volume(level);

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		music.func(ppu, CELL_MUSIC2_EVENT_SET_VOLUME_RESULT, vm::addr_t(CELL_OK), music.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicGetContentsId(vm::ptr<CellSearchContentId> contents_id)
{
	cellMusic.todo("cellMusicGetContentsId(contents_id=*0x%x)", contents_id);

	if (!contents_id)
		return CELL_MUSIC_ERROR_PARAM;

	// HACKY
	auto& music = g_fxo->get<music_state>();
	std::lock_guard lock(music.mtx);
	return music.current_selection_context.find_content_id(contents_id);
}

error_code cellMusicSetSelectionContext(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusic.todo("cellMusicSetSelectionContext(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC_ERROR_PARAM;

	auto& music = g_fxo->get<music_state>();

	if (!music.func)
		return CELL_MUSIC_ERROR_GENERIC;

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		bool result = false;
		{
			std::lock_guard lock(music.mtx);
			result = music.current_selection_context.set(*context);
		}
		const u32 status = result ? u32{CELL_OK} : u32{CELL_MUSIC_ERROR_INVALID_CONTEXT};

		if (result) cellMusic.success("cellMusicSetSelectionContext: new selection context = %s)", music.current_selection_context.to_string());
		else cellMusic.todo("cellMusicSetSelectionContext: failed. context = %s)", context->data);

		music.func(ppu, CELL_MUSIC_EVENT_SET_SELECTION_CONTEXT_RESULT, vm::addr_t(status), music.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicInitialize2SystemWorkload(s32 mode, vm::ptr<CellMusic2Callback> func, vm::ptr<void> userData, vm::ptr<CellSpurs> spurs, vm::cptr<u8> priority, vm::cptr<struct CellSpursSystemWorkloadAttribute> attr)
{
	cellMusic.todo("cellMusicInitialize2SystemWorkload(mode=0x%x, func=*0x%x, userData=*0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, func, userData, spurs, priority, attr);

	if (!func)
		return CELL_MUSIC2_ERROR_PARAM;

	if (mode != CELL_MUSIC2_PLAYER_MODE_NORMAL)
	{
		cellMusic.todo("Unknown player mode: 0x%x", mode);
		return CELL_MUSIC2_ERROR_PARAM;
	}

	auto& music = g_fxo->get<music_state>();
	music.func = func;
	music.userData = userData;

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		music.func(ppu, CELL_MUSIC2_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicGetPlaybackStatus2(vm::ptr<s32> status)
{
	cellMusic.todo("cellMusicGetPlaybackStatus2(status=*0x%x)", status);

	if (!status)
		return CELL_MUSIC2_ERROR_PARAM;

	const auto& music = g_fxo->get<music_state>();
	*status = music.handler->get_state();

	return CELL_OK;
}

error_code cellMusicGetContentsId2(vm::ptr<CellSearchContentId> contents_id)
{
	cellMusic.todo("cellMusicGetContentsId2(contents_id=*0x%x)", contents_id);

	if (!contents_id)
		return CELL_MUSIC2_ERROR_PARAM;

	// HACKY
	auto& music = g_fxo->get<music_state>();
	std::lock_guard lock(music.mtx);
	return music.current_selection_context.find_content_id(contents_id);
}

error_code cellMusicFinalize()
{
	cellMusic.todo("cellMusicFinalize()");

	auto& music = g_fxo->get<music_state>();

	if (music.func)
	{
		sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
		{
			music.func(ppu, CELL_MUSIC_EVENT_FINALIZE_RESULT, vm::addr_t(CELL_OK), music.userData);
			return CELL_OK;
		});
	}

	return CELL_OK;
}

error_code cellMusicInitializeSystemWorkload(s32 mode, u32 container, vm::ptr<CellMusicCallback> func, vm::ptr<void> userData, vm::ptr<CellSpurs> spurs, vm::cptr<u8> priority, vm::cptr<struct CellSpursSystemWorkloadAttribute> attr)
{
	cellMusic.todo("cellMusicInitializeSystemWorkload(mode=0x%x, container=0x%x, func=*0x%x, userData=*0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, container, func, userData, spurs, priority, attr);

	if (!func)
		return CELL_MUSIC_ERROR_PARAM;

	if (mode != CELL_MUSIC_PLAYER_MODE_NORMAL)
	{
		cellMusic.todo("Unknown player mode: 0x%x", mode);
		return CELL_MUSIC_ERROR_PARAM;
	}

	auto& music = g_fxo->get<music_state>();
	music.func = func;
	music.userData = userData;

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		music.func(ppu, CELL_MUSIC_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicInitialize(s32 mode, u32 container, s32 spuPriority, vm::ptr<CellMusicCallback> func, vm::ptr<void> userData)
{
	cellMusic.todo("cellMusicInitialize(mode=0x%x, container=0x%x, spuPriority=0x%x, func=*0x%x, userData=*0x%x)", mode, container, spuPriority, func, userData);

	if (!func)
		return CELL_MUSIC_ERROR_PARAM;

	if (mode != CELL_MUSIC_PLAYER_MODE_NORMAL)
	{
		cellMusic.todo("Unknown player mode: 0x%x", mode);
		return CELL_MUSIC_ERROR_PARAM;
	}

	auto& music = g_fxo->get<music_state>();
	music.func = func;
	music.userData = userData;

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		music.func(ppu, CELL_MUSIC_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicFinalize2()
{
	cellMusic.todo("cellMusicFinalize2()");

	auto& music = g_fxo->get<music_state>();

	if (music.func)
	{
		sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
		{
			music.func(ppu, CELL_MUSIC2_EVENT_FINALIZE_RESULT, vm::addr_t(CELL_OK), music.userData);
			return CELL_OK;
		});
	}

	return CELL_OK;
}

error_code cellMusicGetSelectionContext2(vm::ptr<CellMusicSelectionContext> context)
{
	cellMusic.todo("cellMusicGetSelectionContext2(context=*0x%x)", context);

	if (!context)
		return CELL_MUSIC2_ERROR_PARAM;

	auto& music = g_fxo->get<music_state>();
	std::lock_guard lock(music.mtx);
	*context = music.current_selection_context.get();
	cellMusic.success("cellMusicGetSelectionContext2: selection context = %s", music.current_selection_context.to_string());

	return CELL_OK;
}

error_code cellMusicGetVolume(vm::ptr<f32> level)
{
	cellMusic.todo("cellMusicGetVolume(level=*0x%x)", level);

	if (!level)
		return CELL_MUSIC_ERROR_PARAM;

	const auto& music = g_fxo->get<music_state>();
	*level = music.handler->get_volume();

	return CELL_OK;
}

error_code cellMusicGetPlaybackStatus(vm::ptr<s32> status)
{
	cellMusic.todo("cellMusicGetPlaybackStatus(status=*0x%x)", status);

	if (!status)
		return CELL_MUSIC_ERROR_PARAM;

	const auto& music = g_fxo->get<music_state>();
	*status = music.handler->get_state();

	return CELL_OK;
}

error_code cellMusicSetPlaybackCommand2(s32 command, vm::ptr<void> param)
{
	cellMusic.todo("cellMusicSetPlaybackCommand2(command=0x%x, param=*0x%x)", command, param);

	if (command < CELL_MUSIC2_PB_CMD_STOP || command > CELL_MUSIC2_PB_CMD_FASTREVERSE)
		return CELL_MUSIC2_ERROR_PARAM;

	auto& music = g_fxo->get<music_state>();

	if (!music.func)
		return CELL_MUSIC2_ERROR_GENERIC;

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		// TODO: play proper song when the context is a playlist
		std::string path;
		{
			std::lock_guard lock(music.mtx);
			path = vfs::get(music.current_selection_context.content_path);
			cellMusic.notice("cellMusicSetPlaybackCommand2: current vfs path: '%s' (unresolved='%s')", path, music.current_selection_context.content_path);
		}

		switch (command)
		{
		case CELL_MUSIC2_PB_CMD_STOP:
			music.handler->stop();
			break;
		case CELL_MUSIC2_PB_CMD_PLAY:
			music.handler->play(path);
			break;
		case CELL_MUSIC2_PB_CMD_PAUSE:
			music.handler->pause();
			break;
		case CELL_MUSIC2_PB_CMD_NEXT:
			music.handler->play(path);
			break;
		case CELL_MUSIC2_PB_CMD_PREV:
			music.handler->play(path);
			break;
		case CELL_MUSIC2_PB_CMD_FASTFORWARD:
			music.handler->fast_forward();
			break;
		case CELL_MUSIC2_PB_CMD_FASTREVERSE:
			music.handler->fast_reverse();
			break;
		default:
			break;
		}

		music.func(ppu, CELL_MUSIC2_EVENT_SET_PLAYBACK_COMMAND_RESULT, vm::addr_t(CELL_OK), music.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSetPlaybackCommand(s32 command, vm::ptr<void> param)
{
	cellMusic.todo("cellMusicSetPlaybackCommand(command=0x%x, param=*0x%x)", command, param);

	if (command < CELL_MUSIC_PB_CMD_STOP || command > CELL_MUSIC_PB_CMD_FASTREVERSE)
		return CELL_MUSIC_ERROR_PARAM;

	auto& music = g_fxo->get<music_state>();

	if (!music.func)
		return CELL_MUSIC_ERROR_GENERIC;

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		// TODO: play proper song when the context is a playlist
		std::string path;
		{
			std::lock_guard lock(music.mtx);
			path = vfs::get(music.current_selection_context.content_path);
			cellMusic.notice("cellMusicSetPlaybackCommand: current vfs path: '%s' (unresolved='%s')", path, music.current_selection_context.content_path);
		}

		switch (command)
		{
		case CELL_MUSIC_PB_CMD_STOP:
			music.handler->stop();
			break;
		case CELL_MUSIC_PB_CMD_PLAY:
			music.handler->play(path);
			break;
		case CELL_MUSIC_PB_CMD_PAUSE:
			music.handler->pause();
			break;
		case CELL_MUSIC_PB_CMD_NEXT:
			music.handler->play(path);
			break;
		case CELL_MUSIC_PB_CMD_PREV:
			music.handler->play(path);
			break;
		case CELL_MUSIC_PB_CMD_FASTFORWARD:
			music.handler->fast_forward();
			break;
		case CELL_MUSIC_PB_CMD_FASTREVERSE:
			music.handler->fast_reverse();
			break;
		default:
			break;
		}

		music.func(ppu, CELL_MUSIC_EVENT_SET_PLAYBACK_COMMAND_RESULT, vm::addr_t(CELL_OK), music.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSelectContents2()
{
	cellMusic.todo("cellMusicSelectContents2()");

	return cell_music_select_contents();
}

error_code cellMusicSelectContents(u32 container)
{
	cellMusic.todo("cellMusicSelectContents(container=0x%x)", container);

	return cell_music_select_contents();
}

error_code cellMusicInitialize2(s32 mode, s32 spuPriority, vm::ptr<CellMusic2Callback> func, vm::ptr<void> userData)
{
	cellMusic.todo("cellMusicInitialize2(mode=%d, spuPriority=%d, func=*0x%x, userData=*0x%x)", mode, spuPriority, func, userData);

	if (!func)
		return CELL_MUSIC2_ERROR_PARAM;

	if (mode != CELL_MUSIC2_PLAYER_MODE_NORMAL)
	{
		cellMusic.todo("Unknown player mode: 0x%x", mode);
		return CELL_MUSIC2_ERROR_PARAM;
	}

	auto& music = g_fxo->get<music_state>();
	music.func = func;
	music.userData = userData;

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		music.func(ppu, CELL_MUSIC2_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSetVolume(f32 level)
{
	cellMusic.todo("cellMusicSetVolume(level=%f)", level);

	level = std::clamp(level, 0.0f, 1.0f);

	auto& music = g_fxo->get<music_state>();

	if (!music.func)
		return CELL_MUSIC_ERROR_GENERIC;

	music.handler->set_volume(level);

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		music.func(ppu, CELL_MUSIC_EVENT_SET_VOLUME_RESULT, vm::addr_t(CELL_OK), music.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicGetVolume2(vm::ptr<f32> level)
{
	cellMusic.todo("cellMusicGetVolume2(level=*0x%x)", level);

	if (!level)
		return CELL_MUSIC2_ERROR_PARAM;

	const auto& music = g_fxo->get<music_state>();
	*level = music.handler->get_volume();

	return CELL_OK;
}


DECLARE(ppu_module_manager::cellMusic)("cellMusicUtility", []()
{
	REG_FUNC(cellMusicUtility, cellMusicGetSelectionContext);
	REG_FUNC(cellMusicUtility, cellMusicSetSelectionContext2);
	REG_FUNC(cellMusicUtility, cellMusicSetVolume2);
	REG_FUNC(cellMusicUtility, cellMusicGetContentsId);
	REG_FUNC(cellMusicUtility, cellMusicSetSelectionContext);
	REG_FUNC(cellMusicUtility, cellMusicInitialize2SystemWorkload);
	REG_FUNC(cellMusicUtility, cellMusicGetPlaybackStatus2);
	REG_FUNC(cellMusicUtility, cellMusicGetContentsId2);
	REG_FUNC(cellMusicUtility, cellMusicFinalize);
	REG_FUNC(cellMusicUtility, cellMusicInitializeSystemWorkload);
	REG_FUNC(cellMusicUtility, cellMusicInitialize);
	REG_FUNC(cellMusicUtility, cellMusicFinalize2);
	REG_FUNC(cellMusicUtility, cellMusicGetSelectionContext2);
	REG_FUNC(cellMusicUtility, cellMusicGetVolume);
	REG_FUNC(cellMusicUtility, cellMusicGetPlaybackStatus);
	REG_FUNC(cellMusicUtility, cellMusicSetPlaybackCommand2);
	REG_FUNC(cellMusicUtility, cellMusicSetPlaybackCommand);
	REG_FUNC(cellMusicUtility, cellMusicSelectContents2);
	REG_FUNC(cellMusicUtility, cellMusicSelectContents);
	REG_FUNC(cellMusicUtility, cellMusicInitialize2);
	REG_FUNC(cellMusicUtility, cellMusicSetVolume);
	REG_FUNC(cellMusicUtility, cellMusicGetVolume2);
});
