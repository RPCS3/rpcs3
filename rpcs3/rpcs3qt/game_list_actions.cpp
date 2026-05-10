#include "stdafx.h"
#include "game_list_actions.h"
#include "game_list_frame.h"
#include "gui_settings.h"
#include "category.h"
#include "qt_utils.h"
#include "progress_dialog.h"

#include "Utilities/Thread.h"
#include "Utilities/File.h"
#include "Loader/ISO.h"

#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Emu/VFS.h"

#include "Input/pad_thread.h"

#include <thread>

#include <QApplication>
#include <QCheckBox>
#include <QtConcurrent>
#include <QDir>
#include <QDirIterator>
#include <QGridLayout>
#include <QMessageBox>
#include <QTimer>

LOG_CHANNEL(game_list_log, "GameList");
LOG_CHANNEL(sys_log, "SYS");

extern atomic_t<bool> g_system_progress_canceled;

game_list_actions::game_list_actions(game_list_frame* frame, std::shared_ptr<gui_settings> gui_settings)
	: m_game_list_frame(frame), m_gui_settings(std::move(gui_settings))
{
	ensure(!!m_game_list_frame);
	ensure(!!m_gui_settings);
}

game_list_actions::~game_list_actions()
{
}

void game_list_actions::SetContentList(u16 content_types, const content_info& content_info)
{
	m_content_info = content_info;

	m_content_info.content_types = content_types;
	m_content_info.clear_on_finish = true; // Always overridden by BatchRemoveContentLists()
}

void game_list_actions::ClearContentList(bool refresh)
{
	if (refresh)
	{
		std::vector<std::string> serials_to_remove_from_yml;

		// Prepare the list of serials (title id) to remove in "games.yml" file (if any)
		for (const auto& removedDisc : m_content_info.removed_disc_list)
		{
			serials_to_remove_from_yml.push_back(removedDisc);
		}

		// Finally, refresh the game list
		m_game_list_frame->Refresh(true, serials_to_remove_from_yml);
	}

	m_content_info = {NO_CONTENT};
}

game_list_actions::content_info game_list_actions::GetContentInfo(const std::vector<game_info>& games)
{
	content_info content_info = {NO_CONTENT};

	if (games.empty())
		return content_info;

	bool is_disc_game = false;
	u64 total_disc_size = 0;
	u64 total_data_size = 0;
	QString text;

	// Fill in content_info

	content_info.is_single_selection = games.size() == 1;

	for (const auto& game : games)
	{
		GameInfo& current_game = game->info;

		is_disc_game = QString::fromStdString(current_game.category) == cat::cat_disc_game;

		// +1 if it's a disc game's path and it's present in the shared games folder
		content_info.in_games_dir_count += (is_disc_game && Emu.IsPathInsideDir(current_game.path, rpcs3::utils::get_games_dir())) ? 1 : 0;

		// Add the name to the content's name list for the related serial
		content_info.name_list[current_game.serial].insert(current_game.name);

		if (is_disc_game)
		{
			if (current_game.size_on_disk != umax) // If size was properly detected
				total_disc_size += current_game.size_on_disk;

			// Add the serial to the disc list
			content_info.disc_list.insert(current_game.serial);

			// It could be an empty list for a disc game
			std::set<std::string> data_dir_list = rpcs3::utils::get_dir_list(rpcs3::utils::get_hdd0_game_dir(), current_game.serial);

			// Add the path list to the content's path list for the related serial
			for (const auto& data_dir : data_dir_list)
			{
				content_info.path_list[current_game.serial].insert(data_dir);
			}
		}
		else
		{
			// Add the path to the content's path list for the related serial
			content_info.path_list[current_game.serial].insert(current_game.path);
		}
	}

	// Fill in text based on filled in content_info

	if (content_info.is_single_selection) // Single selection
	{
		GameInfo& current_game = games[0]->info;

		text = tr("%0 - %1\n").arg(QString::fromStdString(current_game.serial)).arg(QString::fromStdString(current_game.name));

		if (is_disc_game)
		{
			text += tr("\nDisc Game Info:\nPath: %0\n").arg(QString::fromStdString(current_game.path));

			if (total_disc_size)
				text += tr("Size: %0\n").arg(gui::utils::format_byte_size(total_disc_size));
		}

		// if a path is present (it could be an empty list for a disc game)
		if (const auto& it = content_info.path_list.find(current_game.serial); it != content_info.path_list.end())
		{
			text += tr("\n%0 Info:\n").arg(is_disc_game ? tr("Game Data") : games[0]->localized_category);

			for (const auto& data_dir : it->second)
			{
				text += tr("Path: %0\n").arg(QString::fromStdString(data_dir));

				if (const u64 data_size = fs::get_dir_size(data_dir, 1); data_size != umax)
				{ // If size was properly detected
					total_data_size += data_size;
					text += tr("Size: %0\n").arg(gui::utils::format_byte_size(data_size));
				}
			}

			if (it->second.size() > 1)
				text += tr("Total size: %0\n").arg(gui::utils::format_byte_size(total_data_size));
		}
	}
	else // Multi selection
	{
		for (const auto& [serial, data_dir_list] : content_info.path_list)
		{
			for (const auto& data_dir : data_dir_list)
			{
				if (const u64 data_size = fs::get_dir_size(data_dir, 1); data_size != umax) // If size was properly detected
					total_data_size += data_size;
			}
		}

		text = tr("%0 selected games: %1 Disc Game - %2 not Disc Game\n").arg(games.size())
				.arg(content_info.disc_list.size()).arg(games.size() - content_info.disc_list.size());

		text += tr("\nDisc Game Info:\n");

		if (content_info.disc_list.size() != content_info.in_games_dir_count)
			text += tr("VFS unhosted: %0\n").arg(content_info.disc_list.size() - content_info.in_games_dir_count);

		if (content_info.in_games_dir_count)
			text += tr("VFS hosted: %0\n").arg(content_info.in_games_dir_count);

		if (content_info.disc_list.size() != content_info.in_games_dir_count && content_info.in_games_dir_count)
			text += tr("Total games: %0\n").arg((content_info.disc_list.size() - content_info.in_games_dir_count) + content_info.in_games_dir_count);

		if (total_disc_size)
			text += tr("Total size: %0\n").arg(gui::utils::format_byte_size(total_disc_size));

		if (content_info.path_list.size())
			text += tr("\nGame Data Info:\nTotal size: %0\n").arg(gui::utils::format_byte_size(total_data_size));
	}

	u64 caches_size = 0;
	u64 icons_size = 0;
	u64 savestates_size = 0;
	u64 captures_size = 0;
	u64 recordings_size = 0;
	u64 screenshots_size = 0;

	for (const auto& [serial, name_list] : content_info.name_list)
	{
		// Main cache
		if (const u64 size = fs::get_dir_size(rpcs3::utils::get_cache_dir_by_serial(serial), 1); size != umax)
			caches_size += size;

		// HDD1 cache
		for (const auto& dir : rpcs3::utils::get_dir_list(rpcs3::utils::get_hdd1_cache_dir(), serial))
		{
			if (const u64 size = fs::get_dir_size(dir, 1); size != umax)
				caches_size += size;
		}

		if (const u64 size = fs::get_dir_size(rpcs3::utils::get_icons_dir(serial), 1); size != umax)
			icons_size += size;

		if (const u64 size = fs::get_dir_size(rpcs3::utils::get_savestates_dir(serial), 1); size != umax)
			savestates_size += size;

		for (const auto& file : rpcs3::utils::get_file_list(rpcs3::utils::get_captures_dir(), serial))
		{
			if (fs::stat_t stat{}; fs::get_stat(file, stat))
				captures_size += stat.size;
		}

		if (const u64 size = fs::get_dir_size(rpcs3::utils::get_recordings_dir(serial), 1); size != umax)
			recordings_size += size;

		if (const u64 size = fs::get_dir_size(rpcs3::utils::get_screenshots_dir(serial), 1); size != umax)
			screenshots_size += size;
	}

	text += tr("\nEmulator Data Info:\nCaches size: %0\n").arg(gui::utils::format_byte_size(caches_size));
	text += tr("Icons size: %0\n").arg(gui::utils::format_byte_size(icons_size));
	text += tr("Savestates size: %0\n").arg(gui::utils::format_byte_size(savestates_size));
	text += tr("Captures size: %0\n").arg(gui::utils::format_byte_size(captures_size));
	text += tr("Recordings size: %0\n").arg(gui::utils::format_byte_size(recordings_size));
	text += tr("Screenshots size: %0\n").arg(gui::utils::format_byte_size(screenshots_size));

	// Retrieve disk space info on data path's drive
	if (fs::device_stat stat{}; fs::statfs(rpcs3::utils::get_hdd0_dir(), stat))
		text += tr("\nCurrent free disk space: %0\n").arg(gui::utils::format_byte_size(stat.avail_free));

	content_info.info = text;

	return content_info;
}

