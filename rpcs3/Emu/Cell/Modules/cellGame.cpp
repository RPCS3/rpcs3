#include "stdafx.h"
#include "Emu/localized_string.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/Cell/lv2/sys_fs.h"
#include "Emu/Cell/lv2/sys_sync.h"

#include "cellSysutil.h"
#include "cellMsgDialog.h"
#include "cellGame.h"

#include "Loader/PSF.h"
#include "Utilities/StrUtil.h"
#include "util/init_mutex.hpp"
#include "util/asm.hpp"
#include "Crypto/utils.h"

#include <span>

LOG_CHANNEL(cellGame);

vm::gvar<CellHddGameStatGet> g_stat_get;
vm::gvar<CellHddGameStatSet> g_stat_set;
vm::gvar<CellHddGameSystemFileParam> g_file_param;
vm::gvar<CellHddGameCBResult> g_cb_result;

stx::init_lock acquire_lock(stx::init_mutex& mtx, ppu_thread* ppu = nullptr);
stx::access_lock acquire_access_lock(stx::init_mutex& mtx, ppu_thread* ppu = nullptr);
stx::reset_lock acquire_reset_lock(stx::init_mutex& mtx, ppu_thread* ppu = nullptr);

template<>
void fmt_class_string<CellGameError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_GAME_ERROR_NOTFOUND);
		STR_CASE(CELL_GAME_ERROR_BROKEN);
		STR_CASE(CELL_GAME_ERROR_INTERNAL);
		STR_CASE(CELL_GAME_ERROR_PARAM);
		STR_CASE(CELL_GAME_ERROR_NOAPP);
		STR_CASE(CELL_GAME_ERROR_ACCESS_ERROR);
		STR_CASE(CELL_GAME_ERROR_NOSPACE);
		STR_CASE(CELL_GAME_ERROR_NOTSUPPORTED);
		STR_CASE(CELL_GAME_ERROR_FAILURE);
		STR_CASE(CELL_GAME_ERROR_BUSY);
		STR_CASE(CELL_GAME_ERROR_IN_SHUTDOWN);
		STR_CASE(CELL_GAME_ERROR_INVALID_ID);
		STR_CASE(CELL_GAME_ERROR_EXIST);
		STR_CASE(CELL_GAME_ERROR_NOTPATCH);
		STR_CASE(CELL_GAME_ERROR_INVALID_THEME_FILE);
		STR_CASE(CELL_GAME_ERROR_BOOTPATH);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellGameDataError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(CELL_GAMEDATA_ERROR_CBRESULT);
		STR_CASE(CELL_GAMEDATA_ERROR_ACCESS_ERROR);
		STR_CASE(CELL_GAMEDATA_ERROR_INTERNAL);
		STR_CASE(CELL_GAMEDATA_ERROR_PARAM);
		STR_CASE(CELL_GAMEDATA_ERROR_NOSPACE);
		STR_CASE(CELL_GAMEDATA_ERROR_BROKEN);
		STR_CASE(CELL_GAMEDATA_ERROR_FAILURE);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellDiscGameError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_DISCGAME_ERROR_INTERNAL);
			STR_CASE(CELL_DISCGAME_ERROR_NOT_DISCBOOT);
			STR_CASE(CELL_DISCGAME_ERROR_PARAM);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellHddGameError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_HDDGAME_ERROR_CBRESULT);
			STR_CASE(CELL_HDDGAME_ERROR_ACCESS_ERROR);
			STR_CASE(CELL_HDDGAME_ERROR_INTERNAL);
			STR_CASE(CELL_HDDGAME_ERROR_PARAM);
			STR_CASE(CELL_HDDGAME_ERROR_NOSPACE);
			STR_CASE(CELL_HDDGAME_ERROR_BROKEN);
			STR_CASE(CELL_HDDGAME_ERROR_FAILURE);
		}

		return unknown;
	});
}

// If dir is empty:
// contentInfo = "/dev_bdvd/PS3_GAME"
// usrdir = "/dev_bdvd/PS3_GAME/USRDIR"
// Temporary content directory (dir is not empty):
// contentInfo = "/dev_hdd0/game/_GDATA_" + time_since_epoch
// usrdir = "/dev_hdd0/game/_GDATA_" + time_since_epoch + "/USRDIR"
// Normal content directory (dir is not empty):
// contentInfo = "/dev_hdd0/game/" + dir
// usrdir = "/dev_hdd0/game/" + dir + "/USRDIR"
struct content_permission final
{
	// Content directory name or path
	std::string dir;

	// SFO file
	psf::registry sfo;

	// Temporary directory path
	std::string temp;

	stx::init_mutex init;

	enum class check_mode
	{
		not_set,
		game_data,
		patch,
		hdd_game,
		disc_game
	};

	atomic_t<u32> can_create = 0;
	atomic_t<bool> exists = false;
	atomic_t<check_mode> mode = check_mode::not_set;

	content_permission() = default;

	content_permission(const content_permission&) = delete;

	content_permission& operator=(const content_permission&) = delete;

	void reset()
	{
		dir.clear();
		sfo.clear();
		temp.clear();
		can_create = 0;
		exists = false;
		mode = check_mode::not_set;
	}

	~content_permission()
	{
		bool success = false;
		fs::g_tls_error = fs::error::ok;

		if (temp.size() <= 1 || fs::remove_all(temp))
		{
			success = true;
		}

		if (!success)
		{
			cellGame.fatal("Failed to clean directory '%s' (%s)", temp, fs::g_tls_error);
		}
	}
};

template<>
void fmt_class_string<content_permission::check_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(content_permission::check_mode::not_set);
			STR_CASE(content_permission::check_mode::game_data);
			STR_CASE(content_permission::check_mode::patch);
			STR_CASE(content_permission::check_mode::hdd_game);
			STR_CASE(content_permission::check_mode::disc_game);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<disc_change_manager::eject_state>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(disc_change_manager::eject_state::unknown);
			STR_CASE(disc_change_manager::eject_state::inserted);
			STR_CASE(disc_change_manager::eject_state::ejected);
			STR_CASE(disc_change_manager::eject_state::busy);
		}

		return unknown;
	});
}

static bool check_system_ver(vm::cptr<char> systemVersion)
{
	// Only allow something like "04.8300".
	// The disassembly shows that "04.83" would also be considered valid, but the initial strlen check makes this void.
	return (
		systemVersion &&
		std::strlen(systemVersion.get_ptr()) == 7 &&
		std::isdigit(systemVersion[0]) &&
		std::isdigit(systemVersion[1]) &&
		systemVersion[2] == '.' &&
		std::isdigit(systemVersion[3]) &&
		std::isdigit(systemVersion[4]) &&
		std::isdigit(systemVersion[5]) &&
		std::isdigit(systemVersion[6])
	);
}

disc_change_manager::disc_change_manager()
{
	Emu.GetCallbacks().enable_disc_eject(false);
	Emu.GetCallbacks().enable_disc_insert(false);
}

disc_change_manager::~disc_change_manager()
{
	Emu.GetCallbacks().enable_disc_eject(false);
	Emu.GetCallbacks().enable_disc_insert(false);
}

error_code disc_change_manager::register_callbacks(vm::ptr<CellGameDiscEjectCallback> func_eject, vm::ptr<CellGameDiscInsertCallback> func_insert)
{
	std::lock_guard lock(mtx);

	eject_callback = func_eject;
	insert_callback = func_insert;

	const bool is_disc_mounted = fs::is_dir(vfs::get("/dev_bdvd/PS3_GAME"));

	if (state == eject_state::unknown)
	{
		state = is_disc_mounted ? eject_state::inserted : eject_state::ejected;
	}

	Emu.GetCallbacks().enable_disc_eject(!!func_eject && is_disc_mounted);
	Emu.GetCallbacks().enable_disc_insert(!!func_insert && !is_disc_mounted);

	return CELL_OK;
}

error_code disc_change_manager::unregister_callbacks()
{
	const auto unregister = [this]() -> void
	{
		eject_callback = vm::null;
		insert_callback = vm::null;

		Emu.GetCallbacks().enable_disc_eject(false);
		Emu.GetCallbacks().enable_disc_insert(false);
	};

	if (is_inserting)
	{
		// NOTE: The insert_callback is known to call cellGameUnregisterDiscChangeCallback.
		// So we keep it out of the mutex lock until it proves to be an issue.
		unregister();
	}
	else
	{
		std::lock_guard lock(mtx);
		unregister();
	}

	return CELL_OK;
}

