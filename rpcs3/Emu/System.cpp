#include "stdafx.h"
#include "Utilities/event.h"
#include "Utilities/bin_patch.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUCallback.h"
#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_prx.h"

#include "Emu/IdManager.h"
#include "Emu/RSX/GSRender.h"

#include "Loader/PSF.h"
#include "Loader/ELF.h"

#include "Utilities/StrUtil.h"

#include "../Crypto/unself.h"
#include "../Crypto/unpkg.h"
#include "yaml-cpp/yaml.h"

#include <thread>
#include <typeinfo>
#include <queue>

#include "Utilities/GDBDebugServer.h"

cfg_root g_cfg;

std::string g_cfg_defaults;

extern atomic_t<u32> g_thread_count;

extern u64 get_system_time();

extern void ppu_load_exec(const ppu_exec_object&);
extern void spu_load_exec(const spu_exec_object&);
extern void ppu_initialize(const ppu_module&);
extern void ppu_unload_prx(const lv2_prx&);
extern std::shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object&, const std::string&);

extern void network_thread_init();

fs::file g_tty;
atomic_t<s64> g_tty_size{0};

template <>
void fmt_class_string<mouse_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](mouse_handler value)
	{
		switch (value)
		{
		case mouse_handler::null: return "Null";
		case mouse_handler::basic: return "Basic";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<pad_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](pad_handler value)
	{
		switch (value)
		{
		case pad_handler::null: return "Null";
		case pad_handler::keyboard: return "Keyboard";
		case pad_handler::ds4: return "DualShock 4";
#ifdef _MSC_VER
		case pad_handler::xinput: return "XInput";
#endif
#ifdef _WIN32
		case pad_handler::mm: return "MMJoystick";
#endif
#ifdef HAVE_LIBEVDEV
		case pad_handler::evdev: return "Evdev";
#endif
		}

		return unknown;
	});
}

template <>
void fmt_class_string<video_renderer>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](video_renderer value)
	{
		switch (value)
		{
		case video_renderer::null: return "Null";
		case video_renderer::opengl: return "OpenGL";
		case video_renderer::vulkan: return "Vulkan";
#ifdef _MSC_VER
		case video_renderer::dx12: return "D3D12";
#endif
		}

		return unknown;
	});
}

template <>
void fmt_class_string<video_resolution>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](video_resolution value)
	{
		switch (value)
		{
		case video_resolution::_1080: return "1920x1080";
		case video_resolution::_720: return "1280x720";
		case video_resolution::_480: return "720x480";
		case video_resolution::_576: return "720x576";
		case video_resolution::_1600x1080: return "1600x1080";
		case video_resolution::_1440x1080: return "1440x1080";
		case video_resolution::_1280x1080: return "1280x1080";
		case video_resolution::_960x1080: return "960x1080";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<video_aspect>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](video_aspect value)
	{
		switch (value)
		{
		case video_aspect::_auto: return "Auto";
		case video_aspect::_4_3: return "4:3";
		case video_aspect::_16_9: return "16:9";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<keyboard_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](keyboard_handler value)
	{
		switch (value)
		{
		case keyboard_handler::null: return "Null";
		case keyboard_handler::basic: return "Basic";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<audio_renderer>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](audio_renderer value)
	{
		switch (value)
		{
		case audio_renderer::null: return "Null";
#ifdef _WIN32
		case audio_renderer::xaudio: return "XAudio2";
#endif
#ifdef HAVE_ALSA
		case audio_renderer::alsa: return "ALSA";
#endif
#ifdef HAVE_PULSE
		case audio_renderer::pulse: return "PulseAudio";
#endif
		case audio_renderer::openal: return "OpenAL";
		}

		return unknown;
	});
}

void Emulator::Init()
{
	if (!g_tty)
	{
		g_tty.open(fs::get_config_dir() + "TTY.log", fs::rewrite + fs::append);
	}

	idm::init();
	fxm::init();

	// Reset defaults, cache them
	g_cfg.from_default();
	g_cfg_defaults = g_cfg.to_string();

	// Reload global configuration
	g_cfg.from_string(fs::file(fs::get_config_dir() + "/config.yml", fs::read + fs::create).to_string());

	// Create directories
	const std::string emu_dir = GetEmuDir();
	const std::string dev_hdd0 = fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", emu_dir);
	const std::string dev_hdd1 = fmt::replace_all(g_cfg.vfs.dev_hdd1, "$(EmulatorDir)", emu_dir);
	const std::string dev_usb = fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir);

	fs::create_path(dev_hdd0);
	fs::create_dir(dev_hdd0 + "game/");
	fs::create_dir(dev_hdd0 + "game/TEST12345/");
	fs::create_dir(dev_hdd0 + "game/TEST12345/USRDIR/");
	fs::create_dir(dev_hdd0 + "game/.locks/");
	fs::create_dir(dev_hdd0 + "home/");
	fs::create_dir(dev_hdd0 + "home/00000001/");
	fs::create_dir(dev_hdd0 + "home/00000001/exdata/");
	fs::create_dir(dev_hdd0 + "home/00000001/savedata/");
	fs::create_dir(dev_hdd0 + "home/00000001/trophy/");
	fs::write_file(dev_hdd0 + "home/00000001/localusername", fs::create + fs::excl + fs::write, "User"s);
	fs::create_dir(dev_hdd0 + "disc/");
	fs::create_dir(dev_hdd1 + "cache/");
	fs::create_dir(dev_hdd1 + "game/");
	fs::create_path(dev_hdd1);
	fs::create_path(dev_usb);

#ifdef WITH_GDB_DEBUGGER
	fxm::make<GDBDebugServer>();
#endif
	// Initialize patch engine
	fxm::make_always<patch_engine>()->append(fs::get_config_dir() + "/patch.yml");
}