void game_list_actions::ShowRemoveGameDialog(const std::vector<game_info>& games)
{
	if (games.empty())
		return;

	content_info content_info = GetContentInfo(games);
	QString text = content_info.info;

	QCheckBox* disc = new QCheckBox(tr("Remove title from game list (Disc Game path is not removed!)"));
	QCheckBox* caches = new QCheckBox(tr("Remove caches and custom configs"));
	QCheckBox* icons = new QCheckBox(tr("Remove icons and shortcuts"));
	QCheckBox* savestate = new QCheckBox(tr("Remove savestates"));
	QCheckBox* captures = new QCheckBox(tr("Remove captures"));
	QCheckBox* recordings = new QCheckBox(tr("Remove recordings"));
	QCheckBox* screenshots = new QCheckBox(tr("Remove screenshots"));

	if (content_info.disc_list.size())
	{
		if (content_info.in_games_dir_count == content_info.disc_list.size())
		{
			disc->setToolTip(tr("Title located under auto-detection VFS \"games\" folder cannot be removed"));
			disc->setDisabled(true);
		}
		else
		{
			if (!content_info.is_single_selection) // Multi selection
				disc->setToolTip(tr("Title located under auto-detection VFS \"games\" folder cannot be removed"));

			disc->setChecked(true);
		}
	}
	else
	{
		disc->setChecked(false);
		disc->setVisible(false);
	}

	if (content_info.path_list.size()) // If a path is present
	{
		text += tr("\nPermanently remove %0 and selected (optional) contents from drive?\n")
	            .arg((content_info.disc_list.size() || !content_info.is_single_selection) ? tr("Game Data") : games[0]->localized_category);
	}
	else
	{
		text += tr("\nPermanently remove selected (optional) contents from drive?\n");
	}

	caches->setChecked(true);
	icons->setChecked(true);

	QMessageBox mb(QMessageBox::Question, tr("Confirm Removal"), text, QMessageBox::Yes | QMessageBox::No, m_game_list_frame);
	mb.setCheckBox(disc);

	QGridLayout* grid = qobject_cast<QGridLayout*>(mb.layout());
	int row, column, rowSpan, columnSpan;

	grid->getItemPosition(grid->indexOf(disc), &row, &column, &rowSpan, &columnSpan);
	grid->addWidget(caches, row + 3, column, rowSpan, columnSpan);
	grid->addWidget(icons, row + 4, column, rowSpan, columnSpan);
	grid->addWidget(savestate, row + 5, column, rowSpan, columnSpan);
	grid->addWidget(captures, row + 6, column, rowSpan, columnSpan);
	grid->addWidget(recordings, row + 7, column, rowSpan, columnSpan);
	grid->addWidget(screenshots, row + 8, column, rowSpan, columnSpan);

	if (mb.exec() != QMessageBox::Yes)
		return;

	// Remove data path in "dev_hdd0/game" folder (if any) and lock file in "dev_hdd0/game/＄locks" folder (if any)
	u16 content_types = DATA | LOCKS;

	// Remove serials (title id) in "games.yml" file (if any)
	if (disc->isChecked())
		content_types |= DISC;

	// Remove caches in "cache" and "dev_hdd1/caches" folders (if any) and custom configs in "config/custom_config" folder (if any)
	if (caches->isChecked())
		content_types |= CACHES | CUSTOM_CONFIG;

	// Remove icons in "Icons/game_icons" folder (if any) and
	// shortcuts in "games/shortcuts" folder and from desktop / start menu (if any)
	if (icons->isChecked())
		content_types |= ICONS | SHORTCUTS;

	if (savestate->isChecked())
		content_types |= SAVESTATES;

	if (captures->isChecked())
		content_types |= CAPTURES;

	if (recordings->isChecked())
		content_types |= RECORDINGS;

	if (screenshots->isChecked())
		content_types |= SCREENSHOTS;

	SetContentList(content_types, content_info);

	if (content_info.is_single_selection) // Single selection
	{
		if (!RemoveContentList(games[0]->info.serial))
		{
			QMessageBox::critical(m_game_list_frame, tr("Failure!"), caches->isChecked()
				? tr("Failed to remove %0 from drive!\nCaches and custom configs have been left intact.").arg(QString::fromStdString(games[0]->info.name))
				: tr("Failed to remove %0 from drive!").arg(QString::fromStdString(games[0]->info.name)));

			return;
		}
	}
	else // Multi selection
	{
		BatchRemoveContentLists(games);
	}
}