void disc_change_manager::eject_disc()
{
	cellGame.notice("Ejecting disc...");

	std::lock_guard lock(mtx);

	if (state != eject_state::inserted)
	{
		cellGame.fatal("Can not eject disc in the current state. (state=%s)", state.load());
		return;
	}

	state = eject_state::busy;
	Emu.GetCallbacks().enable_disc_eject(false);

	ensure(eject_callback);

	sysutil_register_cb([](ppu_thread& cb_ppu) -> s32
	{
		auto& dcm = g_fxo->get<disc_change_manager>();
		std::lock_guard lock(dcm.mtx);

		cellGame.notice("Executing eject_callback...");
		dcm.eject_callback(cb_ppu);

		ensure(vfs::unmount("/dev_bdvd"));
		ensure(vfs::unmount("/dev_ps2disc"));
		dcm.state = eject_state::ejected;

		// Re-enable disc insertion only if the callback is still registered
		Emu.GetCallbacks().enable_disc_insert(!!dcm.insert_callback);

		return CELL_OK;
	});
}

void disc_change_manager::insert_disc(u32 disc_type, std::string title_id)
{
	cellGame.notice("Inserting disc...");

	std::lock_guard lock(mtx);

	if (state != eject_state::ejected)
	{
		cellGame.fatal("Can not insert disc in the current state. (state=%s)", state.load());
		return;
	}

	state = eject_state::busy;
	Emu.GetCallbacks().enable_disc_insert(false);

	ensure(insert_callback);

	is_inserting = true;

	sysutil_register_cb([disc_type, title_id = std::move(title_id)](ppu_thread& cb_ppu) -> s32
	{
		auto& dcm = g_fxo->get<disc_change_manager>();
		std::lock_guard lock(dcm.mtx);

		if (disc_type == CELL_GAME_DISCTYPE_PS3)
		{
			vm::var<char[]> _title_id = vm::make_str(title_id);
			cellGame.notice("Executing insert_callback for title '%s' with disc_type %d...", _title_id.get_ptr(), disc_type);
			dcm.insert_callback(cb_ppu, disc_type, _title_id);
		}
		else
		{
			cellGame.notice("Executing insert_callback with disc_type %d...", disc_type);
			dcm.insert_callback(cb_ppu, disc_type, vm::null);
		}

		dcm.state = eject_state::inserted;

		// Re-enable disc ejection only if the callback is still registered
		Emu.GetCallbacks().enable_disc_eject(!!dcm.eject_callback);

		dcm.is_inserting = false;

		return CELL_OK;
	});
}

extern void lv2_sleep(u64 timeout, ppu_thread* ppu = nullptr)
{
	if (!ppu)
	{
		ppu = ensure(cpu_thread::get_current<ppu_thread>());
	}

	if (!timeout)
	{
		return;
	}

	const bool had_wait = ppu->state.test_and_set(cpu_flag::wait);

	lv2_obj::sleep(*ppu);
	lv2_obj::wait_timeout(timeout);
	ppu->check_state();

	if (had_wait)
	{
		ppu->state += cpu_flag::wait;
	}
}

