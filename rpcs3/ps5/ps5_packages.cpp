// PS5: the launcher's Install tab's packages (ps5_packages.h)

#include "stdafx.h"
#include "ps5_packages.h"

#include "Crypto/unpkg.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Loader/PSF.h"
#include "Utilities/File.h"
#include "Utilities/Thread.h"

#include <algorithm>
#include <cctype>
#include <deque>

LOG_CHANNEL(launcher_log, "Launcher");

namespace ps5_packages
{
	namespace
	{
		std::string lower(std::string_view text)
		{
			std::string result(text);
			std::transform(result.begin(), result.end(), result.begin(), [](char c) { return static_cast<char>(std::tolower(static_cast<u8>(c))); });
			return result;
		}

		std::string extension(std::string_view name)
		{
			const usz dot = name.rfind('.');
			return dot == umax ? std::string() : lower(name.substr(dot + 1));
		}

		// The version of the game installed in dev_hdd0/game/<title id>, if any
		std::string installed_version(const std::string& title_id)
		{
			if (title_id.empty())
			{
				return {};
			}
			const std::string path = rpcs3::utils::get_hdd0_dir() + "game/" + title_id + "/PARAM.SFO";
			const fs::file sfo(path);
			if (!sfo)
			{
				return {};
			}
			const psf::registry psf = psf::load_object(sfo, path);
			std::string version(psf::get_string(psf, "APP_VER", ""));
			if (version.empty())
			{
				version = std::string(psf::get_string(psf, "VERSION", ""));
			}
			return version;
		}

		// Where a licence goes: the user's exdata folder
		std::string licence_folder()
		{
			const std::string user = Emu.GetUsr().empty() ? std::string("00000001") : Emu.GetUsr();
			return rpcs3::utils::get_hdd0_dir() + "home/" + user + "/exdata/";
		}
	}

	std::string folder()
	{
		return fs::get_config_dir() + "packages/";
	}

	std::string ftp_folder()
	{
		// /app0 is the title's folder: /data/homebrew/<title id>/
		std::string path = folder();
		if (path.starts_with("/app0/"))
		{
			path = "/data/homebrew/PPSA99200/" + path.substr(6);
		}
		return path;
	}

	std::vector<file> scan()
	{
		std::vector<file> files;
		const std::string dir = folder();
		fs::create_path(dir);

		fs::dir listing{dir};
		if (!listing)
		{
			return files;
		}

		for (const fs::dir_entry& entry : listing)
		{
			if (entry.is_directory)
			{
				continue;
			}

			const std::string ext = extension(entry.name);
			if (ext != "pkg" && ext != "rap" && ext != "edat")
			{
				continue;
			}

			file f;
			f.name = entry.name;
			f.path = dir + entry.name;
			f.size = entry.size;

			if (ext != "pkg")
			{
				// A licence is named by its content ID: XX0000-ABCD12345_00-...
				f.type = kind::licence;
				f.title = entry.name.substr(0, entry.name.size() - ext.size() - 1);
				if (f.title.size() >= 16 && f.title[6] == '-')
				{
					f.title_id = f.title.substr(7, 9);
				}
				files.push_back(std::move(f));
				continue;
			}

			const package_reader reader(f.path);
			if (!reader.is_valid())
			{
				f.type = kind::unreadable;
				f.title = f.name;
				files.push_back(std::move(f));
				continue;
			}

			const psf::registry& psf = reader.get_psf();
			const std::string category(psf::get_string(psf, "CATEGORY", ""));
			f.title = std::string(psf::get_string(psf, "TITLE", ""));
			f.title_id = std::string(psf::get_string(psf, "TITLE_ID", ""));
			f.version = std::string(psf::get_string(psf, "APP_VER", ""));
			if (f.version.empty())
			{
				f.version = std::string(psf::get_string(psf, "VERSION", ""));
			}
			f.target_version = std::string(psf::get_string(psf, "TARGET_APP_VER", ""));
			f.installed_version = installed_version(f.title_id);
			if (f.title.empty())
			{
				f.title = f.name;
			}

			if (reader.get_metadata().content_type == PKG_CONTENT_TYPE_LICENSE)
			{
				f.type = kind::licence;
			}
			else if (category == "GD")
			{
				// As the desktop tells them apart (game_compatibility::GetPkgInfo)
				f.type = (reader.get_metadata().package_type & pkg_flag::PKG_FLAG_PATCH) ? kind::update : kind::addon;
			}
			else if (category == "HG" || category == "DG" || category == "AP" || category == "AV" || category == "AM" || category == "1P" || category == "2P")
			{
				f.type = kind::game;
			}
			else
			{
				f.type = kind::other;
			}
			files.push_back(std::move(f));
		}

		// Licences first, then games, then the updates and add-ons, each game's
		// oldest first: an update installs on the version before it
		const auto rank = [](kind k)
		{
			switch (k)
			{
			case kind::licence: return 0;
			case kind::game: return 1;
			case kind::other: return 2;
			case kind::update: return 3;
			case kind::addon: return 4;
			case kind::unreadable: return 5;
			}
			return 5;
		};
		std::stable_sort(files.begin(), files.end(), [&](const file& a, const file& b)
		{
			if (rank(a.type) != rank(b.type)) return rank(a.type) < rank(b.type);
			if (a.title_id != b.title_id) return a.title_id < b.title_id;
			if (a.version != b.version) return a.version < b.version;
			return a.name < b.name;
		});

		for (file& f : files)
		{
			f.chosen = f.type != kind::unreadable;
		}
		return files;
	}