void game_list_actions::ShowGameInfoDialog(const std::vector<game_info>& games)
{
	if (games.empty())
		return;

	QMessageBox::information(m_game_list_frame, tr("Game Info"), GetContentInfo(games).info);
}

void game_list_actions::ShowGameIntegrityDialog(const game_info& game)
{
	if (m_game_integrity_future.isRunning()) // Still running the last request
		return;

	// Initialize the validator (set also file size etc.)
	m_iso_validator->init_hash(game->info.path);

	// Game integrity check can take a while (in particular on non ssd/m.2 disks)
	// so run it on a concurrent thread avoiding to block the entire GUI
	m_game_integrity_future = QtConcurrent::run([this]()
	{
		thread_base::set_name("Game Integrity");

		QString text;
		std::string hash, game_name;
		bool info_dialog = false;

		if (m_iso_validator->calculate_hash(hash) != iso_hash_status::COMPLETED)
		{
			text = "Hash calculation failed!\n\nIntegrity check aborted";
		}
		else
		{
			text = "Integrity check completed!\n\n";

			switch (m_iso_validator->check_integrity(hash, &game_name))
			{
			case iso_integrity_status::NO_MATCH:
				text += tr("Game check NOT PASSED\n\nNo match found on DB or game corrupted:\n - Hash: %0")
					.arg(QString::fromStdString(hash));
				break;
			case iso_integrity_status::FOUND_MATCH:
				text += tr("Game check PASSED\n\nMatch found on DB:\n - Game: %0\n - Hash: %1")
					.arg(QString::fromStdString(game_name))
					.arg(QString::fromStdString(hash));

				info_dialog = true;
				break;
			default:
				text += tr("Error parsing DB");
				break;
			}
		}

		Emu.CallFromMainThread([this, text, info_dialog]()
		{
			if (info_dialog)
			{
				sys_log.success("%s", text.toStdString());
				QMessageBox::information(m_game_list_frame, tr("Game Integrity"), text);
			}
			else
			{
				sys_log.error("%s", text.toStdString());
				QMessageBox::critical(m_game_list_frame, tr("Game Integrity"), text);
			}
		}, nullptr, false);
	});

	progress_dialog* pdlg = new progress_dialog(tr("ISO File Hash Calculation"), tr("Calculating hash"), tr("Cancel"),
		0, 100, false, m_game_list_frame);

	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->open();

	connect(pdlg, &progress_dialog::canceled, m_game_list_frame, [this]()
	{
		m_iso_validator->abort_hash();
	});

	QTimer* update_timer = new QTimer(m_game_list_frame);

	connect(update_timer, &QTimer::timeout, m_game_list_frame, [this, pdlg, update_timer]()
	{
		if (m_iso_validator->get_status() == iso_hash_status::INITIALIZED)
		{
			// Set progress in range 0-100
			const int progress = m_iso_validator->get_size() ?
				(static_cast<float>(m_iso_validator->get_bytes_read()) / m_iso_validator->get_size()) * 100 :
				0;

			pdlg->setValue(progress);
		}
		else
		{
			update_timer->stop();
			update_timer->deleteLater();

			// As last, close the progress bar (it will be already closed if the process was aborted) and delete the object
			pdlg->close();
			pdlg->deleteLater();
		}
	});

	update_timer->start(500);
}

void game_list_actions::ShowDiskUsageDialog()
{
	if (m_disk_usage_future.isRunning()) // Still running the last request
		return;

	// Disk usage calculation can take a while (in particular on non ssd/m.2 disks)
	// so run it on a concurrent thread avoiding to block the entire GUI
	m_disk_usage_future = QtConcurrent::run([this]()
	{
		thread_base::set_name("Disk Usage");

		const std::vector<std::pair<std::string, u64>> vfs_disk_usage = rpcs3::utils::get_vfs_disk_usage();
		const u64 cache_disk_usage = rpcs3::utils::get_cache_disk_usage();

		QString text;
		u64 tot_data_size = 0;

		for (const auto& [dev, data_size] : vfs_disk_usage)
		{
			text += tr("\n    %0: %1").arg(QString::fromStdString(dev)).arg(gui::utils::format_byte_size(data_size));
			tot_data_size += data_size;
		}

		if (!text.isEmpty())
			text = tr("\n  VFS disk usage: %0%1").arg(gui::utils::format_byte_size(tot_data_size)).arg(text);

		text += tr("\n  Cache disk usage: %0").arg(gui::utils::format_byte_size(cache_disk_usage));

		sys_log.success("%s", text);

		Emu.CallFromMainThread([this, text]()
		{
			QMessageBox::information(m_game_list_frame, tr("Disk usage"), text);
		}, nullptr, false);
	});
}

bool game_list_actions::IsGameRunning(const std::string& serial)
{
	return !Emu.IsStopped(true) && (serial == Emu.GetTitleID() || (serial == "vsh.self" && Emu.IsVsh()));
}

bool game_list_actions::ValidateRemoval(const std::string& serial, const std::string& path, const std::string& desc, bool is_interactive)
{
	if (serial.empty())
	{
		game_list_log.error("Removal of %s not allowed due to no title ID provided!", desc);
		return false;
	}

	if (path.empty() || !fs::exists(path) || (!fs::is_dir(path) && !fs::is_file(path)))
	{
		game_list_log.success("Could not find %s directory/file: %s (%s)", desc, path, serial);
		return false;
	}

	if (is_interactive)
	{
		if (IsGameRunning(serial))
		{
			game_list_log.error("Removal of %s not allowed due to %s title is running!", desc, serial);

			QMessageBox::critical(m_game_list_frame, tr("Removal Aborted"),
				tr("Removal of %0 not allowed due to %1 title is running!")
				.arg(QString::fromStdString(desc)).arg(QString::fromStdString(serial)));

			return false;
		}

		if (QMessageBox::question(m_game_list_frame, tr("Confirm Removal"), tr("Remove %0?").arg(QString::fromStdString(desc))) != QMessageBox::Yes)
			return false;
	}

	return true;
}