bool Emulator::BootGame(const std::string& path, bool direct, bool add_only)
{
	static const char* boot_list[] =
	{
		"/PS3_GAME/USRDIR/EBOOT.BIN",
		"/USRDIR/EBOOT.BIN",
		"/EBOOT.BIN",
		"/eboot.bin",
	};

	if (direct && fs::exists(path))
	{
		m_path = path;
		Load(add_only);
		return true;
	}

	for (std::string elf : boot_list)
	{
		elf = path + elf;

		if (fs::is_file(elf))
		{
			m_path = elf;
			Load(add_only);
			return true;
		}
	}

	return false;
}

bool Emulator::InstallPkg(const std::string& path)
{
	LOG_SUCCESS(GENERAL, "Installing package: %s", path);

	atomic_t<double> progress(0.);
	int int_progress = 0;
	{
		// Run PKG unpacking asynchronously
		scope_thread worker("PKG Installer", [&]
		{
			if (pkg_install(path, progress))
			{
				progress = 1.;
				return;
			}

			progress = -1.;
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), std::abs(progress) < 1.)
		{
			// TODO: update unified progress dialog
			double pval = progress;
			pval < 0 ? pval += 1. : pval;
			pval *= 100.;

			if (static_cast<int>(pval) > int_progress)
			{
				int_progress = static_cast<int>(pval);
				LOG_SUCCESS(GENERAL, "... %u%%", int_progress);
			}

			m_cb.process_events();
		}
	}

	if (progress >= 1.)
	{
		return true;
	}

	return false;
}

std::string Emulator::GetEmuDir()
{
	const std::string& emu_dir_ = g_cfg.vfs.emulator_dir;
	return emu_dir_.empty() ? fs::get_config_dir() : emu_dir_;
}

std::string Emulator::GetHddDir()
{
	return fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", GetEmuDir());
}

std::string Emulator::GetLibDir()
{
	return fmt::replace_all(g_cfg.vfs.dev_flash, "$(EmulatorDir)", GetEmuDir()) + "sys/external/";
}

void Emulator::SetForceBoot(bool force_boot)
{
	m_force_boot = force_boot;
}