error_code cellHddGameCheck(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellHddGameStatCallback> funcStat, u32 container)
{
	cellGame.warning("cellHddGameCheck(version=%d, dirName=%s, errDialog=%d, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	if (version != CELL_GAMEDATA_VERSION_CURRENT || !dirName || !funcStat || sysutil_check_name_string(dirName.get_ptr(), 1, CELL_GAME_DIRNAME_SIZE) != 0)
	{
		return CELL_HDDGAME_ERROR_PARAM;
	}

	std::string game_dir = dirName.get_ptr();

	// TODO: Find error code
	ensure(game_dir.size() == 9);

	const std::string dir = "/dev_hdd0/game/" + game_dir;

	auto [sfo, psf_error] = psf::load(vfs::get(dir + "/PARAM.SFO"));

	const u32 new_data = psf_error == psf::error::stream ? CELL_GAMEDATA_ISNEWDATA_YES : CELL_GAMEDATA_ISNEWDATA_NO;

	if (!new_data)
	{
		const auto cat = psf::get_string(sfo, "CATEGORY", "");
		if (!psf::is_cat_hdd(cat))
		{
			return { CELL_GAMEDATA_ERROR_BROKEN, "CATEGORY='%s'", cat };
		}
	}

	const std::string usrdir = dir + "/USRDIR";

	auto& get = g_stat_get;
	auto& set = g_stat_set;
	auto& result = g_cb_result;

	std::memset(get.get_ptr(), 0, sizeof(*get));
	std::memset(set.get_ptr(), 0, sizeof(*set));
	std::memset(result.get_ptr(), 0, sizeof(*result));

	const std::string local_dir = vfs::get(dir);

	// 40 GB - 256 kilobytes. The reasoning is that many games take this number and multiply it by 1024, to get the amount of bytes. With 40GB exactly,
	// this will result in an overflow, and the size would be 0, preventing the game from running. By reducing 256 kilobytes, we make sure that even
	// after said overflow, the number would still be high enough to contain the game's data.
	get->hddFreeSizeKB = 40 * 1024 * 1024 - 256;
	get->isNewData = CELL_HDDGAME_ISNEWDATA_EXIST;
	get->sysSizeKB = 0; // TODO
	get->st_atime_ = 0; // TODO
	get->st_ctime_ = 0; // TODO
	get->st_mtime_ = 0; // TODO
	get->sizeKB = CELL_HDDGAME_SIZEKB_NOTCALC;
	strcpy_trunc(get->contentInfoPath, dir);
	strcpy_trunc(get->gameDataPath, usrdir);

	std::memset(g_file_param.get_ptr(), 0, sizeof(*g_file_param));
	set->setParam = g_file_param;

	if (!fs::is_dir(local_dir))
	{
		get->isNewData = CELL_HDDGAME_ISNEWDATA_NODIR;
		get->getParam = {};
	}
	else
	{
		// TODO: Is cellHddGameCheck really responsible for writing the information in get->getParam ? (If not, delete this else)
		const psf::registry psf = psf::load_object(local_dir + "/PARAM.SFO");

		// Some following fields may be zero in old FW 1.00 version PARAM.SFO
		if (psf.contains("PARENTAL_LEVEL")) get->getParam.parentalLevel = ::at32(psf, "PARENTAL_LEVEL").as_integer();
		if (psf.contains("ATTRIBUTE")) get->getParam.attribute = ::at32(psf, "ATTRIBUTE").as_integer();
		if (psf.contains("RESOLUTION")) get->getParam.resolution = ::at32(psf, "RESOLUTION").as_integer();
		if (psf.contains("SOUND_FORMAT")) get->getParam.soundFormat = ::at32(psf, "SOUND_FORMAT").as_integer();
		if (psf.contains("TITLE")) strcpy_trunc(get->getParam.title, ::at32(psf, "TITLE").as_string());
		if (psf.contains("APP_VER")) strcpy_trunc(get->getParam.dataVersion, ::at32(psf, "APP_VER").as_string());
		if (psf.contains("TITLE_ID")) strcpy_trunc(get->getParam.titleId, ::at32(psf, "TITLE_ID").as_string());

		for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
		{
			strcpy_trunc(get->getParam.titleLang[i], psf::get_string(psf, fmt::format("TITLE_%02d", i)));
		}
	}

	// TODO ?

	lv2_sleep(5000, &ppu);

	funcStat(ppu, result, get, set);

	std::string error_msg;

	switch (result->result)
	{
	case CELL_HDDGAME_CBRESULT_OK:
	{
		// Game confirmed that it wants to create directory
		const auto setParam = set->setParam;

		lv2_sleep(2000, &ppu);

		if (new_data)
		{
			if (!setParam)
			{
				return CELL_GAMEDATA_ERROR_PARAM;
			}

			if (!fs::create_path(vfs::get(usrdir)))
			{
				return {CELL_GAME_ERROR_ACCESS_ERROR, usrdir};
			}
		}

		// Nuked until correctly reversed engineered
		// if (setParam)
		// {
		// 	if (new_data)
		// 	{
		// 		psf::assign(sfo, "CATEGORY", psf::string(3, "HG"));
		// 	}

		// 	psf::assign(sfo, "TITLE_ID", psf::string(TITLEID_SFO_ENTRY_SIZE, setParam->titleId));
		// 	psf::assign(sfo, "TITLE", psf::string(CELL_GAME_SYSP_TITLE_SIZE, setParam->title));
		// 	psf::assign(sfo, "VERSION", psf::string(CELL_GAME_SYSP_VERSION_SIZE, setParam->dataVersion));
		// 	psf::assign(sfo, "PARENTAL_LEVEL", +setParam->parentalLevel);
		// 	psf::assign(sfo, "RESOLUTION", +setParam->resolution);
		// 	psf::assign(sfo, "SOUND_FORMAT", +setParam->soundFormat);

		// 	for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
		// 	{
		// 		if (!setParam->titleLang[i][0])
		// 		{
		// 			continue;
		// 		}

		// 		psf::assign(sfo, fmt::format("TITLE_%02d", i), psf::string(CELL_GAME_SYSP_TITLE_SIZE, setParam->titleLang[i]));
		// 	}

		// 	psf::save_object(fs::file(vfs::get(dir + "/PARAM.SFO"), fs::rewrite), sfo);
		// }
		return CELL_OK;
	}
	case CELL_HDDGAME_CBRESULT_OK_CANCEL:
		cellGame.warning("cellHddGameCheck(): callback returned CELL_HDDGAME_CBRESULT_OK_CANCEL");
		return CELL_OK;

	case CELL_HDDGAME_CBRESULT_ERR_NOSPACE:
		cellGame.error("cellHddGameCheck(): callback returned CELL_HDDGAME_CBRESULT_ERR_NOSPACE. Space Needed: %d KB", result->errNeedSizeKB);
		error_msg = get_localized_string(localized_string_id::CELL_HDD_GAME_CHECK_NOSPACE, fmt::format("%d", result->errNeedSizeKB).c_str());
		break;

	case CELL_HDDGAME_CBRESULT_ERR_BROKEN:
		cellGame.error("cellHddGameCheck(): callback returned CELL_HDDGAME_CBRESULT_ERR_BROKEN");
		error_msg = get_localized_string(localized_string_id::CELL_HDD_GAME_CHECK_BROKEN, game_dir.c_str());
		break;

	case CELL_HDDGAME_CBRESULT_ERR_NODATA:
		cellGame.error("cellHddGameCheck(): callback returned CELL_HDDGAME_CBRESULT_ERR_NODATA");
		error_msg = get_localized_string(localized_string_id::CELL_HDD_GAME_CHECK_NODATA, game_dir.c_str());
		break;

	case CELL_HDDGAME_CBRESULT_ERR_INVALID:
		cellGame.error("cellHddGameCheck(): callback returned CELL_HDDGAME_CBRESULT_ERR_INVALID. Error message: %s", result->invalidMsg);
		error_msg = get_localized_string(localized_string_id::CELL_HDD_GAME_CHECK_INVALID, fmt::format("%s", result->invalidMsg).c_str());
		break;

	default:
		cellGame.error("cellHddGameCheck(): callback returned unknown error (code=0x%x). Error message: %s", result->invalidMsg);
		error_msg = get_localized_string(localized_string_id::CELL_HDD_GAME_CHECK_INVALID, fmt::format("%s", result->invalidMsg).c_str());
		break;
	}

	if (errDialog == CELL_GAMEDATA_ERRDIALOG_ALWAYS) // Maybe != CELL_GAMEDATA_ERRDIALOG_NONE
	{
		// Yield before a blocking dialog is being spawned
		lv2_obj::sleep(ppu);

		// Get user confirmation by opening a blocking dialog
		error_code res = open_msg_dialog(true, CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK | CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON, vm::make_str(error_msg), msg_dialog_source::_cellGame);

		// Reschedule after a blocking dialog returns
		if (ppu.check_state())
		{
			return 0;
		}

		if (res != CELL_OK)
		{
			return CELL_GAMEDATA_ERROR_INTERNAL;
		}
	}
	else
	{
		lv2_sleep(2000, &ppu);
	}

	return CELL_HDDGAME_ERROR_CBRESULT;
}

error_code cellHddGameCheck2(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellHddGameStatCallback> funcStat, u32 container)
{
	cellGame.trace("cellHddGameCheck2()");

	// Identical function
	return cellHddGameCheck(ppu, version, dirName, errDialog, funcStat, container);
}

error_code cellHddGameGetSizeKB(ppu_thread& ppu, vm::ptr<u32> size)
{
	ppu.state += cpu_flag::wait;

	cellGame.warning("cellHddGameGetSizeKB(size=*0x%x)", size);

	if (!size)
	{
		return CELL_HDDGAME_ERROR_PARAM;
	}

	lv2_obj::sleep(ppu);

	const u64 start_sleep = ppu.start_time;

	const std::string local_dir = vfs::get(Emu.GetDir());

	const auto dirsz = fs::get_dir_size(local_dir, 1024);

	// This function is very slow by nature
	// TODO: Check if after first use the result is being cached so the sleep can be reduced in this case
	lv2_sleep(utils::sub_saturate<u64>(dirsz == umax ? 2000 : 200000, get_guest_system_time() - start_sleep), &ppu);

	if (dirsz == umax)
	{
		const auto error = fs::g_tls_error;

		if (fs::exists(local_dir))
		{
			cellGame.error("cellHddGameGetSizeKB(): Unknown failure on calculating directory '%s' size (%s)", local_dir, error);
		}

		return CELL_HDDGAME_ERROR_FAILURE;
	}

	ppu.check_state();
	*size = ::narrow<s32>(dirsz / 1024);

	return CELL_OK;
}

error_code cellHddGameSetSystemVer(vm::cptr<char> systemVersion)
{
	cellGame.todo("cellHddGameSetSystemVer(systemVersion=%s)", systemVersion);

	if (!check_system_ver(systemVersion))
	{
		return CELL_HDDGAME_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellHddGameExitBroken()
{
	cellGame.warning("cellHddGameExitBroken()");
	return open_exit_dialog(get_localized_string(localized_string_id::CELL_HDD_GAME_EXIT_BROKEN), true, msg_dialog_source::_cellGame);
}

error_code cellGameDataGetSizeKB(ppu_thread& ppu, vm::ptr<u32> size)
{
	ppu.state += cpu_flag::wait;

	cellGame.warning("cellGameDataGetSizeKB(size=*0x%x)", size);

	if (!size)
	{
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	lv2_obj::sleep(ppu);

	const u64 start_sleep = ppu.start_time;

	const std::string local_dir = vfs::get(Emu.GetDir());

	const auto dirsz = fs::get_dir_size(local_dir, 1024);

	// This function is very slow by nature
	// TODO: Check if after first use the result is being cached so the sleep can be reduced in this case
	lv2_sleep(utils::sub_saturate<u64>(dirsz == umax ? 2000 : 200000, get_guest_system_time() - start_sleep), &ppu);

	if (dirsz == umax)
	{
		const auto error = fs::g_tls_error;

		if (fs::exists(local_dir))
		{
			cellGame.error("cellGameDataGetSizeKB(): Unknown failure on calculating directory '%s' size (%s)", local_dir, error);
		}

		return CELL_GAMEDATA_ERROR_FAILURE;
	}

	ppu.check_state();
	*size = ::narrow<s32>(dirsz / 1024);

	return CELL_OK;
}

error_code cellGameDataSetSystemVer(vm::cptr<char> systemVersion)
{
	cellGame.todo("cellGameDataSetSystemVer(systemVersion=%s)", systemVersion);

	if (!check_system_ver(systemVersion))
	{
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellGameDataExitBroken()
{
	cellGame.warning("cellGameDataExitBroken()");
	return open_exit_dialog(get_localized_string(localized_string_id::CELL_GAME_DATA_EXIT_BROKEN), true, msg_dialog_source::_cellGame);
}

error_code cellGameBootCheck(vm::ptr<u32> type, vm::ptr<u32> attributes, vm::ptr<CellGameContentSize> size, vm::ptr<char[CELL_GAME_DIRNAME_SIZE]> dirName)
{
	cellGame.warning("cellGameBootCheck(type=*0x%x, attributes=*0x%x, size=*0x%x, dirName=*0x%x)", type, attributes, size, dirName);

	if (!type || !attributes)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	auto& perm = g_fxo->get<content_permission>();

	lv2_sleep(500);

	const auto init = acquire_lock(perm.init);

	if (!init)
	{
		return CELL_GAME_ERROR_BUSY;
	}

	std::string dir;
	psf::registry sfo;

	const std::string& cat = Emu.GetFakeCat();

	u32 _type{};

	if (cat == "DG")
	{
		perm.mode = content_permission::check_mode::disc_game;

		_type = CELL_GAME_GAMETYPE_DISC;
		*attributes = 0; // TODO
		// TODO: dirName might be a read only string when BootCheck is called on a disc game. (e.g. Ben 10 Ultimate Alien: Cosmic Destruction)

		sfo = psf::load_object(vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO"));
	}
	else if (cat == "GD")
	{
		perm.mode = content_permission::check_mode::patch;

		_type = CELL_GAME_GAMETYPE_DISC;
		*attributes = CELL_GAME_ATTRIBUTE_PATCH; // TODO

		sfo = psf::load_object(vfs::get(Emu.GetDir() + "PARAM.SFO"));
	}
	else
	{
		perm.mode = content_permission::check_mode::hdd_game;

		_type = CELL_GAME_GAMETYPE_HDD;
		*attributes = 0; // TODO

		sfo = psf::load_object(vfs::get(Emu.GetDir() + "PARAM.SFO"));
		dir = fmt::trim(Emu.GetDir().substr(fs::get_parent_dir_view(Emu.GetDir()).size() + 1), fs::delim);
	}

	*type = _type;

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40 * 1024 * 1024 - 256; // Read explanation in cellHddGameCheck

		// TODO: Calculate data size for HG and DG games, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 4;
	}

	if (_type == u32{CELL_GAME_GAMETYPE_HDD} && dirName)
	{
		ensure(dir.size() < CELL_GAME_DIRNAME_SIZE);
		strcpy_trunc(*dirName, dir);
	}

	perm.dir = std::move(dir);
	perm.sfo = std::move(sfo);
	perm.exists = true;

	return CELL_OK;
}

error_code cellGamePatchCheck(vm::ptr<CellGameContentSize> size, vm::ptr<void> reserved)
{
	cellGame.warning("cellGamePatchCheck(size=*0x%x, reserved=*0x%x)", size, reserved);

	lv2_sleep(5000);

	if (Emu.GetCat() != "GD")
	{
		return CELL_GAME_ERROR_NOTPATCH;
	}

	psf::registry sfo = psf::load_object(vfs::get(Emu.GetDir() + "PARAM.SFO"));

	auto& perm = g_fxo->get<content_permission>();

	const auto init = acquire_lock(perm.init);

	if (!init)
	{
		return CELL_GAME_ERROR_BUSY;
	}

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40 * 1024 * 1024 - 256; // Read explanation in cellHddGameCheck

		// TODO: Calculate data size for patch data, if necessary.
		size->sizeKB = CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0; // TODO
	}

	perm.mode = content_permission::check_mode::patch;
	perm.dir = Emu.GetTitleID();
	perm.sfo = std::move(sfo);
	perm.exists = true;

	return CELL_OK;
}

error_code cellGameDataCheck(u32 type, vm::cptr<char> dirName, vm::ptr<CellGameContentSize> size)
{
	cellGame.warning("cellGameDataCheck(type=%d, dirName=%s, size=*0x%x)", type, dirName, size);

	if ((type - 1) >= 3 || (type != CELL_GAME_GAMETYPE_DISC && !dirName))
	{
		return {CELL_GAME_ERROR_PARAM, type};
	}

	std::string name;

	if (type != CELL_GAME_GAMETYPE_DISC)
	{
		name = dirName.get_ptr();
	}

	const std::string dir = type == CELL_GAME_GAMETYPE_DISC ? "/dev_bdvd/PS3_GAME"s : "/dev_hdd0/game/" + name;

	// TODO: not sure what should be checked there

	auto& perm = g_fxo->get<content_permission>();

	auto init = acquire_lock(perm.init);

	if (!init)
	{
		lv2_sleep(300);
		return CELL_GAME_ERROR_BUSY;
	}

	// This function is incredibly slow, slower for DISC type and even if the game/disc data does not exist
	// Null size does not change it
	lv2_sleep(type == CELL_GAME_GAMETYPE_DISC ? 300000 : 120000);

	auto [sfo, psf_error] = psf::load(vfs::get(dir + "/PARAM.SFO"));

	if (const std::string_view cat = psf::get_string(sfo, "CATEGORY"); [&]()
	{
		switch (type)
		{
		case CELL_GAME_GAMETYPE_HDD: return !psf::is_cat_hdd(cat);
		case CELL_GAME_GAMETYPE_GAMEDATA: return cat != "GD"sv;
		case CELL_GAME_GAMETYPE_DISC: return cat != "DG"sv;
		default: fmt::throw_exception("Unreachable");
		}
	}())
	{
		if (psf_error != psf::error::stream)
		{
			init.cancel();
			return {CELL_GAME_ERROR_BROKEN, "psf::error='%s', type='%d' CATEGORY='%s'", psf_error, type, cat};
		}
	}

	if (size)
	{
		// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
		size->hddFreeSizeKB = 40 * 1024 * 1024 - 256; // Read explanation in cellHddGameCheck

		// TODO: Calculate data size for game data, if necessary.
		size->sizeKB = sfo.empty() ? 0 : CELL_GAME_SIZEKB_NOTCALC;
		size->sysSizeKB = 0; // TODO
	}

	perm.dir = std::move(name);
	perm.can_create = type == CELL_GAME_GAMETYPE_GAMEDATA;
	perm.mode = content_permission::check_mode::game_data;

	if (sfo.empty())
	{
		cellGame.warning("cellGameDataCheck(): directory '%s' not found", dir);
		return not_an_error(CELL_GAME_RET_NONE);
	}

	perm.exists = true;
	perm.sfo = std::move(sfo);
	return CELL_OK;
}

error_code cellGameContentPermit(ppu_thread& ppu, vm::ptr<char[CELL_GAME_PATH_MAX]> contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> usrdirPath)
{
	cellGame.warning("cellGameContentPermit(contentInfoPath=*0x%x, usrdirPath=*0x%x)", contentInfoPath, usrdirPath);

	if (!contentInfoPath || !usrdirPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	auto& perm = g_fxo->get<content_permission>();

	const auto init = acquire_reset_lock(perm.init);

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const std::string dir = perm.dir.empty() ? "/dev_bdvd/PS3_GAME"s : "/dev_hdd0/game/" + perm.dir;

	if (perm.temp.empty() && !perm.exists)
	{
		perm.reset();
		strcpy_trunc(*contentInfoPath, "");
		strcpy_trunc(*usrdirPath, "");
		return CELL_OK;
	}

	lv2_obj::sleep(ppu);

	const u64 start_sleep = ppu.start_time;

	if (!perm.temp.empty())
	{
		std::vector<shared_ptr<lv2_file>> lv2_files;

		const std::string real_dir = vfs::get(dir) + "/";

		std::lock_guard lock(g_mp_sys_dev_hdd0.mutex);

		// Create PARAM.SFO
		fs::pending_file temp(perm.temp + "/PARAM.SFO");
		temp.file.write(psf::save_object(perm.sfo));
		ensure(temp.commit());

		idm::select<lv2_fs_object, lv2_file>([&](u32 id, lv2_file& file)
		{
			if (file.mp != &g_mp_sys_dev_hdd0)
			{
				return;
			}

			if (real_dir.starts_with(file.real_path))
			{
				if (!file.file)
				{
					return;
				}

				if (file.flags & CELL_FS_O_ACCMODE)
				{
					// Synchronize outside IDM lock scope
					lv2_files.emplace_back(ensure(idm::get_unlocked<lv2_fs_object, lv2_file>(id)));
				}
			}
		});

		for (auto& file : lv2_files)
		{
			// For atomicity
			file->file.sync();
		}

		// Make temporary directory persistent (atomically)
		if (vfs::host::rename(perm.temp, real_dir, &g_mp_sys_dev_hdd0, false, false))
		{
			cellGame.success("cellGameContentPermit(): directory '%s' has been created", dir);

			// Prevent cleanup
			perm.temp.clear();
		}
		else
		{
			cellGame.error("cellGameContentPermit(): failed to initialize directory '%s' (%s)", dir, fs::g_tls_error);
		}
	}
	else if (perm.can_create)
	{
		// Update PARAM.SFO
		fs::pending_file temp(vfs::get(dir + "/PARAM.SFO"));
		temp.file.write(psf::save_object(perm.sfo));
		ensure(temp.commit());
	}

	// This function is very slow by nature
	lv2_sleep(utils::sub_saturate<u64>(!perm.temp.empty() || perm.can_create ? 200000 : 2000, get_guest_system_time() - start_sleep), &ppu);

	// Cleanup
	perm.reset();

	strcpy_trunc(*contentInfoPath, dir);
	strcpy_trunc(*usrdirPath, dir + "/USRDIR");
	return CELL_OK;
}

error_code cellGameDataCheckCreate2(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellGameDataStatCallback> funcStat, u32 container)
{
	cellGame.success("cellGameDataCheckCreate2(version=0x%x, dirName=%s, errDialog=0x%x, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	//older sdk. it might not care about game type.

	if (version != CELL_GAMEDATA_VERSION_CURRENT || !funcStat || !dirName || sysutil_check_name_string(dirName.get_ptr(), 1, CELL_GAME_DIRNAME_SIZE) != 0)
	{
		return CELL_GAMEDATA_ERROR_PARAM;
	}

	const std::string game_dir = dirName.get_ptr();
	const std::string dir = "/dev_hdd0/game/"s + game_dir;

	auto [sfo, psf_error] = psf::load(vfs::get(dir + "/PARAM.SFO"));

	const u32 new_data = psf_error == psf::error::stream ? CELL_GAMEDATA_ISNEWDATA_YES : CELL_GAMEDATA_ISNEWDATA_NO;

	if (!new_data)
	{
		const auto cat = psf::get_string(sfo, "CATEGORY", "");
		if (cat != "GD" && cat != "DG")
		{
			return CELL_GAMEDATA_ERROR_BROKEN;
		}
	}

	const std::string usrdir = dir + "/USRDIR";

	auto& cbResult = g_cb_result;
	auto& cbGet = g_stat_get;
	auto& cbSet = g_stat_set;

	std::memset(cbGet.get_ptr(), 0, sizeof(*cbGet));
	std::memset(cbSet.get_ptr(), 0, sizeof(*cbSet));
	std::memset(cbResult.get_ptr(), 0, sizeof(*cbResult));

	cbGet->isNewData = new_data;

	// TODO: Use the free space of the computer's HDD where RPCS3 is being run.
	cbGet->hddFreeSizeKB = 40 * 1024 * 1024 - 256; // Read explanation in cellHddGameCheck

	strcpy_trunc(cbGet->contentInfoPath, dir);
	strcpy_trunc(cbGet->gameDataPath, usrdir);

	// TODO: set correct time
	cbGet->st_atime_ = 0;
	cbGet->st_ctime_ = 0;
	cbGet->st_mtime_ = 0;

	// TODO: calculate data size, if necessary
	cbGet->sizeKB = CELL_GAMEDATA_SIZEKB_NOTCALC;
	cbGet->sysSizeKB = 0; // TODO

	cbGet->getParam.attribute = CELL_GAMEDATA_ATTR_NORMAL;
	cbGet->getParam.parentalLevel = psf::get_integer(sfo, "PARENTAL_LEVEL", 0);
	strcpy_trunc(cbGet->getParam.dataVersion, psf::get_string(sfo, "APP_VER", psf::get_string(sfo, "VERSION", ""))); // Old games do not have APP_VER key
	strcpy_trunc(cbGet->getParam.titleId, psf::get_string(sfo, "TITLE_ID", ""));
	strcpy_trunc(cbGet->getParam.title, psf::get_string(sfo, "TITLE", ""));
	for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
	{
		strcpy_trunc(cbGet->getParam.titleLang[i], psf::get_string(sfo, fmt::format("TITLE_%02d", i)));
	}

	lv2_sleep(5000, &ppu);

	funcStat(ppu, cbResult, cbGet, cbSet);

	std::string error_msg;

	switch (cbResult->result)
	{
	case CELL_GAMEDATA_CBRESULT_OK_CANCEL:
	{
		cellGame.warning("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_OK_CANCEL");
		return CELL_OK;
	}
	case CELL_GAMEDATA_CBRESULT_OK:
	{
		// Game confirmed that it wants to create directory
		const auto setParam = cbSet->setParam;

		lv2_sleep(2000, &ppu);

		if (new_data)
		{
			if (!setParam)
			{
				return CELL_GAMEDATA_ERROR_PARAM;
			}

			if (!fs::create_path(vfs::get(usrdir)))
			{
				return {CELL_GAME_ERROR_ACCESS_ERROR, usrdir};
			}
		}

		if (setParam)
		{
			if (new_data)
			{
				psf::assign(sfo, "CATEGORY", psf::string(3, "GD"));
			}

			psf::assign(sfo, "TITLE_ID", psf::string(TITLEID_SFO_ENTRY_SIZE, setParam->titleId, true));
			psf::assign(sfo, "TITLE", psf::string(CELL_GAME_SYSP_TITLE_SIZE, setParam->title));
			psf::assign(sfo, "VERSION", psf::string(CELL_GAME_SYSP_VERSION_SIZE, setParam->dataVersion));
			psf::assign(sfo, "PARENTAL_LEVEL", +setParam->parentalLevel);

			for (u32 i = 0; i < CELL_HDDGAME_SYSP_LANGUAGE_NUM; i++)
			{
				if (!setParam->titleLang[i][0])
				{
					continue;
				}

				psf::assign(sfo, fmt::format("TITLE_%02d", i), psf::string(CELL_GAME_SYSP_TITLE_SIZE, setParam->titleLang[i]));
			}

			if (!psf::check_registry(sfo))
			{
				// This results in CELL_OK, broken SFO and CELL_GAMEDATA_ERROR_BROKEN on the next load
				// Avoid creation for now
				cellGame.error("Broken SFO paramters: %s", sfo);
				return CELL_OK;
			}

			fs::pending_file temp(vfs::get(dir + "/PARAM.SFO"));
			temp.file.write(psf::save_object(sfo));
			ensure(temp.commit());
		}

		return CELL_OK;
	}
	case CELL_GAMEDATA_CBRESULT_ERR_NOSPACE:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_NOSPACE. Space Needed: %d KB", cbResult->errNeedSizeKB);
		error_msg = get_localized_string(localized_string_id::CELL_GAMEDATA_CHECK_NOSPACE, fmt::format("%d", cbResult->errNeedSizeKB).c_str());
		break;

	case CELL_GAMEDATA_CBRESULT_ERR_BROKEN:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_BROKEN");
		error_msg = get_localized_string(localized_string_id::CELL_GAMEDATA_CHECK_BROKEN, game_dir.c_str());
		break;

	case CELL_GAMEDATA_CBRESULT_ERR_NODATA:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_NODATA");
		error_msg = get_localized_string(localized_string_id::CELL_GAMEDATA_CHECK_NODATA, game_dir.c_str());
		break;

	case CELL_GAMEDATA_CBRESULT_ERR_INVALID:
		cellGame.error("cellGameDataCheckCreate2(): callback returned CELL_GAMEDATA_CBRESULT_ERR_INVALID. Error message: %s", cbResult->invalidMsg);
		error_msg = get_localized_string(localized_string_id::CELL_GAMEDATA_CHECK_INVALID, fmt::format("%s", cbResult->invalidMsg).c_str());
		break;

	default:
		cellGame.error("cellGameDataCheckCreate2(): callback returned unknown error (code=0x%x). Error message: %s", cbResult->invalidMsg);
		error_msg = get_localized_string(localized_string_id::CELL_GAMEDATA_CHECK_INVALID, fmt::format("%s", cbResult->invalidMsg).c_str());
		break;
	}

	if (errDialog == CELL_GAMEDATA_ERRDIALOG_ALWAYS)
	{
		// Yield before a blocking dialog is being spawned
		lv2_obj::sleep(ppu);

		// Get user confirmation by opening a blocking dialog
		error_code res = open_msg_dialog(true, CELL_MSGDIALOG_TYPE_SE_TYPE_ERROR | CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK | CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON, vm::make_str(error_msg), msg_dialog_source::_cellGame);

		// Reschedule after a blocking dialog returns
		if (ppu.check_state())
		{
			return 0;
		}

		if (res != CELL_OK)
		{
			return CELL_GAMEDATA_ERROR_INTERNAL;
		}
	}
	else
	{
		lv2_sleep(2000, &ppu);
	}

	return CELL_GAMEDATA_ERROR_CBRESULT;
}

error_code cellGameDataCheckCreate(ppu_thread& ppu, u32 version, vm::cptr<char> dirName, u32 errDialog, vm::ptr<CellGameDataStatCallback> funcStat, u32 container)
{
	cellGame.warning("cellGameDataCheckCreate(version=0x%x, dirName=%s, errDialog=0x%x, funcStat=*0x%x, container=%d)", version, dirName, errDialog, funcStat, container);

	// TODO: almost identical, the only difference is that this function will always calculate the size of game data
	return cellGameDataCheckCreate2(ppu, version, dirName, errDialog, funcStat, container);
}

error_code cellGameCreateGameData(vm::ptr<CellGameSetInitParams> init, vm::ptr<char[CELL_GAME_PATH_MAX]> tmp_contentInfoPath, vm::ptr<char[CELL_GAME_PATH_MAX]> tmp_usrdirPath)
{
	cellGame.success("cellGameCreateGameData(init=*0x%x, tmp_contentInfoPath=*0x%x, tmp_usrdirPath=*0x%x)", init, tmp_contentInfoPath, tmp_usrdirPath);

	if (!init)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	auto& perm = g_fxo->get<content_permission>();

	const auto _init = acquire_access_lock(perm.init);

	lv2_sleep(2000);

	if (!_init || perm.dir.empty())
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	if (!perm.can_create)
	{
		return CELL_GAME_ERROR_NOTSUPPORTED;
	}

	if (perm.exists)
	{
		return CELL_GAME_ERROR_EXIST;
	}

	// Account for for filesystem operations
	lv2_sleep(50'000);

	std::string dirname = "_GDATA_" + std::to_string(steady_clock::now().time_since_epoch().count());
	std::string tmp_contentInfo = "/dev_hdd0/game/" + dirname;
	std::string tmp_usrdir = "/dev_hdd0/game/" + dirname + "/USRDIR";

	if (!fs::create_dir(vfs::get(tmp_contentInfo)))
	{
		cellGame.error("cellGameCreateGameData(): failed to create directory '%s' (%s)", tmp_contentInfo, fs::g_tls_error);
		return CELL_GAME_ERROR_ACCESS_ERROR; // ???
	}

	// cellGameContentPermit should then move files in non-temporary location and return their non-temporary displacement
	if (tmp_contentInfoPath) strcpy_trunc(*tmp_contentInfoPath, tmp_contentInfo);

	if (!fs::create_dir(vfs::get(tmp_usrdir)))
	{
		cellGame.error("cellGameCreateGameData(): failed to create directory '%s' (%s)", tmp_usrdir, fs::g_tls_error);
		return CELL_GAME_ERROR_ACCESS_ERROR; // ???
	}

	if (tmp_usrdirPath) strcpy_trunc(*tmp_usrdirPath, tmp_usrdir);

	perm.temp = vfs::get(tmp_contentInfo);
	cellGame.success("cellGameCreateGameData(): temporary directory '%s' has been created", tmp_contentInfo);

	// Initial PARAM.SFO parameters (overwrite)
	perm.sfo =
	{
		{ "CATEGORY", psf::string(3, "GD") },
		{ "TITLE_ID", psf::string(TITLEID_SFO_ENTRY_SIZE, init->titleId) },
		{ "TITLE", psf::string(CELL_GAME_SYSP_TITLE_SIZE, init->title) },
		{ "VERSION", psf::string(CELL_GAME_SYSP_VERSION_SIZE, init->version) },
	};

	return CELL_OK;
}

error_code cellGameDeleteGameData(vm::cptr<char> dirName)
{
	cellGame.warning("cellGameDeleteGameData(dirName=%s)", dirName);

	if (!dirName)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const std::string name = dirName.get_ptr();
	const std::string dir = vfs::get("/dev_hdd0/game/"s + name);

	auto& perm = g_fxo->get<content_permission>();

	auto remove_gd = [&]() -> error_code
	{
		if (Emu.GetCat() == "GD" && Emu.GetDir().substr(Emu.GetDir().find_last_of('/') + 1) == vfs::escape(name))
		{
			// Boot patch cannot delete its own directory
			return CELL_GAME_ERROR_NOTSUPPORTED;
		}

		const auto [sfo, psf_error] = psf::load(dir + "/PARAM.SFO");

		if (psf::get_string(sfo, "CATEGORY") != "GD" && psf_error != psf::error::stream)
		{
			return {CELL_GAME_ERROR_NOTSUPPORTED, psf_error};
		}

		if (sfo.empty())
		{
			// Nothing to remove
			return CELL_GAME_ERROR_NOTFOUND;
		}

		if (auto id = psf::get_string(sfo, "TITLE_ID"); !id.empty() && id != Emu.GetTitleID())
		{
			cellGame.error("cellGameDeleteGameData(%s): Attempts to delete GameData with TITLE ID which does not match the program's (%s)", id, Emu.GetTitleID());
		}

		// Actually remove game data
		if (!vfs::host::remove_all(dir, rpcs3::utils::get_hdd0_dir(), &g_mp_sys_dev_hdd0, true))
		{
			return {CELL_GAME_ERROR_ACCESS_ERROR, dir};
		}

		return CELL_OK;
	};

	while (true)
	{
		// Obtain exclusive lock and cancel init
		auto _init = perm.init.init();

		if (!_init)
		{
			// Or access it
			if (auto access = acquire_access_lock(perm.init); access)
			{
				// Cannot remove it when it is accessed by cellGameDataCheck
				// If it is HG data then resort to remove_gd for ERROR_BROKEN
				if (perm.dir == name && perm.can_create)
				{
					return CELL_GAME_ERROR_NOTSUPPORTED;
				}

				return remove_gd();
			}
			else
			{
				// Reacquire lock
				continue;
			}
		}

		auto err = remove_gd();
		_init.cancel();
		return err;
	}
}

error_code cellGameGetParamInt(s32 id, vm::ptr<s32> value)
{
	cellGame.warning("cellGameGetParamInt(id=%d, value=*0x%x)", id, value);

	if (!value)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	lv2_sleep(2000);

	auto& perm = g_fxo->get<content_permission>();

	const auto init = acquire_access_lock(perm.init);

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	std::string key;

	switch(id)
	{
	case CELL_GAME_PARAMID_PARENTAL_LEVEL:  key = "PARENTAL_LEVEL"; break;
	case CELL_GAME_PARAMID_RESOLUTION:      key = "RESOLUTION";     break;
	case CELL_GAME_PARAMID_SOUND_FORMAT:    key = "SOUND_FORMAT";   break;
	default:
	{
		return CELL_GAME_ERROR_INVALID_ID;
	}
	}

	if (!perm.sfo.count(key))
	{
		// TODO: Check if special values need to be set here
		cellGame.warning("cellGameGetParamInt(): id=%d was not found", id);
	}

	*value = psf::get_integer(perm.sfo, key, 0);
	return CELL_OK;
}

// String key flags
enum class strkey_flag : u32
{
	get_game_data, // reading is allowed for game data PARAM.SFO
	set_game_data, // writing is allowed for game data PARAM.SFO
	get_other,     // reading is allowed for other types of PARAM.SFO
	//set_other,     // writing is allowed for other types of PARAM.SFO (not possible)

	__bitset_enum_max
};

struct string_key_info
{
public:
	string_key_info() = default;
	string_key_info(std::string_view _name, u32 _max_size, bs_t<strkey_flag> _flags)
		: name(_name), max_size(_max_size), flags(_flags)
	{}

	std::string_view name;
	u32 max_size = 0;

	inline bool is_supported(bool is_setter, content_permission::check_mode mode) const
	{
		switch (mode)
		{
		case content_permission::check_mode::game_data:
		case content_permission::check_mode::patch: // TODO: it's unclear if patch mode should also support these flags
		{
			return !!(flags & (is_setter ? strkey_flag::set_game_data : strkey_flag::get_game_data));
		}
		case content_permission::check_mode::hdd_game:
		case content_permission::check_mode::disc_game:
		{
			return !is_setter && (flags & (strkey_flag::get_other));
		}
		case content_permission::check_mode::not_set:
		{
			fmt::throw_exception("This should never happen!");
		}
		}

		return false; // Fixes some VS warning
	}

private:
	bs_t<strkey_flag> flags{}; // allowed operations
};

static string_key_info get_param_string_key(s32 id)
{
	switch (id)
	{
	case CELL_GAME_PARAMID_TITLE:                    return string_key_info("TITLE", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::get_other + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_DEFAULT:            return string_key_info("TITLE", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::get_other + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_JAPANESE:           return string_key_info("TITLE_00", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_ENGLISH:            return string_key_info("TITLE_01", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_FRENCH:             return string_key_info("TITLE_02", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_SPANISH:            return string_key_info("TITLE_03", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_GERMAN:             return string_key_info("TITLE_04", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_ITALIAN:            return string_key_info("TITLE_05", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_DUTCH:              return string_key_info("TITLE_06", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE:         return string_key_info("TITLE_07", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_RUSSIAN:            return string_key_info("TITLE_08", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_KOREAN:             return string_key_info("TITLE_09", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_CHINESE_T:          return string_key_info("TITLE_10", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_CHINESE_S:          return string_key_info("TITLE_11", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_FINNISH:            return string_key_info("TITLE_12", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_SWEDISH:            return string_key_info("TITLE_13", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_DANISH:             return string_key_info("TITLE_14", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_NORWEGIAN:          return string_key_info("TITLE_15", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_POLISH:             return string_key_info("TITLE_16", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_PORTUGUESE_BRAZIL:  return string_key_info("TITLE_17", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_ENGLISH_UK:         return string_key_info("TITLE_18", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);
	case CELL_GAME_PARAMID_TITLE_TURKISH:            return string_key_info("TITLE_19", CELL_GAME_SYSP_TITLE_SIZE, strkey_flag::get_game_data + strkey_flag::set_game_data);

	case CELL_GAME_PARAMID_TITLE_ID:                 return string_key_info("TITLE_ID", CELL_GAME_SYSP_TITLEID_SIZE, strkey_flag::get_game_data + strkey_flag::get_other);
	case CELL_GAME_PARAMID_VERSION:                  return string_key_info("VERSION", CELL_GAME_SYSP_VERSION_SIZE, strkey_flag::get_game_data);
	case CELL_GAME_PARAMID_PS3_SYSTEM_VER:           return string_key_info("PS3_SYSTEM_VER", CELL_GAME_SYSP_PS3_SYSTEM_VER_SIZE, {}); // TODO
	case CELL_GAME_PARAMID_APP_VER:                  return string_key_info("APP_VER", CELL_GAME_SYSP_APP_VER_SIZE, strkey_flag::get_game_data + strkey_flag::get_other);
	}

	return {};
}

error_code cellGameGetParamString(s32 id, vm::ptr<char> buf, u32 bufsize)
{
	cellGame.warning("cellGameGetParamString(id=%d, buf=*0x%x, bufsize=%d)", id, buf, bufsize);

	if (!buf || bufsize == 0)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	auto& perm = g_fxo->get<content_permission>();

	lv2_sleep(2000);

	const auto init = acquire_access_lock(perm.init);

	if (!init || perm.mode == content_permission::check_mode::not_set)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const auto key = get_param_string_key(id);

	if (key.name.empty())
	{
		return CELL_GAME_ERROR_INVALID_ID;
	}

	if (!key.is_supported(false, perm.mode))
	{
		// TODO: this error is possibly only returned during debug mode
		return { CELL_GAME_ERROR_NOTSUPPORTED, "id %d is not supported in the current check mode: %s", id, perm.mode.load() };
	}

	const auto value = psf::get_string(perm.sfo, key.name);

	if (value.empty() && !perm.sfo.count(std::string(key.name)))
	{
		// TODO: Check if special values need to be set here
		cellGame.warning("cellGameGetParamString(): id=%d was not found", id);
	}

	std::span dst(buf.get_ptr(), bufsize);
	strcpy_trunc(dst, value);
	return CELL_OK;
}

error_code cellGameSetParamString(s32 id, vm::cptr<char> buf)
{
	cellGame.warning("cellGameSetParamString(id=%d, buf=*0x%x %s)", id, buf, buf);

	if (!buf)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	lv2_sleep(2000);

	auto& perm = g_fxo->get<content_permission>();

	const auto init = acquire_access_lock(perm.init);

	if (!init || perm.mode == content_permission::check_mode::not_set)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	const auto key = get_param_string_key(id);

	if (key.name.empty())
	{
		return CELL_GAME_ERROR_INVALID_ID;
	}

	if (!perm.can_create || !key.is_supported(true, perm.mode))
	{
		return CELL_GAME_ERROR_NOTSUPPORTED;
	}

	psf::assign(perm.sfo, key.name, psf::string(key.max_size, buf.get_ptr()));

	return CELL_OK;
}

error_code cellGameGetSizeKB(ppu_thread& ppu, vm::ptr<s32> size)
{
	cellGame.warning("cellGameGetSizeKB(size=*0x%x)", size);

	if (!size)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	// Always reset to 0 at start
	*size = 0;
	ppu.state += cpu_flag::wait;

	auto& perm = g_fxo->get<content_permission>();

	const auto init = acquire_access_lock(perm.init);

	if (!init)
	{
		return CELL_GAME_ERROR_FAILURE;
	}

	lv2_obj::sleep(ppu);

	const u64 start_sleep = ppu.start_time;

	const std::string local_dir = !perm.temp.empty() ? perm.temp : vfs::get("/dev_hdd0/game/" + perm.dir);

	const auto dirsz = fs::get_dir_size(local_dir, 1024);

	// This function is very slow by nature
	// TODO: Check if after first use the result is being cached so the sleep can be reduced in this case
	lv2_sleep(utils::sub_saturate<u64>(dirsz == umax ? 1000 : 200000, get_guest_system_time() - start_sleep), &ppu);

	if (dirsz == umax)
	{
		const auto error = fs::g_tls_error;

		if (!fs::exists(local_dir))
		{
			return CELL_OK;
		}
		else
		{
			cellGame.error("cellGameGetSizeKb(): Unknown failure on calculating directory size '%s' (%s)", local_dir, error);
			return CELL_GAME_ERROR_ACCESS_ERROR;
		}
	}

	ppu.check_state();
	*size = ::narrow<s32>(dirsz / 1024);

	return CELL_OK;
}

error_code cellGameGetDiscContentInfoUpdatePath(vm::ptr<char> updatePath)
{
	cellGame.todo("cellGameGetDiscContentInfoUpdatePath(updatePath=*0x%x)", updatePath);

	if (!updatePath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellGameGetLocalWebContentPath(vm::ptr<char> contentPath)
{
	cellGame.todo("cellGameGetLocalWebContentPath(contentPath=*0x%x)", contentPath);

	if (!contentPath)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	return CELL_OK;
}

error_code cellGameContentErrorDialog(s32 type, s32 errNeedSizeKB, vm::cptr<char> dirName)
{
	cellGame.warning("cellGameContentErrorDialog(type=%d, errNeedSizeKB=%d, dirName=%s)", type, errNeedSizeKB, dirName);

	std::string error_msg;

	switch (type)
	{
	case CELL_GAME_ERRDIALOG_BROKEN_GAMEDATA:
		// Game data is corrupted. The application will continue.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_BROKEN_GAMEDATA);
		break;
	case CELL_GAME_ERRDIALOG_BROKEN_HDDGAME:
		// HDD boot game is corrupted. The application will continue.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_BROKEN_HDDGAME);
		break;
	case CELL_GAME_ERRDIALOG_NOSPACE:
		// Not enough available space. The application will continue.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_NOSPACE, fmt::format("%d", errNeedSizeKB).c_str());
		break;
	case CELL_GAME_ERRDIALOG_BROKEN_EXIT_GAMEDATA:
		// Game data is corrupted. The application will be terminated.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_BROKEN_EXIT_GAMEDATA);
		break;
	case CELL_GAME_ERRDIALOG_BROKEN_EXIT_HDDGAME:
		// HDD boot game is corrupted. The application will be terminated.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_BROKEN_EXIT_HDDGAME);
		break;
	case CELL_GAME_ERRDIALOG_NOSPACE_EXIT:
		// Not enough available space. The application will be terminated.
		error_msg = get_localized_string(localized_string_id::CELL_GAME_ERROR_NOSPACE_EXIT, fmt::format("%d", errNeedSizeKB).c_str());
		break;
	default:
		return CELL_GAME_ERROR_PARAM;
	}

	if (dirName)
	{
		if (!memchr(dirName.get_ptr(), '\0', CELL_GAME_DIRNAME_SIZE))
		{
			return CELL_GAME_ERROR_PARAM;
		}

		error_msg += '\n';
		error_msg += get_localized_string(localized_string_id::CELL_GAME_ERROR_DIR_NAME, fmt::format("%s", dirName).c_str());
	}

	return open_exit_dialog(error_msg, type > CELL_GAME_ERRDIALOG_NOSPACE, msg_dialog_source::_cellGame);
}

error_code cellGameThemeInstall(vm::cptr<char> usrdirPath, vm::cptr<char> fileName, u32 option)
{
	cellGame.todo("cellGameThemeInstall(usrdirPath=%s, fileName=%s, option=0x%x)", usrdirPath, fileName, option);

	if (!usrdirPath || !fileName || !memchr(usrdirPath.get_ptr(), '\0', CELL_GAME_PATH_MAX) || option > CELL_GAME_THEME_OPTION_APPLY)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const std::string src_path = vfs::get(fmt::format("%s/%s", usrdirPath, fileName));

	// Use hash to get a hopefully unique filename
	std::string hash;

	if (fs::file theme = fs::file(src_path))
	{
		u32 magic{};

		if (src_path.ends_with(".p3t") || !theme.read(magic) || magic != "P3TF"_u32)
		{
			return CELL_GAME_ERROR_INVALID_THEME_FILE;
		}

		hash = sha256_get_hash(theme.to_string().c_str(), theme.size(), true);
	}
	else
	{
		return CELL_GAME_ERROR_NOTFOUND;
	}

	const std::string dst_path = vfs::get(fmt::format("/dev_hdd0/theme/%s_%s.p3t", Emu.GetTitleID(), hash)); // TODO: this is renamed with some other scheme

	if (fs::is_file(dst_path))
	{
		cellGame.notice("cellGameThemeInstall: theme already installed: '%s'", dst_path);
	}
	else
	{
		cellGame.notice("cellGameThemeInstall: copying theme from '%s' to '%s'", src_path, dst_path);

		if (!fs::copy_file(src_path, dst_path, false)) // TODO: new file is write protected
		{
			cellGame.error("cellGameThemeInstall: failed to copy theme from '%s' to '%s' (error=%s)", src_path, dst_path, fs::g_tls_error);
			return CELL_GAME_ERROR_ACCESS_ERROR;
		}
	}

	if (false && !fs::remove_file(src_path)) // TODO: disabled for now
	{
		cellGame.error("cellGameThemeInstall: failed to remove source theme from '%s' (error=%s)", src_path, fs::g_tls_error);
	}

	if (option == CELL_GAME_THEME_OPTION_APPLY)
	{
		// TODO: apply new theme
	}

	return CELL_OK;
}

error_code cellGameThemeInstallFromBuffer(ppu_thread& ppu, u32 fileSize, u32 bufSize, vm::ptr<void> buf, vm::ptr<CellGameThemeInstallCallback> func, u32 option)
{
	cellGame.todo("cellGameThemeInstallFromBuffer(fileSize=%d, bufSize=%d, buf=*0x%x, func=*0x%x, option=0x%x)", fileSize, bufSize, buf, func, option);

	if (!buf || !fileSize || (fileSize > bufSize && !func) || bufSize < CELL_GAME_THEMEINSTALL_BUFSIZE_MIN || option > CELL_GAME_THEME_OPTION_APPLY)
	{
		return CELL_GAME_ERROR_PARAM;
	}

	const std::string hash = sha256_get_hash(reinterpret_cast<char*>(buf.get_ptr()), fileSize, true);
	const std::string dst_path = vfs::get(fmt::format("/dev_hdd0/theme/%s_%s.p3t", Emu.GetTitleID(), hash)); // TODO: this is renamed with some scheme

	if (fs::file theme = fs::file(dst_path, fs::write_new + fs::isfile)) // TODO: new file is write protected
	{
		const u32 magic = *reinterpret_cast<u32*>(buf.get_ptr());

		if (magic != "P3TF"_u32)
		{
			return CELL_GAME_ERROR_INVALID_THEME_FILE;
		}

		if (func && bufSize < fileSize)
		{
			cellGame.notice("cellGameThemeInstallFromBuffer: writing theme with func callback to '%s'", dst_path);

			for (u32 file_offset = 0; file_offset < fileSize;)
			{
				const u32 read_size = std::min(bufSize, fileSize - file_offset);
				cellGame.notice("cellGameThemeInstallFromBuffer: writing %d bytes at pos %d", read_size, file_offset);

				if (theme.write(reinterpret_cast<u8*>(buf.get_ptr()) + file_offset, read_size) != read_size)
				{
					cellGame.error("cellGameThemeInstallFromBuffer: failed to write to destination file '%s' (error=%s)", dst_path, fs::g_tls_error);

					if (fs::g_tls_error == fs::error::nospace)
					{
						return CELL_GAME_ERROR_NOSPACE;
					}

					return CELL_GAME_ERROR_ACCESS_ERROR;
				}

				file_offset += read_size;

				// Report status with callback
				cellGame.notice("cellGameThemeInstallFromBuffer: func(fileOffset=%d, readSize=%d, buf=0x%x)", file_offset, read_size, buf);
				const s32 result = func(ppu, file_offset, read_size, buf);

				if (result == CELL_GAME_RET_CANCEL) // same as CELL_GAME_CBRESULT_CANCEL
				{
					cellGame.notice("cellGameThemeInstallFromBuffer: theme installation was cancelled");
					return not_an_error(CELL_GAME_RET_CANCEL);
				}
			}
		}
		else
		{
			cellGame.notice("cellGameThemeInstallFromBuffer: writing theme to '%s'", dst_path);

			if (theme.write(buf.get_ptr(), fileSize) != fileSize)
			{
				cellGame.error("cellGameThemeInstallFromBuffer: failed to write to destination file '%s' (error=%s)", dst_path, fs::g_tls_error);

				if (fs::g_tls_error == fs::error::nospace)
				{
					return CELL_GAME_ERROR_NOSPACE;
				}

				return CELL_GAME_ERROR_ACCESS_ERROR;
			}
		}
	}
	else if (fs::g_tls_error == fs::error::exist) // Do not overwrite files, but continue.
	{
		cellGame.notice("cellGameThemeInstallFromBuffer: theme already installed: '%s'", dst_path);
	}
	else
	{
		cellGame.error("cellGameThemeInstallFromBuffer: failed to open destination file '%s' (error=%s)", dst_path, fs::g_tls_error);
		return CELL_GAME_ERROR_ACCESS_ERROR;
	}

	if (option == CELL_GAME_THEME_OPTION_APPLY)
	{
		// TODO: apply new theme
	}

	return CELL_OK;
}

error_code cellDiscGameGetBootDiscInfo(vm::ptr<CellDiscGameSystemFileParam> getParam)
{
	cellGame.warning("cellDiscGameGetBootDiscInfo(getParam=*0x%x)", getParam);

	if (!getParam)
	{
		return CELL_DISCGAME_ERROR_PARAM;
	}

	// Always sets 0 at first dword
	write_to_ptr<u32>(getParam->titleId, 0);

	lv2_sleep(2000);

	// This is also called by non-disc games, see NPUB90029
	static const std::string dir = "/dev_bdvd/PS3_GAME"s;

	if (!fs::is_dir(vfs::get(dir)))
	{
		return CELL_DISCGAME_ERROR_NOT_DISCBOOT;
	}

	const psf::registry psf = psf::load_object(vfs::get(dir + "/PARAM.SFO"));

	if (psf.contains("PARENTAL_LEVEL")) getParam->parentalLevel = ::at32(psf, "PARENTAL_LEVEL").as_integer();
	if (psf.contains("TITLE_ID")) strcpy_trunc(getParam->titleId, ::at32(psf, "TITLE_ID").as_string());

	return CELL_OK;
}

error_code cellDiscGameRegisterDiscChangeCallback(vm::ptr<CellDiscGameDiscEjectCallback> funcEject, vm::ptr<CellDiscGameDiscInsertCallback> funcInsert)
{
	cellGame.warning("cellDiscGameRegisterDiscChangeCallback(funcEject=*0x%x, funcInsert=*0x%x)", funcEject, funcInsert);

	return g_fxo->get<disc_change_manager>().register_callbacks(funcEject, funcInsert);
}

error_code cellDiscGameUnregisterDiscChangeCallback()
{
	cellGame.warning("cellDiscGameUnregisterDiscChangeCallback()");

	return g_fxo->get<disc_change_manager>().unregister_callbacks();
}

error_code cellGameRegisterDiscChangeCallback(vm::ptr<CellGameDiscEjectCallback> funcEject, vm::ptr<CellGameDiscInsertCallback> funcInsert)
{
	cellGame.warning("cellGameRegisterDiscChangeCallback(funcEject=*0x%x, funcInsert=*0x%x)", funcEject, funcInsert);

	return g_fxo->get<disc_change_manager>().register_callbacks(funcEject, funcInsert);
}

error_code cellGameUnregisterDiscChangeCallback()
{
	cellGame.warning("cellGameUnregisterDiscChangeCallback()");

	return g_fxo->get<disc_change_manager>().unregister_callbacks();
}

void cellSysutil_GameData_init()
{
	REG_FUNC(cellSysutil, cellHddGameCheck);
	REG_FUNC(cellSysutil, cellHddGameCheck2);
	REG_FUNC(cellSysutil, cellHddGameGetSizeKB);
	REG_FUNC(cellSysutil, cellHddGameSetSystemVer);
	REG_FUNC(cellSysutil, cellHddGameExitBroken);

	REG_FUNC(cellSysutil, cellGameDataGetSizeKB);
	REG_FUNC(cellSysutil, cellGameDataSetSystemVer);
	REG_FUNC(cellSysutil, cellGameDataExitBroken);

	REG_FUNC(cellSysutil, cellGameDataCheckCreate);
	REG_FUNC(cellSysutil, cellGameDataCheckCreate2);

	REG_FUNC(cellSysutil, cellDiscGameGetBootDiscInfo);
	REG_FUNC(cellSysutil, cellDiscGameRegisterDiscChangeCallback);
	REG_FUNC(cellSysutil, cellDiscGameUnregisterDiscChangeCallback);
	REG_FUNC(cellSysutil, cellGameRegisterDiscChangeCallback);
	REG_FUNC(cellSysutil, cellGameUnregisterDiscChangeCallback);
}

DECLARE(ppu_module_manager::cellGame)("cellGame", []()
{
	REG_FUNC(cellGame, cellGameBootCheck);
	REG_FUNC(cellGame, cellGamePatchCheck);
	REG_FUNC(cellGame, cellGameDataCheck);
	REG_FUNC(cellGame, cellGameContentPermit);

	REG_FUNC(cellGame, cellGameCreateGameData);
	REG_FUNC(cellGame, cellGameDeleteGameData);

	REG_FUNC(cellGame, cellGameGetParamInt);
	REG_FUNC(cellGame, cellGameGetParamString);
	REG_FUNC(cellGame, cellGameSetParamString);
	REG_FUNC(cellGame, cellGameGetSizeKB);
	REG_FUNC(cellGame, cellGameGetDiscContentInfoUpdatePath);
	REG_FUNC(cellGame, cellGameGetLocalWebContentPath);

	REG_FUNC(cellGame, cellGameContentErrorDialog);

	REG_FUNC(cellGame, cellGameThemeInstall);
	REG_FUNC(cellGame, cellGameThemeInstallFromBuffer);

	REG_VAR(cellGame, g_stat_get).flag(MFF_HIDDEN);
	REG_VAR(cellGame, g_stat_set).flag(MFF_HIDDEN);
	REG_VAR(cellGame, g_file_param).flag(MFF_HIDDEN);
	REG_VAR(cellGame, g_cb_result).flag(MFF_HIDDEN);
});