bool game_list_actions::ValidateBatchRemoval(const std::string& desc, bool is_interactive)
{
	if (!Emu.IsStopped(true))
	{
		game_list_log.error("Removal of %s not allowed due to emulator is running!", desc);

		if (is_interactive)
		{
			QMessageBox::critical(m_game_list_frame, tr("Removal Aborted"),
				tr("Removal of %0 not allowed due to emulator is running!").arg(QString::fromStdString(desc)));
		}

		return false;
	}

	if (is_interactive)
	{
		if (QMessageBox::question(m_game_list_frame, tr("Confirm Removal"), tr("Remove %0?").arg(QString::fromStdString(desc))) != QMessageBox::Yes)
			return false;
	}

	return true;
}

bool game_list_actions::CreateCPUCaches(const std::string& path, const std::string& serial, bool is_fast_compilation)
{
	Emu.GracefulShutdown(false);
	Emu.SetForceBoot(true);
	Emu.SetPrecompileCacheOption(emu_precompilation_option_t{.is_fast = is_fast_compilation});

	if (const auto error = Emu.BootGame(fs::is_file(path) ? fs::get_parent_dir(path) : path, serial, true); error != game_boot_result::no_errors)
	{
		game_list_log.error("Could not create LLVM caches for %s, error: %s", path, error);
		return false;
	}

	game_list_log.warning("Creating LLVM Caches for %s", path);
	return true;
}

bool game_list_actions::CreateCPUCaches(const game_info& game, bool is_fast_compilation)
{
	return game && CreateCPUCaches(game->info.path, game->info.serial, is_fast_compilation);
}

bool game_list_actions::RemoveCustomConfiguration(const std::string& serial, const game_info& game, bool is_interactive)
{
	const std::string path = rpcs3::utils::get_custom_config_path(serial);

	if (!ValidateRemoval(serial, path, "custom configuration", is_interactive))
		return true;

	bool result = true;

	if (fs::is_file(path))
	{
		if (fs::remove_file(path))
		{
			if (game)
			{
				game->has_custom_config = false;
			}
			game_list_log.success("Removed configuration file: %s", path);
		}
		else
		{
			game_list_log.fatal("Failed to remove configuration file: %s\nError: %s", path, fs::g_tls_error);
			result = false;
		}
	}

	if (is_interactive && !result)
	{
		QMessageBox::warning(m_game_list_frame, tr("Warning!"), tr("Failed to remove configuration file!"));
	}

	return result;
}

bool game_list_actions::RemoveCustomPadConfiguration(const std::string& serial, const game_info& game, bool is_interactive)
{
	const std::string config_dir = rpcs3::utils::get_input_config_dir(serial);

	if (!ValidateRemoval(serial, config_dir, "custom gamepad configuration", false)) // no interation needed here
		return true;

	if (is_interactive && QMessageBox::question(m_game_list_frame, tr("Confirm Removal"), (!Emu.IsStopped(true) && Emu.GetTitleID() == serial)
		? tr("Remove custom gamepad configuration?\nYour configuration will revert to the global pad settings.")
		: tr("Remove custom gamepad configuration?")) != QMessageBox::Yes)
		return true;

	g_cfg_input_configs.load();
	g_cfg_input_configs.active_configs.erase(serial);
	g_cfg_input_configs.save();
	game_list_log.notice("Removed active input configuration entry for key '%s'", serial);

	if (QDir(QString::fromStdString(config_dir)).removeRecursively())
	{
		if (game)
		{
			game->has_custom_pad_config = false;
		}
		if (!Emu.IsStopped(true) && Emu.GetTitleID() == serial)
		{
			pad::set_enabled(false);
			pad::reset(serial);
			pad::set_enabled(true);
		}
		game_list_log.notice("Removed gamepad configuration directory: %s", config_dir);
		return true;
	}

	if (is_interactive)
	{
		QMessageBox::warning(m_game_list_frame, tr("Warning!"), tr("Failed to completely remove gamepad configuration directory!"));
		game_list_log.fatal("Failed to completely remove gamepad configuration directory: %s\nError: %s", config_dir, fs::g_tls_error);
	}
	return false;
}

bool game_list_actions::RemoveShaderCache(const std::string& serial, bool is_interactive)
{
	const std::string base_dir = rpcs3::utils::get_cache_dir_by_serial(serial);

	if (!ValidateRemoval(serial, base_dir, "shader cache", is_interactive))
		return true;

	u32 caches_removed = 0;
	u32 caches_total   = 0;

	const QStringList filter{ QStringLiteral("shaders_cache") };
	const QString q_base_dir = QString::fromStdString(base_dir);

	QDirIterator dir_iter(q_base_dir, filter, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (QDir(filepath).removeRecursively())
		{
			++caches_removed;
			game_list_log.notice("Removed shader cache directory: %s", filepath);
		}
		else
		{
			game_list_log.warning("Could not completely remove shader cache directory: %s", filepath);
		}

		++caches_total;
	}

	const bool success = caches_total == caches_removed;

	if (success)
		game_list_log.success("Removed shader cache in %s", base_dir);
	else
		game_list_log.fatal("Only %d/%d shader cache directories could be removed in %s", caches_removed, caches_total, base_dir);

	if (QDir(q_base_dir).isEmpty())
	{
		if (fs::remove_dir(base_dir))
			game_list_log.notice("Removed empty shader cache directory: %s", base_dir);
		else
			game_list_log.error("Could not remove empty shader cache directory: '%s' (%s)", base_dir, fs::g_tls_error);
	}

	return success;
}

bool game_list_actions::RemovePPUCache(const std::string& serial, bool is_interactive)
{
	const std::string base_dir = rpcs3::utils::get_cache_dir_by_serial(serial);

	if (!ValidateRemoval(serial, base_dir, "PPU cache", is_interactive))
		return true;

	u32 files_removed = 0;
	u32 files_total = 0;

	const QStringList filter{ QStringLiteral("v*.obj"), QStringLiteral("v*.obj.gz") };
	const QString q_base_dir = QString::fromStdString(base_dir);

	QDirIterator dir_iter(q_base_dir, filter, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (QFile::remove(filepath))
		{
			++files_removed;
			game_list_log.notice("Removed PPU cache file: %s", filepath);
		}
		else
		{
			game_list_log.warning("Could not remove PPU cache file: %s", filepath);
		}

		++files_total;
	}

	const bool success = files_total == files_removed;

	if (success)
		game_list_log.success("Removed PPU cache in %s", base_dir);
	else
		game_list_log.fatal("Only %d/%d PPU cache files could be removed in %s", files_removed, files_total, base_dir);

	if (QDir(q_base_dir).isEmpty())
	{
		if (fs::remove_dir(base_dir))
			game_list_log.notice("Removed empty PPU cache directory: %s", base_dir);
		else
			game_list_log.error("Could not remove empty PPU cache directory: '%s' (%s)", base_dir, fs::g_tls_error);
	}

	return success;
}

