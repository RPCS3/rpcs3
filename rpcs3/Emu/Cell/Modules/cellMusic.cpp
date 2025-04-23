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
	shared_mutex mtx;
	std::shared_ptr<music_handler_base> handler;
	music_selection_context current_selection_context{};

	SAVESTATE_INIT_POS(16);

	music_state()
	{
		handler = Emu.GetCallbacks().get_music_handler();
		handler->set_status_callback([this](music_handler_base::player_status status)
		{
			// TODO: disabled until I find a game that uses CELL_MUSIC_EVENT_STATUS_NOTIFICATION
			return;

			if (!func)
			{
				return;
			}

			s32 result = CELL_OK;

			switch (status)
			{
			case music_handler_base::player_status::end_of_media:
				result = CELL_MUSIC_PLAYBACK_FINISHED;
				break;
			default:
				return;
			}

			sysutil_register_cb([this, &result](ppu_thread& ppu) -> s32
			{
				cellMusic.notice("Sending status notification %d", result);
				func(ppu, CELL_MUSIC_EVENT_STATUS_NOTIFICATION, vm::addr_t(result), userData);
				return CELL_OK;
			});
		});
	}

	music_state(utils::serial& ar)
		: music_state()
	{
		save(ar);
	}

	void save(utils::serial& ar)
	{
		ar(func);

		if (!func)
		{
			return;
		}

		GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), cellMusic);

		ar(userData);
	}

	// NOTE: This function only uses CELL_MUSIC enums. CELL_MUSIC2 enums are identical.
	error_code set_playback_command(s32 command)
	{
		switch (command)
		{
		case CELL_MUSIC_PB_CMD_STOP:
			handler->stop();
			break;
		case CELL_MUSIC_PB_CMD_PAUSE:
			handler->pause();
			break;
		case CELL_MUSIC_PB_CMD_PLAY:
		case CELL_MUSIC_PB_CMD_FASTFORWARD:
		case CELL_MUSIC_PB_CMD_FASTREVERSE:
		case CELL_MUSIC_PB_CMD_NEXT:
		case CELL_MUSIC_PB_CMD_PREV:
		{
			std::string path;
			bool no_more_tracks = false;
			{
				std::lock_guard lock(mtx);
				const std::vector<std::string>& playlist = current_selection_context.playlist;
				const u32 current_track = current_selection_context.current_track;
				u32 next_track = current_track;

				if (command == CELL_MUSIC_PB_CMD_NEXT || command == CELL_MUSIC_PB_CMD_PREV)
				{
					next_track = current_selection_context.step_track(command == CELL_MUSIC_PB_CMD_NEXT);
				}

				if (next_track < playlist.size())
				{
					path = vfs::get(::at32(playlist, next_track));
					cellMusic.notice("set_playback_command: current vfs path: '%s' (unresolved='%s')", path, ::at32(playlist, next_track));
				}
				else
				{
					current_selection_context.current_track = current_track;
					no_more_tracks = true;
				}
			}

			if (no_more_tracks)
			{
				cellMusic.notice("set_playback_command: no more tracks to play");
				return CELL_MUSIC_ERROR_NO_MORE_CONTENT;
			}

			if (!fs::is_file(path))
			{
				cellMusic.error("set_playback_command: File does not exist: '%s'", path);
			}

			switch (command)
			{
			case CELL_MUSIC_PB_CMD_FASTFORWARD:
				handler->fast_forward(path);
				break;
			case CELL_MUSIC_PB_CMD_FASTREVERSE:
				handler->fast_reverse(path);
				break;
			default:
				handler->play(path);
				break;
			}

			break;
		}
		default:
			break;
		}

		return CELL_OK;
	}
};