	std::string kind_text(const file& f)
	{
		switch (f.type)
		{
		case kind::game: return "Game";
		case kind::update: return f.version.empty() ? "Update" : "Update " + f.version;
		case kind::addon: return "Add-on";
		case kind::other: return "Package";
		case kind::licence: return "Licence";
		case kind::unreadable: return "Not a package RPCS3 can read";
		}
		return {};
	}

	std::string size_text(u64 bytes)
	{
		if (bytes >= 1024ull * 1024 * 1024)
		{
			return fmt::format("%.1f GB", bytes / (1024. * 1024 * 1024));
		}
		if (bytes >= 1024ull * 1024)
		{
			return fmt::format("%u MB", (bytes + 512 * 1024) / (1024 * 1024));
		}
		return fmt::format("%u KB", (bytes + 1023) / 1024);
	}

	struct installer::impl
	{
		std::vector<file> files;
		bool delete_when_installed = false;
		std::vector<std::unique_ptr<std::deque<package_reader>>> readers; // a .pkg's, made before the thread starts
		std::unique_ptr<atomic_t<u8>[]> states;
		std::vector<std::string> messages;
		u64 total_size = 0;
		atomic_t<bool> cancel = false;
		atomic_t<bool> done = false;
		std::unique_ptr<named_thread<std::function<void()>>> thread;

		void run();
		state install_licence(const file& f, std::string& message) const;
		state install_package(usz index, std::string& message);
	};

	installer::state installer::impl::install_licence(const file& f, std::string& message) const
	{
		if (f.size < 0x10)
		{
			message = "Not installed: too small to be a licence.";
			return state::failed;
		}

		const std::string dir = licence_folder();
		const usz dot = f.name.rfind('.');
		const std::string target = dir + f.name.substr(0, dot) + "." + extension(f.name);
		if (!fs::create_path(dir) || !fs::copy_file(f.path, target, true))
		{
			message = fmt::format("Not installed: it could not be copied (%s).", fs::g_tls_error);
			return state::failed;
		}

		launcher_log.notice("Install: licence %s copied to %s", f.name, target);
		message = "Installed with the user's licences.";
		return state::installed;
	}

	installer::state installer::impl::install_package(usz index, std::string& message)
	{
		const file& f = files[index];
		std::deque<package_reader>& deck = *readers[index];
		if (!deck.front().is_valid())
		{
			message = "Not installed: RPCS3 could not read this package.";
			return state::failed;
		}

		std::deque<std::string> bootable;
		const package_install_result result = package_reader::extract_data(deck, bootable, false);

		switch (deck.front().get_result())
		{
		case package_reader::result::success:
		{
			const std::string now = installed_version(f.title_id);
			message = f.type == kind::game ? "Installed: it is in the Library."
				: f.type == kind::update && !now.empty() ? fmt::format("Installed: the game is version %s now.", now)
				: "Installed.";
			launcher_log.notice("Install: %s installed (%s %s, %s)", f.name, f.title_id, f.version, kind_text(f));
			return state::installed;
		}
		case package_reader::result::not_started:
		case package_reader::result::started:
		case package_reader::result::aborted:
		case package_reader::result::aborted_dirty:
			if (result.error == package_install_result::error_type::no_error || cancel)
			{
				message = deck.front().get_result() == package_reader::result::aborted_dirty ? "Stopped part way: some of its files were left in place." : "Stopped before it was installed.";
				launcher_log.notice("Install: %s stopped", f.name);
				return state::cancelled;
			}
			break;
		case package_reader::result::error:
		case package_reader::result::error_dirty:
			break;
		}

		if (result.error == package_install_result::error_type::app_version)
		{
			const auto& v = result.version;
			if (!v.expected.empty() && !v.installed.empty())
			{
				message = fmt::format("Not installed: this update goes on version %s of the game, and version %s is installed. Install the update for version %s first.", v.expected, v.installed, v.expected);
			}
			else if (!v.expected.empty())
			{
				message = fmt::format("Not installed: this update goes on version %s of the game, and no update is installed yet. Install the updates before it first, oldest first.", v.expected);
			}
			else if (!v.installed.empty())
			{
				message = fmt::format("Not installed: version %s is installed already, newer than this update (%s).", v.installed, v.app_ver);
			}
			else
			{
				message = "Not installed: it does not fit the version of the game that is installed.";
			}
		}
		else
		{
			message = deck.front().get_result() == package_reader::result::error_dirty
				? "Failed part way: some of its files were left in place. The trace says why."
				: "Failed: nothing was installed. The trace says why.";
		}
		launcher_log.error("Install: %s failed: %s", f.name, message);
		return state::failed;
	}