bool game_list_actions::RemoveSPUCache(const std::string& serial, bool is_interactive)
{
	const std::string base_dir = rpcs3::utils::get_cache_dir_by_serial(serial);

	if (!ValidateRemoval(serial, base_dir, "SPU cache", is_interactive))
		return true;

	u32 files_removed = 0;
	u32 files_total = 0;

	const QStringList filter{ QStringLiteral("spu*.dat"), QStringLiteral("spu*.dat.gz"), QStringLiteral("spu*.obj"), QStringLiteral("spu*.obj.gz") };
	const QString q_base_dir = QString::fromStdString(base_dir);

	QDirIterator dir_iter(q_base_dir, filter, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (QFile::remove(filepath))
		{
			++files_removed;
			game_list_log.notice("Removed SPU cache file: %s", filepath);
		}
		else
		{
			game_list_log.warning("Could not remove SPU cache file: %s", filepath);
		}

		++files_total;
	}

	const bool success = files_total == files_removed;

	if (success)
		game_list_log.success("Removed SPU cache in %s", base_dir);
	else
		game_list_log.fatal("Only %d/%d SPU cache files could be removed in %s", files_removed, files_total, base_dir);

	if (QDir(q_base_dir).isEmpty())
	{
		if (fs::remove_dir(base_dir))
			game_list_log.notice("Removed empty SPU cache directory: %s", base_dir);
		else
			game_list_log.error("Could not remove empty SPU cache directory: '%s' (%s)", base_dir, fs::g_tls_error);
	}

	return success;
}

bool game_list_actions::RemoveHDD1Cache(const std::string& serial, bool is_interactive)
{
	const std::string base_dir = rpcs3::utils::get_hdd1_cache_dir();

	if (!ValidateRemoval(serial, base_dir, "HDD1 cache", is_interactive))
		return true;

	u32 dirs_removed = 0;
	u32 dirs_total = 0;

	const QStringList filter{ QString::fromStdString(serial + "_*") };
	const QString q_base_dir = QString::fromStdString(base_dir);

	QDirIterator dir_iter(q_base_dir, filter, QDir::Dirs | QDir::NoDotAndDotDot);

	while (dir_iter.hasNext())
	{
		const QString filepath = dir_iter.next();

		if (fs::remove_all(filepath.toStdString()))
		{
			++dirs_removed;
			game_list_log.notice("Removed HDD1 cache directory: %s", filepath);
		}
		else
		{
			game_list_log.warning("Could not completely remove HDD1 cache directory: %s", filepath);
		}

		++dirs_total;
	}

	const bool success = dirs_removed == dirs_total;

	if (success)
		game_list_log.success("Removed HDD1 cache in %s (%s)", base_dir, serial);
	else
		game_list_log.fatal("Only %d/%d HDD1 cache directories could be removed in %s (%s)", dirs_removed, dirs_total, base_dir, serial);

	return success;
}

bool game_list_actions::RemoveAllCaches(const std::string& serial, bool is_interactive)
{
	// Just used for confirmation, if requested. Folder returned by fs::get_config_dir() is always present!
	if (!ValidateRemoval(serial, fs::get_config_dir(), "all caches", is_interactive))
		return true;

	const std::string base_dir = rpcs3::utils::get_cache_dir_by_serial(serial);

	if (!ValidateRemoval(serial, base_dir, "main cache", false)) // no interation needed here
		return true;

	bool success = false;

	if (fs::remove_all(base_dir))
	{
		success = true;
		game_list_log.success("Removed main cache in %s", base_dir);

	}
	else
	{
		game_list_log.fatal("Could not completely remove main cache in %s (%s)", base_dir, serial);
	}

	success |= RemoveHDD1Cache(serial);

	return success;
}

bool game_list_actions::RemoveContentList(const std::string& serial, bool is_interactive)
{
	// Just used for confirmation, if requested. Folder returned by fs::get_config_dir() is always present!
	if (!ValidateRemoval(serial, fs::get_config_dir(), "selected content", is_interactive))
	{
		if (m_content_info.clear_on_finish)
			ClearContentList(); // Clear only the content's info

		return true;
	}

	u16 content_types = m_content_info.content_types;

	// Remove data path in "dev_hdd0/game" folder (if any)
	if (content_types & DATA)
	{
		if (const auto it = m_content_info.path_list.find(serial); it != m_content_info.path_list.cend())
		{
			if (RemoveContentPathList(it->second, "data") != it->second.size())
			{
				if (m_content_info.clear_on_finish)
					ClearContentList(); // Clear only the content's info

				// Skip the removal of the remaining selected contents in case some data paths could not be removed
				return false;
			}
		}
	}

	// Add serial (title id) to the list of serials to be removed in "games.yml" file (if any)
	if (content_types & DISC)
	{
		if (m_content_info.disc_list.contains(serial))
			m_content_info.removed_disc_list.insert(serial);
	}

	// Remove lock file in "dev_hdd0/game/＄locks" folder (if any)
	if (content_types & LOCKS)
	{
		if (ValidateRemoval(serial, rpcs3::utils::get_hdd0_locks_dir(), "lock"))
			RemoveContentBySerial(rpcs3::utils::get_hdd0_locks_dir(), serial, "lock");
	}

	// Remove caches in "cache" and "dev_hdd1/caches" folders (if any)
	if (content_types & CACHES)
	{
		if (ValidateRemoval(serial, rpcs3::utils::get_cache_dir_by_serial(serial), "all caches"))
			RemoveAllCaches(serial);
	}

	// Remove custom configs in "config/custom_config" folder (if any)
	if (content_types & CUSTOM_CONFIG)
	{
		if (ValidateRemoval(serial, rpcs3::utils::get_custom_config_path(serial), "custom configuration"))
			RemoveCustomConfiguration(serial);

		if (ValidateRemoval(serial, rpcs3::utils::get_input_config_dir(serial), "custom gamepad configuration"))
			RemoveCustomPadConfiguration(serial);
	}

	// Remove icons in "Icons/game_icons" folder (if any)
	if (content_types & ICONS)
	{
		if (ValidateRemoval(serial, rpcs3::utils::get_icons_dir(serial), "icons"))
			RemoveContentBySerial(rpcs3::utils::get_icons_dir(), serial, "icons");
	}

	// Remove shortcuts in "games/shortcuts" folder and from desktop / start menu / Steam (if any)
	if (content_types & SHORTCUTS)
	{
		if (const auto it = m_content_info.name_list.find(serial); it != m_content_info.name_list.cend())
		{
			std::vector<std::pair<std::string, std::string>> games;
			for (const std::string& name : it->second)
			{
				games.push_back(std::pair(name, serial));
			}

			// Batch remove all shortcuts
			gui::utils::batch_remove_shortcuts(games);
		}
	}

	if (content_types & SAVESTATES)
	{
		if (ValidateRemoval(serial, rpcs3::utils::get_savestates_dir(serial), "savestates"))
			RemoveContentBySerial(rpcs3::utils::get_savestates_dir(), serial, "savestates");
	}

	if (content_types & CAPTURES)
	{
		if (ValidateRemoval(serial, rpcs3::utils::get_captures_dir(), "captures"))
			RemoveContentBySerial(rpcs3::utils::get_captures_dir(), serial, "captures");
	}

	if (content_types & RECORDINGS)
	{
		if (ValidateRemoval(serial, rpcs3::utils::get_recordings_dir(serial), "recordings"))
			RemoveContentBySerial(rpcs3::utils::get_recordings_dir(), serial, "recordings");
	}

	if (content_types & SCREENSHOTS)
	{
		if (ValidateRemoval(serial, rpcs3::utils::get_screenshots_dir(serial), "screenshots"))
			RemoveContentBySerial(rpcs3::utils::get_screenshots_dir(), serial, "screenshots");
	}

	if (m_content_info.clear_on_finish)
		ClearContentList(true); // Update the game list and clear the content's info once removed

	return true;
}