void Emulator::Load(bool add_only)
{
	if (!IsStopped())
	{
		Stop();
	}

	try
	{
		Init();

		// Load game list (maps ABCD12345 IDs to /dev_bdvd/ locations)
		YAML::Node games = YAML::Load(fs::file{fs::get_config_dir() + "/games.yml", fs::read + fs::create}.to_string());

		if (!games.IsMap())
		{
			games.reset();
		}

		LOG_NOTICE(LOADER, "Path: %s", m_path);

		const std::string elf_dir = fs::get_parent_dir(m_path);

		// Load PARAM.SFO (TODO)
		const auto _psf = psf::load_object([&]() -> fs::file
		{
			if (fs::file sfov{elf_dir + "/sce_sys/param.sfo"})
			{
				return sfov;
			}

			if (fs::is_dir(m_path))
			{
				// Special case (directory scan)
				if (fs::file sfo{m_path + "/PS3_GAME/PARAM.SFO"})
				{
					return sfo;
				}

				return fs::file{m_path + "/PARAM.SFO"};
			}

			if (disc.size())
			{
				// Check previously used category before it's overwritten
				if (m_cat == "DG")
				{
					return fs::file{disc + "/PS3_GAME/PARAM.SFO"};
				}

				if (m_cat == "GD")
				{
					return fs::file{GetHddDir() + "game/" + m_title_id + "/PARAM.SFO"};
				}

				return fs::file{disc + "/PARAM.SFO"};
			}

			return fs::file{elf_dir + "/../PARAM.SFO"};
		}());
		m_title = psf::get_string(_psf, "TITLE", m_path.substr(m_path.find_last_of('/') + 1));
		m_title_id = psf::get_string(_psf, "TITLE_ID");
		m_cat = psf::get_string(_psf, "CATEGORY");

		for (auto& c : m_title)
		{
			// Replace newlines with spaces
			if (c == '\n')
				c = ' ';
		}

		if (!_psf.empty() && m_cat.empty())
		{
			LOG_FATAL(LOADER, "Corrupted PARAM.SFO found! Assuming category GD. Try reinstalling the game.");
			m_cat = "GD";
		}

		LOG_NOTICE(LOADER, "Title: %s", GetTitle());
		LOG_NOTICE(LOADER, "Serial: %s", GetTitleID());
		LOG_NOTICE(LOADER, "Category: %s", GetCat());

		// Initialize data/cache directory
		if (fs::is_dir(m_path))
		{
			m_cache_path = fs::get_config_dir() + "data/" + GetTitleID() + '/';
			LOG_NOTICE(LOADER, "Cache: %s", GetCachePath());
		}
		else
		{
			m_cache_path = fs::get_data_dir(m_title_id, m_path);
			LOG_NOTICE(LOADER, "Cache: %s", GetCachePath());
		}

		// Load custom config-0
		if (fs::file cfg_file{m_cache_path + "/config.yml"})
		{
			LOG_NOTICE(LOADER, "Applying custom config: %s/config.yml", m_cache_path);
			g_cfg.from_string(cfg_file.to_string());
		}

		// Load custom config-1
		if (fs::file cfg_file{fs::get_config_dir() + "data/" + m_title_id + "/config.yml"})
		{
			LOG_NOTICE(LOADER, "Applying custom config: data/%s/config.yml", m_title_id);
			g_cfg.from_string(cfg_file.to_string());
		}

		// Load custom config-2
		if (fs::file cfg_file{m_path + ".yml"})
		{
			LOG_NOTICE(LOADER, "Applying custom config: %s.yml", m_path);
			g_cfg.from_string(cfg_file.to_string());
		}

		LOG_NOTICE(LOADER, "Used configuration:\n%s\n", g_cfg.to_string());

		// Load patches from different locations
		fxm::check_unlocked<patch_engine>()->append(fs::get_config_dir() + "data/" + m_title_id + "/patch.yml");
		fxm::check_unlocked<patch_engine>()->append(m_cache_path + "/patch.yml");

		// Mount all devices
		const std::string emu_dir = GetEmuDir();
		const std::string home_dir = g_cfg.vfs.app_home;
		std::string bdvd_dir = g_cfg.vfs.dev_bdvd;

		// Mount default relative path to non-existent directory
		vfs::mount("", fs::get_config_dir() + "delete_this_dir/");
		vfs::mount("dev_hdd0", fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", emu_dir));
		vfs::mount("dev_hdd1", fmt::replace_all(g_cfg.vfs.dev_hdd1, "$(EmulatorDir)", emu_dir));
		vfs::mount("dev_flash", fmt::replace_all(g_cfg.vfs.dev_flash, "$(EmulatorDir)", emu_dir));
		vfs::mount("dev_usb", fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir));
		vfs::mount("dev_usb000", fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir));
		vfs::mount("app_home", home_dir.empty() ? elf_dir + '/' : fmt::replace_all(home_dir, "$(EmulatorDir)", emu_dir));

		// Special boot mode (directory scan)
		if (fs::is_dir(m_path))
		{
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			Run();
			m_force_boot = false;

			// Force LLVM recompiler
			g_cfg.core.ppu_decoder.from_default();

			// Workaround for analyser glitches
			vm::falloc(0x10000, 0xf0000, vm::main);

			return thread_ctrl::spawn("SPRX Loader", [this]
			{
				std::vector<std::string> dir_queue;
				dir_queue.emplace_back(m_path + '/');

				std::queue<std::shared_ptr<thread_ctrl>> thread_queue;
				const uint max_threads = std::thread::hardware_concurrency();

				// Find all .sprx files recursively (TODO: process .mself files)
				for (std::size_t i = 0; i < dir_queue.size(); i++)
				{
					if (Emu.IsStopped())
					{
						break;
					}

					LOG_NOTICE(LOADER, "Scanning directory: %s", dir_queue[i]);

					for (auto&& entry : fs::dir(dir_queue[i]))
					{
						if (Emu.IsStopped())
						{
							break;
						}

						if (entry.is_directory)
						{
							if (entry.name != "." && entry.name != "..")
							{
								dir_queue.emplace_back(dir_queue[i] + entry.name + '/');
							}

							continue;
						}

						// Check .sprx filename
						if (entry.name.size() >= 5 && fmt::to_upper(entry.name).compare(entry.name.size() - 5, 5, ".SPRX", 5) == 0)
						{
							if (entry.name == "libfs_155.sprx")
							{
								continue;
							}

							// Get full path
							const std::string path = dir_queue[i] + entry.name;

							LOG_NOTICE(LOADER, "Trying to load SPRX: %s", path);

							// Some files may fail to decrypt due to the lack of klic
							const ppu_prx_object obj = decrypt_self(fs::file(path));

							if (obj == elf_error::ok)
							{
								if (auto prx = ppu_load_prx(obj, path))
								{
									while (g_thread_count >= max_threads + 2)
									{
										std::this_thread::sleep_for(10ms);
									}

									thread_queue.emplace();

									thread_ctrl::spawn(thread_queue.back(), "Worker " + std::to_string(thread_queue.size()), [_prx = std::move(prx)]
									{
										ppu_initialize(*_prx);
										ppu_unload_prx(*_prx);
									});
								}
							}
							else
							{
								LOG_ERROR(LOADER, "Failed to load SPRX '%s' (%s)", path, obj.get_error());
							}
						}
					}
				}

				// Join every thread and print exceptions
				while (!thread_queue.empty())
				{
					try
					{
						thread_queue.front()->join();
					}
					catch (const std::exception& e)
					{
						LOG_FATAL(LOADER, "[%s] %s thrown: %s", thread_queue.front()->get_name(), typeid(e).name(), e.what());
					}

					thread_queue.pop();
				}

				// Exit "process"
				Emu.CallAfter([]
				{
					Emu.Stop();
				});
			});
		}

		// Detect boot location
		const std::string hdd0_game = vfs::get("/dev_hdd0/game/");
		const std::string hdd0_disc = vfs::get("/dev_hdd0/disc/");
		const std::size_t bdvd_pos = m_cat == "DG" && bdvd_dir.empty() && disc.empty() ? elf_dir.rfind("/PS3_GAME/") + 1 : 0;
		const bool from_hdd0_game = m_path.find(hdd0_game) != -1;

		if (bdvd_pos && from_hdd0_game)
		{
			// Booting disc game from wrong location
			LOG_ERROR(LOADER, "Disc game %s found at invalid location /dev_hdd0/game/", m_title_id);

			// Move and retry from correct location
			if (fs::rename(elf_dir + "/../../", hdd0_disc + elf_dir.substr(hdd0_game.size()) + "/../../", false))
			{
				LOG_SUCCESS(LOADER, "Disc game %s moved to special location /dev_hdd0/disc/", m_title_id);
				return m_path = hdd0_disc + m_path.substr(hdd0_game.size()), Load();
			}
			else
			{
				LOG_ERROR(LOADER, "Failed to move disc game %s to /dev_hdd0/disc/ (%s)", m_title_id, fs::g_tls_error);
				return;
			}
		}

		// Booting disc game
		if (bdvd_pos)
		{
			// Mount /dev_bdvd/ if necessary
			bdvd_dir = elf_dir.substr(0, bdvd_pos);
		}

		// Booting patch data
		if (m_cat == "GD" && bdvd_dir.empty() && disc.empty())
		{
			// Load /dev_bdvd/ from game list if available
			if (auto node = games[m_title_id])
			{
				bdvd_dir = node.Scalar();
			}
			else
			{
				LOG_FATAL(LOADER, "Disc directory not found. Try to run the game from the actual game disc directory.");
			}
		}

		// Check /dev_bdvd/
		if (disc.empty() && !bdvd_dir.empty() && fs::is_dir(bdvd_dir))
		{
			fs::file sfb_file;

			vfs::mount("dev_bdvd", bdvd_dir);
			LOG_NOTICE(LOADER, "Disc: %s", vfs::get("/dev_bdvd"));

			if (!sfb_file.open(vfs::get("/dev_bdvd/PS3_DISC.SFB")) || sfb_file.size() < 4 || sfb_file.read<u32>() != ".SFB"_u32)
			{
				LOG_ERROR(LOADER, "Invalid disc directory for the disc game %s", m_title_id);
				return;
			}

			const std::string bdvd_title_id = psf::get_string(psf::load_object(fs::file{vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO")}), "TITLE_ID");

			if (bdvd_title_id != m_title_id)
			{
				LOG_ERROR(LOADER, "Unexpected disc directory for the disc game %s (found %s)", m_title_id, bdvd_title_id);
				return;
			}

			// Store /dev_bdvd/ location
			games[m_title_id] = bdvd_dir;
			YAML::Emitter out;
			out << games;
			fs::file(fs::get_config_dir() + "/games.yml", fs::rewrite).write(out.c_str(), out.size());
		}
		else if (m_cat != "DG" && m_cat != "GD")
		{
			// Don't need /dev_bdvd
		}
		else if (m_cat == "DG" && from_hdd0_game)
		{
			vfs::mount("dev_bdvd/PS3_GAME", hdd0_game + m_path.substr(hdd0_game.size(), 10));
			LOG_NOTICE(LOADER, "Game: %s", vfs::get("/dev_bdvd/PS3_GAME"));
		}
		else if (disc.empty())
		{
			LOG_ERROR(LOADER, "Failed to mount disc directory for the disc game %s", m_title_id);
			return;
		}
		else
		{
			bdvd_dir = disc;
			vfs::mount("dev_bdvd", bdvd_dir);
			LOG_NOTICE(LOADER, "Disk: %s", vfs::get("/dev_bdvd"));
		}

		if (add_only)
		{
			LOG_NOTICE(LOADER, "Finished to add data to games.yml by boot for: %s", m_path);
			return;
		}

		// Install PKGDIR, INSDIR, PS3_EXTRA
		if (!bdvd_dir.empty())
		{
			const std::string ins_dir = vfs::get("/dev_bdvd/PS3_GAME/INSDIR/");
			const std::string pkg_dir = vfs::get("/dev_bdvd/PS3_GAME/PKGDIR/");
			const std::string extra_dir = vfs::get("/dev_bdvd/PS3_GAME/PS3_EXTRA/");
			fs::file lock_file;

			if (fs::is_dir(ins_dir) || fs::is_dir(pkg_dir) || fs::is_dir(extra_dir))
			{
				// Create lock file to prevent double installation
				lock_file.open(hdd0_game + ".locks/" + m_title_id, fs::read + fs::create + fs::excl);
			}

			if (lock_file && fs::is_dir(ins_dir))
			{
				LOG_NOTICE(LOADER, "Found INSDIR: %s", ins_dir);

				for (auto&& entry : fs::dir{ins_dir})
				{
					if (!entry.is_directory && ends_with(entry.name, ".PKG") && !InstallPkg(ins_dir + entry.name))
					{
						LOG_ERROR(LOADER, "Failed to install /dev_bdvd/PS3_GAME/INSDIR/%s", entry.name);
						return;
					}
				}
			}

			if (lock_file && fs::is_dir(pkg_dir))
			{
				LOG_NOTICE(LOADER, "Found PKGDIR: %s", pkg_dir);

				for (auto&& entry : fs::dir{pkg_dir})
				{
					if (entry.is_directory && entry.name.compare(0, 3, "PKG", 3) == 0)
					{
						const std::string pkg_file = pkg_dir + entry.name + "/INSTALL.PKG";

						if (fs::is_file(pkg_file) && !InstallPkg(pkg_file))
						{
							LOG_ERROR(LOADER, "Failed to install /dev_bdvd/PS3_GAME/PKGDIR/%s/INSTALL.PKG", entry.name);
							return;
						}
					}
				}
			}

			if (lock_file && fs::is_dir(extra_dir))
			{
				LOG_NOTICE(LOADER, "Found PS3_EXTRA: %s", extra_dir);

				for (auto&& entry : fs::dir{extra_dir})
				{
					if (entry.is_directory && entry.name[0] == 'D')
					{
						const std::string pkg_file = extra_dir + entry.name + "/DATA000.PKG";

						if (fs::is_file(pkg_file) && !InstallPkg(pkg_file))
						{
							LOG_ERROR(LOADER, "Failed to install /dev_bdvd/PS3_GAME/PKGDIR/%s/DATA000.PKG", entry.name);
							return;
						}
					}
				}
			}
		}

		// Check game updates
		const std::string hdd0_boot = hdd0_game + m_title_id + "/USRDIR/EBOOT.BIN";

		if (disc.empty() && m_cat == "DG" && fs::is_file(hdd0_boot))
		{
			// Booting game update
			LOG_SUCCESS(LOADER, "Updates found at /dev_hdd0/game/%s/!", m_title_id);
			return m_path = hdd0_boot, Load();
		}

		// Mount /host_root/ if necessary
		if (g_cfg.vfs.host_root)
		{
			vfs::mount("host_root", {});
		}

		// Open SELF or ELF
		fs::file elf_file(m_path);

		if (!elf_file)
		{
			LOG_ERROR(LOADER, "Failed to open executable: %s", m_path);
			return;
		}

		// Check SELF header
		if (elf_file.size() >= 4 && elf_file.read<u32>() == "SCE\0"_u32)
		{
			const std::string decrypted_path = m_cache_path + "boot.elf";

			fs::stat_t encrypted_stat = elf_file.stat();
			fs::stat_t decrypted_stat;

			// Check modification time and try to load decrypted ELF
			if (fs::stat(decrypted_path, decrypted_stat) && decrypted_stat.mtime == encrypted_stat.mtime)
			{
				elf_file.open(decrypted_path);
			}
			else
			{
				// Decrypt SELF
				elf_file = decrypt_self(std::move(elf_file), klic.empty() ? nullptr : klic.data());

				if (fs::file elf_out{decrypted_path, fs::rewrite})
				{
					elf_out.write(elf_file.to_vector<u8>());
					elf_out.close();
					fs::utime(decrypted_path, encrypted_stat.atime, encrypted_stat.mtime);
				}
				else
				{
					LOG_ERROR(LOADER, "Failed to create boot.elf");
				}
			}
		}

		ppu_exec_object ppu_exec;
		ppu_prx_object ppu_prx;
		spu_exec_object spu_exec;

		if (!elf_file)
		{
			LOG_ERROR(LOADER, "Failed to decrypt SELF: %s", m_path);
			return;
		}
		else if (ppu_exec.open(elf_file) == elf_error::ok)
		{
			// PS3 executable
			m_state = system_state::ready;
			GetCallbacks().on_ready();

			vm::init();

			if (argv.empty())
			{
				argv.resize(1);
			}

			if (argv[0].empty())
			{
				if (from_hdd0_game && m_cat == "DG")
				{
					argv[0] = "/dev_bdvd/PS3_GAME/" + m_path.substr(hdd0_game.size() + 10);
					m_dir = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size(), 10);
					LOG_NOTICE(LOADER, "Disc path: %s", m_dir);
				}
				else if (from_hdd0_game)
				{
					argv[0] = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size());
					m_dir = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size(), 10);
					LOG_NOTICE(LOADER, "Boot path: %s", m_dir);
				}
				else if (!bdvd_dir.empty() && fs::is_dir(bdvd_dir))
				{
					// Disc games are on /dev_bdvd/
					const std::size_t pos = m_path.rfind("PS3_GAME");
					argv[0] = "/dev_bdvd/" + m_path.substr(pos);
					m_dir = "/dev_bdvd/PS3_GAME/";
				}
				else
				{
					// For homebrew
					argv[0] = "/host_root/" + m_path;
					m_dir = "/host_root/" + elf_dir + '/';
				}

				LOG_NOTICE(LOADER, "Elf path: %s", argv[0]);
			}

			if (g_cfg.core.spu_debug)
			{
				fs::file log;

				if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit)
				{
					log.open(Emu.GetCachePath() + "SPUJIT.log", fs::rewrite);
				}

				if (g_cfg.core.spu_decoder == spu_decoder_type::llvm)
				{
					log.open(Emu.GetCachePath() + "SPU.log", fs::rewrite);
				}

				log.write(fmt::format("SPU JIT Log\n\nTitle: %s\nTitle ID: %s\n\n", Emu.GetTitle(), Emu.GetTitleID()));
				fs::create_dir(Emu.GetCachePath() + "SPU");
				fs::remove_all(Emu.GetCachePath() + "SPU", false);
			}

			ppu_load_exec(ppu_exec);

			fxm::import<GSRender>(Emu.GetCallbacks().get_gs_render); // TODO: must be created in appropriate sys_rsx syscall
			network_thread_init();
		}
		else if (ppu_prx.open(elf_file) == elf_error::ok)
		{
			// PPU PRX (experimental)
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			ppu_load_prx(ppu_prx, m_path);
		}
		else if (spu_exec.open(elf_file) == elf_error::ok)
		{
			// SPU executable (experimental)
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			spu_load_exec(spu_exec);
		}
		else
		{
			LOG_ERROR(LOADER, "Invalid or unsupported file format: %s", m_path);

			LOG_WARNING(LOADER, "** ppu_exec -> %s", ppu_exec.get_error());
			LOG_WARNING(LOADER, "** ppu_prx  -> %s", ppu_prx.get_error());
			LOG_WARNING(LOADER, "** spu_exec -> %s", spu_exec.get_error());
			return;
		}

		if ((m_force_boot || g_cfg.misc.autostart) && IsReady())
		{
			Run();
			m_force_boot = false;
		}
		else if (IsPaused())
		{
			m_state = system_state::ready;
			GetCallbacks().on_ready();
		}
	}
	catch (const std::exception& e)
	{
		LOG_FATAL(LOADER, "%s thrown: %s", typeid(e).name(), e.what());
		Stop();
	}
}