	void installer::impl::run()
	{
		for (usz i = 0; i < files.size(); i++)
		{
			const file& f = files[i];
			std::string message;
			state result;

			if (cancel)
			{
				message = "Not installed: the installation was stopped.";
				result = state::cancelled;
			}
			else
			{
				states[i] = static_cast<u8>(state::installing);
				launcher_log.notice("Install: %s (%s %s, %s, %s)", f.name, f.title_id, f.version, kind_text(f), size_text(f.size));
				result = readers[i] ? install_package(i, message) : install_licence(f, message);
			}

			if (result == state::installed && delete_when_installed)
			{
				// Unlinked while the reader still holds it open: the space comes
				// back once the installation ends
				if (fs::remove_file(f.path))
				{
					message += " Its file was deleted.";
				}
			}

			messages[i] = std::move(message);
			states[i] = static_cast<u8>(result);
		}
		done = true;
	}

	installer::installer(std::vector<file> files, bool delete_when_installed)
		: m(std::make_unique<impl>())
	{
		m->files = std::move(files);
		m->delete_when_installed = delete_when_installed;
		m->states = std::make_unique<atomic_t<u8>[]>(m->files.size());
		m->messages.resize(m->files.size());
		for (const file& f : m->files)
		{
			m->total_size += f.size;
			if (extension(f.name) == "pkg")
			{
				auto deck = std::make_unique<std::deque<package_reader>>();
				deck->emplace_back(f.path);
				m->readers.push_back(std::move(deck));
			}
			else
			{
				m->readers.push_back(nullptr);
			}
		}

		m->thread = std::make_unique<named_thread<std::function<void()>>>("Launcher Install", [impl = m.get()]()
		{
			impl->run();
		});
	}

	installer::~installer()
	{
		cancel();
		m->thread.reset();
	}

	void installer::cancel()
	{
		m->cancel = true;
		for (usz i = 0; i < m->files.size(); i++)
		{
			if (m->readers[i] && m->states[i] == static_cast<u8>(state::installing))
			{
				m->readers[i]->front().abort_extract();
			}
		}
	}

	bool installer::done() const
	{
		return m->done;
	}

	bool installer::cancelled() const
	{
		return m->cancel;
	}

	const std::vector<file>& installer::files() const
	{
		return m->files;
	}

	installer::state installer::state_of(usz index) const
	{
		return index < m->files.size() ? static_cast<state>(m->states[index].load()) : state::waiting;
	}

	f32 installer::progress_of(usz index) const
	{
		switch (state_of(index))
		{
		case state::installed: return 1.f;
		case state::installing: return m->readers[index] ? m->readers[index]->front().get_progress(1000) / 1000.f : 0.f;
		default: return 0.f;
		}
	}

	std::string installer::message_of(usz index) const
	{
		const state s = state_of(index);
		return s == state::waiting || s == state::installing ? std::string() : m->messages[index];
	}

	f32 installer::progress() const
	{
		if (!m->total_size)
		{
			return m->done ? 1.f : 0.f;
		}
		f64 written = 0.;
		for (usz i = 0; i < m->files.size(); i++)
		{
			const state s = state_of(i);
			if (s == state::installed || s == state::failed || s == state::cancelled)
			{
				written += static_cast<f64>(m->files[i].size);
			}
			else if (s == state::installing)
			{
				written += static_cast<f64>(m->files[i].size) * progress_of(i);
			}
		}
		return static_cast<f32>(std::min(1., written / m->total_size));
	}
}