void game_list_actions::BatchActionBySerials(progress_dialog* pdlg, const std::set<std::string>& serials,
	QString progressLabel, std::function<bool(const std::string&)> action,
	std::function<void(u32, u32)> cancel_log, std::function<void()> action_on_finish, bool refresh_on_finish,
	bool can_be_concurrent, std::function<bool()> should_wait_cb)
{
	// Concurrent tasks should not wait (at least not in current implementation)
	ensure(!should_wait_cb || !can_be_concurrent);

	g_system_progress_canceled = false;

	const std::shared_ptr<std::function<bool(int)>> iterate_over_serial = std::make_shared<std::function<bool(int)>>();

	const std::shared_ptr<atomic_t<int>> index = std::make_shared<atomic_t<int>>(0);

	const int serials_size = ::narrow<int>(serials.size());

	*iterate_over_serial = [=, index_ptr = index](int index)
	{
		if (index == serials_size)
		{
			return false;
		}

		const std::string& serial = *std::next(serials.begin(), index);

		if (pdlg->wasCanceled() || g_system_progress_canceled.exchange(false))
		{
			if (cancel_log)
			{
				cancel_log(index, serials_size);
			}
			return false;
		}

		if (action(serial))
		{
			const int done = index_ptr->load();
			pdlg->setLabelText(progressLabel.arg(done + 1).arg(serials_size));
			pdlg->SetValue(done + 1);
		}

		(*index_ptr)++;
		return true;
	};

	if (can_be_concurrent)
	{
		// Unused currently

		QList<int> indices;

		for (int i = 0; i < serials_size; i++)
		{
			indices.append(i);
		}

		QFutureWatcher<void>* future_watcher = new QFutureWatcher<void>(m_game_list_frame);

		future_watcher->setFuture(QtConcurrent::map(std::move(indices), *iterate_over_serial));

		connect(future_watcher, &QFutureWatcher<void>::finished, m_game_list_frame, [=, this]()
		{
			pdlg->setLabelText(progressLabel.arg(index->load()).arg(serials_size));
			pdlg->setCancelButtonText(tr("OK"));
			QApplication::beep();

			if (action_on_finish)
			{
				action_on_finish();
			}

			if (refresh_on_finish && index)
			{
				m_game_list_frame->Refresh(true);
			}

			future_watcher->deleteLater();
		});

		return;
	}

	const std::shared_ptr<std::function<void()>> periodic_func = std::make_shared<std::function<void()>>();

	*periodic_func = [=, this]()
	{
		if (should_wait_cb && should_wait_cb())
		{
			// Conditions are not met for execution
			// Check again later
			QTimer::singleShot(5, m_game_list_frame, *periodic_func);
			return;
		}

		if ((*iterate_over_serial)(*index))
		{
			QTimer::singleShot(1, m_game_list_frame, *periodic_func);
			return;
		}

		pdlg->setLabelText(progressLabel.arg(index->load()).arg(serials_size));
		pdlg->setCancelButtonText(tr("OK"));
		connect(pdlg, &progress_dialog::canceled, m_game_list_frame, [pdlg](){ pdlg->deleteLater(); });
		QApplication::beep();

		if (action_on_finish)
		{
			action_on_finish();
		}

		// Signal termination back to the callback
		action("");

		if (refresh_on_finish && index)
		{
			m_game_list_frame->Refresh(true);
		}
	};

	// Invoked on the next event loop processing iteration
	QTimer::singleShot(1, m_game_list_frame, *periodic_func);
}

void game_list_actions::BatchCreateCPUCaches(const std::vector<game_info>& games, bool is_fast_compilation, bool is_interactive)
{
	if (is_interactive && QMessageBox::question(m_game_list_frame, tr("Confirm Creation"), tr("Create LLVM cache?")) != QMessageBox::Yes)
	{
		return;
	}

	std::set<std::string> serials;

	if (games.empty())
	{
		serials.emplace("vsh.self");
	}

	for (const auto& game : (games.empty() ? m_game_list_frame->GetGameInfo() : games))
	{
		serials.emplace(game->info.serial);
	}

	const usz total = serials.size();

	if (total == 0)
	{
		QMessageBox::information(m_game_list_frame, tr("LLVM Cache Batch Creation"), tr("No titles found"), QMessageBox::Ok);
		return;
	}

	if (!m_gui_settings->GetBootConfirmation(m_game_list_frame))
	{
		return;
	}

	const QString main_label = tr("Creating all LLVM caches");

	progress_dialog* pdlg = new progress_dialog(tr("LLVM Cache Batch Creation"), main_label, tr("Cancel"), 0, ::narrow<s32>(total), false, m_game_list_frame);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->open();

	connect(pdlg, &progress_dialog::canceled, m_game_list_frame, []()
	{
		if (!Emu.IsStopped())
		{
			Emu.GracefulShutdown(false, true);
		}
	});

	BatchActionBySerials(pdlg, serials, tr("%0\nProgress: %1/%2 caches compiled").arg(main_label),
		[this, is_fast_compilation](const std::string& serial)
		{
			if (serial.empty())
			{
				return false;
			}

			if (Emu.IsStopped(true))
			{
				const auto& games = m_game_list_frame->GetGameInfo();

				const auto it = std::find_if(games.cbegin(), games.cend(), FN(x->info.serial == serial));

				if (it != games.cend())
				{
					return CreateCPUCaches((*it)->info.path, serial, is_fast_compilation);
				}
			}

			return false;
		},
		[](u32, u32)
		{
			game_list_log.notice("LLVM Cache Batch Creation was canceled");
		}, nullptr, false, false,
		[]()
		{
			return !Emu.IsStopped(true);
		});
}