void Emulator::Run()
{
	if (!IsReady())
	{
		Load();
		if(!IsReady()) return;
	}

	if (IsRunning()) Stop();

	if (IsPaused())
	{
		Resume();
		return;
	}

	GetCallbacks().on_run();

	m_pause_start_time = 0;
	m_pause_amend_time = 0;
	m_state = system_state::running;

	auto on_select = [](u32, cpu_thread& cpu)
	{
		cpu.run();
	};

	idm::select<ppu_thread>(on_select);
	idm::select<RawSPUThread>(on_select);
	idm::select<SPUThread>(on_select);
}

bool Emulator::Pause()
{
	const u64 start = get_system_time();

	// Try to pause
	if (!m_state.compare_and_swap_test(system_state::running, system_state::paused))
	{
		return m_state.compare_and_swap_test(system_state::ready, system_state::paused);
	}

	GetCallbacks().on_pause();

	// Update pause start time
	if (m_pause_start_time.exchange(start))
	{
		LOG_ERROR(GENERAL, "Emulator::Pause() error: concurrent access");
	}

	auto on_select = [](u32, cpu_thread& cpu)
	{
		cpu.state += cpu_flag::dbg_global_pause;
	};

	idm::select<ppu_thread>(on_select);
	idm::select<RawSPUThread>(on_select);
	idm::select<SPUThread>(on_select);
	return true;
}