error_code cell_music_select_contents()
{
	auto& music = g_fxo->get<music_state>();

	if (!music.func)
		return CELL_MUSIC_ERROR_GENERIC;

	const std::string vfs_dir_path = vfs::get("/dev_hdd0/music");
	const std::string title = get_localized_string(localized_string_id::RSX_OVERLAYS_MEDIA_DIALOG_TITLE);

	error_code error = rsx::overlays::show_media_list_dialog(rsx::overlays::media_list_dialog::media_type::audio, vfs_dir_path, title,
		[&music](s32 status, utils::media_info info)
		{
			sysutil_register_cb([&music, info = std::move(info), status](ppu_thread& ppu) -> s32
			{
				std::lock_guard lock(music.mtx);
				const u32 result = status >= 0 ? u32{CELL_OK} : u32{CELL_MUSIC_CANCELED};
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
					music.current_selection_context = std::move(context);
					music.current_selection_context.create_playlist(music_selection_context::get_next_hash());
					cellMusic.success("Media list dialog: selected entry '%s'", music.current_selection_context.playlist.front());
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

	if (!music.current_selection_context)
	{
		return CELL_MUSIC_ERROR_NO_ACTIVE_CONTENT;
	}

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

	sysutil_register_cb([context = *context, &music](ppu_thread& ppu) -> s32
	{
		bool result = false;
		{
			std::lock_guard lock(music.mtx);
			result = music.current_selection_context.set(context);
		}
		const u32 status = result ? u32{CELL_OK} : u32{CELL_MUSIC2_ERROR_INVALID_CONTEXT};

		if (result) cellMusic.success("cellMusicSetSelectionContext2: new selection context = %s", music.current_selection_context.to_string());
		else cellMusic.todo("cellMusicSetSelectionContext2: failed. context = %s", music_selection_context::context_to_hex(context));

		music.func(ppu, CELL_MUSIC2_EVENT_SET_SELECTION_CONTEXT_RESULT, vm::addr_t(status), music.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSetVolume2(f32 level)
{
	cellMusic.warning("cellMusicSetVolume2(level=%f)", level);

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

	if (!music.current_selection_context)
	{
		return CELL_MUSIC_ERROR_NO_ACTIVE_CONTENT;
	}

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

	sysutil_register_cb([context = *context, &music](ppu_thread& ppu) -> s32
	{
		bool result = false;
		{
			std::lock_guard lock(music.mtx);
			result = music.current_selection_context.set(context);
		}
		const u32 status = result ? u32{CELL_OK} : u32{CELL_MUSIC_ERROR_INVALID_CONTEXT};

		if (result) cellMusic.success("cellMusicSetSelectionContext: new selection context = %s)", music.current_selection_context.to_string());
		else cellMusic.todo("cellMusicSetSelectionContext: failed. context = %s)", music_selection_context::context_to_hex(context));

		music.func(ppu, CELL_MUSIC_EVENT_SET_SELECTION_CONTEXT_RESULT, vm::addr_t(status), music.userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicInitialize2SystemWorkload(s32 mode, vm::ptr<CellMusic2Callback> func, vm::ptr<void> userData, vm::ptr<CellSpurs> spurs, vm::cptr<u8> priority, vm::cptr<struct CellSpursSystemWorkloadAttribute> attr)
{
	cellMusic.todo("cellMusicInitialize2SystemWorkload(mode=0x%x, func=*0x%x, userData=*0x%x, spurs=*0x%x, priority=*0x%x, attr=*0x%x)", mode, func, userData, spurs, priority, attr);

	if (mode != CELL_MUSIC2_PLAYER_MODE_NORMAL || !func || !spurs || !priority)
	{
		return CELL_MUSIC2_ERROR_PARAM;
	}

	auto& music = g_fxo->get<music_state>();
	music.func = func;
	music.userData = userData;
	music.current_selection_context = {};

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		music.func(ppu, CELL_MUSIC2_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicGetPlaybackStatus2(vm::ptr<s32> status)
{
	cellMusic.warning("cellMusicGetPlaybackStatus2(status=*0x%x)", status);

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

	if (!music.current_selection_context)
	{
		return CELL_MUSIC2_ERROR_NO_ACTIVE_CONTENT;
	}

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

	if (mode != CELL_MUSIC2_PLAYER_MODE_NORMAL || !func || !spurs || !priority)
	{
		return CELL_MUSIC2_ERROR_PARAM;
	}

	auto& music = g_fxo->get<music_state>();
	music.func = func;
	music.userData = userData;
	music.current_selection_context = {};

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

	if (mode != CELL_MUSIC_PLAYER_MODE_NORMAL || !func || spuPriority < 16 || spuPriority > 255)
	{
		return CELL_MUSIC_ERROR_PARAM;
	}

	auto& music = g_fxo->get<music_state>();
	music.func = func;
	music.userData = userData;
	music.current_selection_context = {};

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

	if (!music.current_selection_context)
	{
		return CELL_MUSIC2_ERROR_NO_ACTIVE_CONTENT;
	}

	*context = music.current_selection_context.get();
	cellMusic.success("cellMusicGetSelectionContext2: selection context = %s", music.current_selection_context.to_string());

	return CELL_OK;
}

error_code cellMusicGetVolume(vm::ptr<f32> level)
{
	cellMusic.warning("cellMusicGetVolume(level=*0x%x)", level);

	if (!level)
		return CELL_MUSIC_ERROR_PARAM;

	const auto& music = g_fxo->get<music_state>();
	*level = music.handler->get_volume();

	return CELL_OK;
}

error_code cellMusicGetPlaybackStatus(vm::ptr<s32> status)
{
	cellMusic.warning("cellMusicGetPlaybackStatus(status=*0x%x)", status);

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
		return { CELL_MUSIC2_ERROR_GENERIC, "Not initialized" };

	error_code result = CELL_OK;

	if (!music.current_selection_context)
	{
		result = CELL_MUSIC_ERROR_GENERIC;
	}

	sysutil_register_cb([=, &music, prev_res = result](ppu_thread& ppu) -> s32
	{
		const error_code result = prev_res ? prev_res : music.set_playback_command(command);
		music.func(ppu, CELL_MUSIC2_EVENT_SET_PLAYBACK_COMMAND_RESULT, vm::addr_t(+result), music.userData);
		return CELL_OK;
	});

	return result;
}

error_code cellMusicSetPlaybackCommand(s32 command, vm::ptr<void> param)
{
	cellMusic.todo("cellMusicSetPlaybackCommand(command=0x%x, param=*0x%x)", command, param);

	if (command < CELL_MUSIC_PB_CMD_STOP || command > CELL_MUSIC_PB_CMD_FASTREVERSE)
		return CELL_MUSIC_ERROR_PARAM;

	auto& music = g_fxo->get<music_state>();

	if (!music.func)
		return { CELL_MUSIC_ERROR_GENERIC, "Not initialized" };

	error_code result = CELL_OK;

	if (!music.current_selection_context)
	{
		result = CELL_MUSIC2_ERROR_GENERIC;
	}

	sysutil_register_cb([=, &music, prev_res = result](ppu_thread& ppu) -> s32
	{
		const error_code result = prev_res ? prev_res : music.set_playback_command(command);
		music.func(ppu, CELL_MUSIC_EVENT_SET_PLAYBACK_COMMAND_RESULT, vm::addr_t(+result), music.userData);
		return CELL_OK;
	});

	return result;
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

	if (mode != CELL_MUSIC_PLAYER_MODE_NORMAL || !func || spuPriority < 16 || spuPriority > 255)
	{
		return CELL_MUSIC2_ERROR_PARAM;
	}

	auto& music = g_fxo->get<music_state>();
	music.func = func;
	music.userData = userData;
	music.current_selection_context = {};

	sysutil_register_cb([=, &music](ppu_thread& ppu) -> s32
	{
		music.func(ppu, CELL_MUSIC2_EVENT_INITIALIZE_RESULT, vm::addr_t(CELL_OK), userData);
		return CELL_OK;
	});

	return CELL_OK;
}

error_code cellMusicSetVolume(f32 level)
{
	cellMusic.warning("cellMusicSetVolume(level=%f)", level);

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
	cellMusic.warning("cellMusicGetVolume2(level=*0x%x)", level);

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