void game_list_actions::BatchRemoveCustomConfigurations(const std::vector<game_info>& games, bool is_interactive)
{
	if (is_interactive && QMessageBox::question(m_game_list_frame, tr("Confirm Removal"), tr("Remove custom configuration?")) != QMessageBox::Yes)
	{
		return;
	}

	std::set<std::string> serials;

	for (const auto& game : (games.empty() ? m_game_list_frame->GetGameInfo() : games))
	{
		if (game->has_custom_config && !serials.count(game->info.serial))
		{
			serials.emplace(game->info.serial);
		}
	}

	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(m_game_list_frame, tr("Custom Configuration Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("Custom Configuration Batch Removal"), tr("Removing all custom configurations"), tr("Cancel"), 0, total, false, m_game_list_frame);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->open();

	BatchActionBySerials(pdlg, serials, tr("%0/%1 custom configurations cleared"), [this](const std::string& serial)
		{
			return !serial.empty() && Emu.IsStopped(true) && RemoveCustomConfiguration(serial);
		},
		[](u32 removed, u32 total)
		{
			game_list_log.notice("Custom Configuration Batch Removal was canceled. %d/%d custom configurations cleared", removed, total);
		}, nullptr, true);
}

void game_list_actions::BatchRemoveCustomPadConfigurations(const std::vector<game_info>& games, bool is_interactive)
{
	if (is_interactive && QMessageBox::question(m_game_list_frame, tr("Confirm Removal"), tr("Remove custom gamepad configuration?")) != QMessageBox::Yes)
	{
		return;
	}

	std::set<std::string> serials;

	for (const auto& game : (games.empty() ? m_game_list_frame->GetGameInfo() : games))
	{
		if (game->has_custom_pad_config && !serials.count(game->info.serial))
		{
			serials.emplace(game->info.serial);
		}
	}

	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(m_game_list_frame, tr("Custom Gamepad Configuration Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("Custom Gamepad Configuration Batch Removal"), tr("Removing all custom gamepad configurations"), tr("Cancel"), 0, total, false, m_game_list_frame);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->open();

	BatchActionBySerials(pdlg, serials, tr("%0/%1 custom gamepad configurations cleared"), [this](const std::string& serial)
		{
			return !serial.empty() && Emu.IsStopped(true) && RemoveCustomPadConfiguration(serial);
		},
		[](u32 removed, u32 total)
		{
			game_list_log.notice("Custom Gamepad Configuration Batch Removal was canceled. %d/%d custom gamepad configurations cleared", removed, total);
		}, nullptr, true);
}

void game_list_actions::BatchRemoveShaderCaches(const std::vector<game_info>& games, bool is_interactive)
{
	if (!ValidateBatchRemoval("shader cache", is_interactive))
	{
		return;
	}

	std::set<std::string> serials;

	if (games.empty())
	{
		serials.emplace("vsh.self");
	}

	for (const auto& game : (games.empty() ? m_game_list_frame->GetGameInfo() : games))
	{
		serials.emplace(game->info.serial);
	}

	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(m_game_list_frame, tr("Shader Cache Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("Shader Cache Batch Removal"), tr("Removing all shader caches"), tr("Cancel"), 0, total, false, m_game_list_frame);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->open();

	BatchActionBySerials(pdlg, serials, tr("%0/%1 shader caches cleared"), [this](const std::string& serial)
		{
			return !serial.empty() && Emu.IsStopped(true) && RemoveShaderCache(serial);
		},
		[](u32 removed, u32 total)
		{
			game_list_log.notice("Shader Cache Batch Removal was canceled. %d/%d caches cleared", removed, total);
		}, nullptr, false);
}

void game_list_actions::BatchRemovePPUCaches(const std::vector<game_info>& games, bool is_interactive)
{
	if (!ValidateBatchRemoval("PPU cache", is_interactive))
	{
		return;
	}

	std::set<std::string> serials;

	if (games.empty())
	{
		serials.emplace("vsh.self");
	}

	for (const auto& game : (games.empty() ? m_game_list_frame->GetGameInfo() : games))
	{
		serials.emplace(game->info.serial);
	}

	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(m_game_list_frame, tr("PPU Cache Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("PPU Cache Batch Removal"), tr("Removing all PPU caches"), tr("Cancel"), 0, total, false, m_game_list_frame);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->open();

	BatchActionBySerials(pdlg, serials, tr("%0/%1 PPU caches cleared"),
		[this](const std::string& serial)
		{
			return !serial.empty() && Emu.IsStopped(true) && RemovePPUCache(serial);
		},
		[](u32 removed, u32 total)
		{
			game_list_log.notice("PPU Cache Batch Removal was canceled. %d/%d caches cleared", removed, total);
		}, nullptr, false);
}

void game_list_actions::BatchRemoveSPUCaches(const std::vector<game_info>& games, bool is_interactive)
{
	if (!ValidateBatchRemoval("SPU cache", is_interactive))
	{
		return;
	}

	std::set<std::string> serials;

	if (games.empty())
	{
		serials.emplace("vsh.self");
	}

	for (const auto& game : (games.empty() ? m_game_list_frame->GetGameInfo() : games))
	{
		serials.emplace(game->info.serial);
	}

	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(m_game_list_frame, tr("SPU Cache Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("SPU Cache Batch Removal"), tr("Removing all SPU caches"), tr("Cancel"), 0, total, false, m_game_list_frame);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->open();

	BatchActionBySerials(pdlg, serials, tr("%0/%1 SPU caches cleared"),
		[this](const std::string& serial)
		{
			return !serial.empty() && Emu.IsStopped(true) && RemoveSPUCache(serial);
		},
		[](u32 removed, u32 total)
		{
			game_list_log.notice("SPU Cache Batch Removal was canceled. %d/%d caches cleared", removed, total);
		}, nullptr, false);
}

void game_list_actions::BatchRemoveHDD1Caches(const std::vector<game_info>& games, bool is_interactive)
{
	if (!ValidateBatchRemoval("HDD1 cache", is_interactive))
	{
		return;
	}

	std::set<std::string> serials;

	if (games.empty())
	{
		serials.emplace("vsh.self");
	}

	for (const auto& game : (games.empty() ? m_game_list_frame->GetGameInfo() : games))
	{
		serials.emplace(game->info.serial);
	}

	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(m_game_list_frame, tr("HDD1 Cache Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("HDD1 Cache Batch Removal"), tr("Removing all HDD1 caches"), tr("Cancel"), 0, total, false, m_game_list_frame);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->open();

	BatchActionBySerials(pdlg, serials, tr("%0/%1 HDD1 caches cleared"),
		[this](const std::string& serial)
		{
			return !serial.empty() && Emu.IsStopped(true) && RemoveHDD1Cache(serial);
		},
		[](u32 removed, u32 total)
		{
			game_list_log.notice("HDD1 Cache Batch Removal was canceled. %d/%d caches cleared", removed, total);
		}, nullptr, false);
}

void game_list_actions::BatchRemoveAllCaches(const std::vector<game_info>& games, bool is_interactive)
{
	if (!ValidateBatchRemoval("all caches", is_interactive))
	{
		return;
	}

	std::set<std::string> serials;

	if (games.empty())
	{
		serials.emplace("vsh.self");
	}

	for (const auto& game : (games.empty() ? m_game_list_frame->GetGameInfo() : games))
	{
		serials.emplace(game->info.serial);
	}

	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(m_game_list_frame, tr("Cache Batch Removal"), tr("No files found"), QMessageBox::Ok);
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("Cache Batch Removal"), tr("Removing all caches"), tr("Cancel"), 0, total, false, m_game_list_frame);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->open();

	BatchActionBySerials(pdlg, serials, tr("%0/%1 caches cleared"),
		[this](const std::string& serial)
		{
			return !serial.empty() && Emu.IsStopped(true) && RemoveAllCaches(serial);
		},
		[](u32 removed, u32 total)
		{
			game_list_log.notice("Cache Batch Removal was canceled. %d/%d caches cleared", removed, total);
		}, nullptr, false);
}

void game_list_actions::BatchRemoveContentLists(const std::vector<game_info>& games, bool is_interactive)
{
	// Let the batch process (not RemoveContentList()) make cleanup when terminated
	m_content_info.clear_on_finish = false;

	if (!ValidateBatchRemoval("selected content", is_interactive))
	{
		ClearContentList(); // Clear only the content's info
		return;
	}

	std::set<std::string> serials;

	if (games.empty())
	{
		serials.emplace("vsh.self");
	}

	for (const auto& game : (games.empty() ? m_game_list_frame->GetGameInfo() : games))
	{
		serials.emplace(game->info.serial);
	}

	const u32 total = ::size32(serials);

	if (total == 0)
	{
		QMessageBox::information(m_game_list_frame, tr("Content Batch Removal"), tr("No files found"), QMessageBox::Ok);

		ClearContentList(); // Clear only the content's info
		return;
	}

	progress_dialog* pdlg = new progress_dialog(tr("Content Batch Removal"), tr("Removing all contents"), tr("Cancel"), 0, total, false, m_game_list_frame);
	pdlg->setAutoClose(false);
	pdlg->setAutoReset(false);
	pdlg->open();

	BatchActionBySerials(pdlg, serials, tr("%0/%1 contents cleared"),
		[this](const std::string& serial)
		{
			return !serial.empty() && Emu.IsStopped(true) && RemoveContentList(serial);
		},
		[](u32 removed, u32 total)
		{
			game_list_log.notice("Content Batch Removal was canceled. %d/%d contents cleared", removed, total);
		},
		[this]() // Make cleanup when batch process terminated
		{
			ClearContentList(true); // Update the game list and clear the content's info once removed
		}, false);
}

void game_list_actions::CreateShortcuts(const std::vector<game_info>& games, const std::set<gui::utils::shortcut_location>& locations)
{
	if (games.empty())
	{
		game_list_log.notice("Skip creating shortcuts. No games selected.");
		return;
	}

	if (locations.empty())
	{
		game_list_log.error("Failed to create shortcuts. No locations selected.");
		return;
	}

	const bool success = gui::utils::batch_create_shortcuts(games, locations);

#ifdef _WIN32
	if (locations.size() == 1 && locations.contains(gui::utils::shortcut_location::rpcs3_shortcuts))
	{
		return;
	}
#endif

	if (success)
	{
		QMessageBox::information(m_game_list_frame, tr("Success!"), tr("Successfully created shortcut(s)."));
	}
	else
	{
		QMessageBox::warning(m_game_list_frame, tr("Warning!"), tr("Failed to create one or more shortcuts!"));
	}
}

bool game_list_actions::RemoveContentPath(const std::string& path, const std::string& desc)
{
	if (!fs::exists(path))
	{
		return true;
	}

	if (fs::is_dir(path))
	{
		if (fs::remove_all(path))
		{
			game_list_log.notice("Removed '%s' directory: '%s'", desc, path);
		}
		else
		{
			game_list_log.error("Could not remove '%s' directory: '%s' (%s)", desc, path, fs::g_tls_error);

			return false;
		}
	}
	else // If file
	{
		if (fs::remove_file(path))
		{
			game_list_log.notice("Removed '%s' file: '%s'", desc, path);
		}
		else
		{
			game_list_log.error("Could not remove '%s' file: '%s' (%s)", desc, path, fs::g_tls_error);

			return false;
		}
	}

	return true;
}

u32 game_list_actions::RemoveContentPathList(const std::set<std::string>& path_list, const std::string& desc)
{
	u32 paths_removed = 0;

	for (const std::string& path : path_list)
	{
		if (RemoveContentPath(path, desc))
		{
			paths_removed++;
		}
	}

	return paths_removed;
}

bool game_list_actions::RemoveContentBySerial(const std::string& base_dir, const std::string& serial, const std::string& desc)
{
	bool success = true;

	for (const auto& entry : fs::dir(base_dir))
	{
		// Search for any path starting with serial (e.g. BCES01118_BCES01118)
		if (!entry.name.starts_with(serial))
		{
			continue;
		}

		if (!RemoveContentPath(base_dir + entry.name, desc))
		{
			success = false; // Mark as failed if there is at least one failure
		}
	}

	return success;
}