void Emulator::Resume()
{
	// Get pause start time
	const u64 time = m_pause_start_time.exchange(0);

	// Try to increment summary pause time
	if (time)
	{
		m_pause_amend_time += get_system_time() - time;
	}

	// Print and reset debug data collected
	if (m_state == system_state::paused && g_cfg.core.ppu_debug)
	{
		PPUDisAsm dis_asm(CPUDisAsm_InterpreterMode);
		dis_asm.offset = vm::g_base_addr;

		std::string dump;

		for (u32 i = 0x10000; i < 0x40000000;)
		{
			if (vm::check_addr(i))
			{
				if (auto& data = *(be_t<u32>*)(vm::g_stat_addr + i))
				{
					dis_asm.dump_pc = i;
					dis_asm.disasm(i);
					fmt::append(dump, "\n\t'%08X' %s", data, dis_asm.last_opcode);
					data = 0;
				}

				i += sizeof(u32);
			}
			else
			{
				i += 4096;
			}
		}

		LOG_NOTICE(PPU, "[RESUME] Dumping instruction stats:%s", dump);
	}

	// Try to resume
	if (!m_state.compare_and_swap_test(system_state::paused, system_state::running))
	{
		return;
	}

	if (!time)
	{
		LOG_ERROR(GENERAL, "Emulator::Resume() error: concurrent access");
	}

	auto on_select = [](u32, cpu_thread& cpu)
	{
		cpu.state -= cpu_flag::dbg_global_pause;
		cpu.notify();
	};

	idm::select<ppu_thread>(on_select);
	idm::select<RawSPUThread>(on_select);
	idm::select<SPUThread>(on_select);
	GetCallbacks().on_resume();
}

void Emulator::Stop(bool restart)
{
	if (m_state.exchange(system_state::stopped) == system_state::stopped)
	{
		m_force_boot = false;
		return;
	}

	const bool do_exit = !restart && !m_force_boot && g_cfg.misc.autoexit;

	LOG_NOTICE(GENERAL, "Stopping emulator...");

	GetCallbacks().on_stop();

#ifdef WITH_GDB_DEBUGGER
	//fxm for some reason doesn't call on_stop
	fxm::get<GDBDebugServer>()->on_stop();
	fxm::remove<GDBDebugServer>();
#endif

	auto e_stop = std::make_exception_ptr(cpu_flag::dbg_global_stop);

	auto on_select = [&](u32, cpu_thread& cpu)
	{
		cpu.state += cpu_flag::dbg_global_stop;

		// Can't normally be null.
		// Hack for a possible vm deadlock on thread creation.
		if (auto thread = cpu.get())
		{
			thread->set_exception(e_stop);
		}
	};

	idm::select<ppu_thread>(on_select);
	idm::select<RawSPUThread>(on_select);
	idm::select<SPUThread>(on_select);

	LOG_NOTICE(GENERAL, "All threads signaled...");

	while (g_thread_count)
	{
		m_cb.process_events();

		std::this_thread::sleep_for(10ms);
	}

	LOG_NOTICE(GENERAL, "All threads stopped...");

	lv2_obj::cleanup();
	idm::clear();
	fxm::clear();

	LOG_NOTICE(GENERAL, "Objects cleared...");

	RSXIOMem.Clear();
	vm::close();

	if (do_exit)
	{
		GetCallbacks().exit();
	}
	else
	{
		Init();
	}

#ifdef LLVM_AVAILABLE
	extern void jit_finalize();
	jit_finalize();
#endif

	if (restart)
	{
		return Load();
	}

	// Boot arg cleanup (preserved in the case restarting)
	argv.clear();
	envp.clear();
	data.clear();
	disc.clear();
	klic.clear();

	m_force_boot = false;
}

s32 error_code::error_report(const fmt_type_info* sup, u64 arg, const fmt_type_info* sup2, u64 arg2)
{
	static thread_local std::unordered_map<std::string, std::size_t> g_tls_error_stats;
	static thread_local std::string g_tls_error_str;

	if (g_tls_error_stats.empty())
	{
		thread_ctrl::atexit([]
		{
			for (auto&& pair : g_tls_error_stats)
			{
				if (pair.second > 3)
				{
					LOG_ERROR(GENERAL, "Stat: %s [x%u]", pair.first, pair.second);
				}
			}
		});
	}

	logs::channel* channel = &logs::GENERAL;
	logs::level level = logs::level::error;
	const char* func = "Unknown function";

	if (auto thread = get_current_cpu_thread())
	{
		if (thread->id_type() == 1)
		{
			auto& ppu = static_cast<ppu_thread&>(*thread);

			if (ppu.last_function)
			{
				func = ppu.last_function;
			}
		}
	}

	// Format log message (use preallocated buffer)
	g_tls_error_str.clear();
	fmt::append(g_tls_error_str, "'%s' failed with 0x%08x%s%s%s%s", func, arg, sup ? " : " : "", std::make_pair(sup, arg), sup2 ? ", " : "", std::make_pair(sup2, arg2));

	// Update stats and check log threshold
	const auto stat = ++g_tls_error_stats[g_tls_error_str];

	if (stat <= 3)
	{
		channel->format(level, "%s [%u]", g_tls_error_str, stat);
	}

	return static_cast<s32>(arg);
}

Emulator Emu;
