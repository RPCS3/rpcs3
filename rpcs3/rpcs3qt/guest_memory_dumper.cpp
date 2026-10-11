#include "stdafx.h"
#include "guest_memory_dumper.h"
#include "progress_dialog.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Memory/vm.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/System.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QStorageInfo>

LOG_CHANNEL(gui_log, "GUI");

extern void qt_events_aware_op(int repeat_duration_ms, std::function<bool()> wrapped_op);

guest_memory_dumper::guest_memory_dumper(QWidget* parent, bool delete_later)
	: QObject(parent), m_parent(parent), m_delete_later(delete_later)
{
}

// Emu.Pause() only requests the pause: every emulated thread drains into its
// wait or stop state asynchronously. The dump must not begin until each PPU,
// SPU and RSX thread has actually settled, or it could capture torn state.
bool guest_memory_dumper::emulated_processors_quiesced()
{
	bool quiesced = true;
	const auto check_cpu = [&](u32, cpu_thread& cpu)
	{
		const auto state = +cpu.state;
		if (!::is_stopped(state) && !(state & cpu_flag::wait))
		{
			quiesced = false;
		}
	};

	if (g_fxo->is_init<id_manager::id_map<named_thread<ppu_thread>>>())
	{
		idm::select<named_thread<ppu_thread>>(check_cpu);
	}

	if (g_fxo->is_init<id_manager::id_map<named_thread<spu_thread>>>())
	{
		idm::select<named_thread<spu_thread>>(check_cpu);
	}

	if (const auto rsx = g_fxo->try_get<rsx::thread>())
	{
		check_cpu(0, *rsx);
	}

	return quiesced;
}

QString guest_memory_dumper::hex_u64(u64 value, int width)
{
	return QStringLiteral("0x%1").arg(value, width, 16, QLatin1Char('0'));
}

void guest_memory_dumper::dump_guest_memory()
{
	if (Emu.IsStopped() || Emu.IsStarting())
	{
		QMessageBox::warning(m_parent, tr("Dump Guest Memory"), tr("Start a game before creating a guest-memory dump."));
		return;
	}

	const QString default_parent = QString::fromStdString(fs::get_config_dir() + "guest_memory_dumps");
	QDir().mkpath(default_parent);

	QString title_id = QString::fromStdString(Emu.GetTitleID());
	if (title_id.isEmpty())
	{
		title_id = QStringLiteral("RPCS3");
	}

	// Remove the output folder again unless the dump ran to completion, so a
	// failed or cancelled dump does not leave an empty folder or a partial
	// image behind. This is declared before the output file so it destructs
	// after the file is closed.
	struct cleanup_guard
	{
		QDir dir;
		bool enabled = false;
		bool keep = false;
		~cleanup_guard()
		{
			if (enabled && !keep && !dir.removeRecursively())
			{
				gui_log.error("Could not remove incomplete guest memory dump folder: %s", dir.absolutePath());
			}
		}
	} cleanup;

	QString picker_parent = default_parent;
	QDir output;
	fs::file guest_file;
	bool sparse_image = false;

	while (!sparse_image)
	{
		const QString selected_parent = QFileDialog::getExistingDirectory(m_parent, tr("Select Guest Memory Dump Folder"), picker_parent, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

		if (selected_parent.isEmpty())
		{
			return;
		}

		// The native folder dialog runs its own event loop, so the game may have
		// stopped while the user was choosing a destination.
		if (Emu.IsStopped() || Emu.IsStarting())
		{
			QMessageBox::warning(m_parent, tr("Dump Guest Memory"), tr("The game stopped before the guest-memory dump could begin."));
			return;
		}

		const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
		const QString folder_stem = QStringLiteral("%1_guest_memory_%2").arg(title_id, timestamp);
		QDir parent(selected_parent);
		QString folder_name = folder_stem;

		for (u32 suffix = 2; parent.exists(folder_name); suffix++)
		{
			folder_name = QStringLiteral("%1_%2").arg(folder_stem).arg(suffix);
		}

		if (!parent.mkpath(folder_name))
		{
			QMessageBox::critical(m_parent, tr("Dump Guest Memory"), tr("Could not create the output folder:\n%0").arg(parent.filePath(folder_name)));
			return;
		}

		output.setPath(parent.filePath(folder_name));
		cleanup.dir = output;
		cleanup.enabled = true;

		const QString guest_path = output.filePath(QStringLiteral("guest_memory.bin"));
		if (!guest_file.open(guest_path.toStdString(), fs::rewrite))
		{
			QMessageBox::critical(m_parent, tr("Dump Guest Memory"), tr("Could not create:\n%0").arg(guest_path));
			return;
		}

		// POSIX file systems create holes for unwritten ranges by default; Windows
		// needs the sparse attribute set explicitly.
		sparse_image = fs::set_sparse(guest_file);

		if (!sparse_image)
		{
			guest_file.close();
			if (!output.removeRecursively())
			{
				QMessageBox::critical(m_parent, tr("Dump Guest Memory"), tr("The selected destination does not support sparse files, and the incomplete output folder could not be removed:\n%0").arg(output.absolutePath()));
				return;
			}

			cleanup.enabled = false;
			QMessageBox message(QMessageBox::Critical, tr("Dump Guest Memory"), tr("The selected destination does not support sparse files. Choose a location on an NTFS volume."), QMessageBox::NoButton, m_parent);
			QPushButton* choose_button = message.addButton(tr("Choose Another Location"), QMessageBox::AcceptRole);
			message.addButton(QMessageBox::Cancel);
			message.setDefaultButton(choose_button);
			message.exec();

			if (message.clickedButton() != choose_button)
			{
				return;
			}

			picker_parent = selected_parent;
		}
	}

	const bool resume_after_dump = Emu.IsRunning();

	if (resume_after_dump && !Emu.Pause(false, false))
	{
		QMessageBox::critical(m_parent, tr("Dump Guest Memory"), tr("Could not pause emulation for a consistent memory snapshot."));
		return;
	}

	struct resume_guard
	{
		bool enabled = false;
		~resume_guard()
		{
			if (enabled)
			{
				Emu.Resume();
			}
		}
	} resume{resume_after_dump};

	bool quiesced = false;
	QElapsedTimer pause_timer;
	pause_timer.start();
	qt_events_aware_op(5, [&]()
	{
		quiesced = emulated_processors_quiesced();
		return quiesced || pause_timer.elapsed() >= 5000;
	});

	if (!quiesced)
	{
		QMessageBox::critical(m_parent, tr("Dump Guest Memory"), tr("One or more emulated processors did not stop within five seconds. No dump was created."));
		return;
	}

	std::vector<guest_memory_region> regions;
	guest_memory_region current{};

	const auto finish_region = [&]()
	{
		if (current.size)
		{
			regions.emplace_back(current);
			current = {};
		}
	};

	// Walk the guest page table and coalesce contiguous pages with identical
	// flags into regions, so the manifest mirrors the guest memory map instead
	// of listing a million single pages.
	for (u32 page = 0; page < guest_address_space_size / guest_page_size; page++)
	{
		const u32 address = static_cast<u32>(static_cast<u64>(page) * guest_page_size);
		const auto [allocated, flags] = vm::get_addr_flags(address);

		if (!allocated)
		{
			finish_region();
			continue;
		}

		if (current.size && (current.flags != flags || static_cast<u64>(current.start) + current.size != address))
		{
			finish_region();
		}

		if (!current.size)
		{
			current.start = address;
			current.flags = flags;
		}

		current.size += guest_page_size;
	}

	finish_region();

	// SPU local store is also part of the guest map (at each thread's
	// vm_offset), but a per thread copy with id, pc and type recorded in the
	// manifest is much easier to analyze than digging it out of the main image.
	std::vector<spu_local_store_dump> spu_dumps;
	if (g_fxo->is_init<id_manager::id_map<named_thread<spu_thread>>>())
	{
		idm::select<named_thread<spu_thread>>([&](u32 id, spu_thread& spu)
		{
			spu_local_store_dump dump;
			dump.id = id;
			dump.lv2_id = spu.lv2_id;
			dump.index = spu.index;
			dump.pc = spu.pc;
			dump.vm_offset = spu.vm_offset();
			dump.type = static_cast<u32>(spu.get_type());
			dump.name = spu.get_name();
			spu_dumps.emplace_back(std::move(dump));
		});
	}

	u64 guest_bytes = 0;
	for (const auto& region : regions)
	{
		guest_bytes += region.size;
	}

	const u64 total_bytes = guest_bytes + static_cast<u64>(spu_dumps.size()) * SPU_LS_SIZE;

	// Preflight the destination. A sparse image costs roughly the allocated
	// bytes, so warn when even that plus some margin does not fit.
	const QStorageInfo storage(output.absolutePath());
	const u64 available = storage.bytesAvailable() > 0 ? static_cast<u64>(storage.bytesAvailable()) : 0;

	if (const u64 needed = total_bytes + (64ull << 20); available < needed)
	{
		if (QMessageBox::question(m_parent, tr("Dump Guest Memory"), tr("The destination may not have enough free space (roughly %0 MiB needed, %1 MiB available).\n\nContinue anyway?").arg(needed >> 20).arg(available >> 20)) != QMessageBox::Yes)
		{
			return;
		}
	}

	atomic_t<u64> completed_bytes{0};
	progress_dialog progress(tr("Dump Guest Memory"), tr("Dumping PS3 guest memory..."), tr("Cancel"), 0, 1000, false, m_parent);
	progress.setCancelButton(nullptr);
	progress.setMinimumDuration(0);

	if (!guest_file.trunc(guest_address_space_size))
	{
		QMessageBox::critical(m_parent, tr("Dump Guest Memory"), tr("Could not size the 4 GB address image."));
		return;
	}

	// Read through the sudo mirror: it maps every allocated page as read/write,
	// so pages the guest keeps protected (reservation notification, no access
	// areas) are captured as well instead of faulting or being skipped. Copy
	// into ordinary host memory before passing data to fs::file, and perform all
	// guest-memory reads and file writes off the UI thread.
	enum class dump_write_error
	{
		none,
		guest_seek,
		guest_write,
		spu_write,
	};

	dump_write_error write_error = dump_write_error::none;
	u64 error_address = 0;
	QString error_path;
	const QString output_path = output.absolutePath();

	named_thread dump_thread("Guest Memory Dumper", [&]()
	{
		std::vector<u8> middle_buffer(dump_io_chunk_size);

		for (const auto& region : regions)
		{
			if (guest_file.seek(region.start) != region.start)
			{
				write_error = dump_write_error::guest_seek;
				error_address = region.start;
				return;
			}

			for (u64 offset = 0; offset < region.size;)
			{
				const u64 chunk_size = std::min(dump_io_chunk_size, region.size - offset);
				const u64 address = static_cast<u64>(region.start) + offset;

				std::memcpy(middle_buffer.data(), vm::g_sudo_addr + address, chunk_size);
				if (guest_file.write(middle_buffer.data(), chunk_size) != chunk_size)
				{
					write_error = dump_write_error::guest_write;
					error_address = address;
					return;
				}

				offset += chunk_size;
				completed_bytes.fetch_add(chunk_size);
			}
		}

		guest_file.close();

		QDir worker_output(output_path);
		for (const auto& dump : spu_dumps)
		{
			const QString filename = QStringLiteral("spu_%1_ls.bin").arg(dump.id, 8, 16, QLatin1Char('0'));
			const QString file_path = worker_output.filePath(filename);
			std::memcpy(middle_buffer.data(), vm::g_sudo_addr + dump.vm_offset, SPU_LS_SIZE);
			fs::file file(file_path.toStdString(), fs::rewrite);

			if (!file || file.write(middle_buffer.data(), SPU_LS_SIZE) != SPU_LS_SIZE)
			{
				write_error = dump_write_error::spu_write;
				error_path = file_path;
				return;
			}

			completed_bytes.fetch_add(SPU_LS_SIZE);
		}
	});

	qt_events_aware_op(10, [&]()
	{
		const u64 completed = completed_bytes.load();
		progress.SetValue(total_bytes ? static_cast<int>(completed * 1000 / total_bytes) : 1000);
		return static_cast<thread_state>(dump_thread) == thread_state::finished;
	});
	dump_thread();

	if (write_error == dump_write_error::guest_seek)
	{
		QMessageBox::critical(m_parent, tr("Dump Guest Memory"), tr("Could not seek to guest address %0 in the output file.").arg(hex_u64(error_address)));
		return;
	}

	if (write_error == dump_write_error::guest_write)
	{
		QMessageBox::critical(m_parent, tr("Dump Guest Memory"), tr("Writing failed at guest address %0. The dump is incomplete.").arg(hex_u64(error_address)));
		return;
	}

	if (write_error == dump_write_error::spu_write)
	{
		QMessageBox::critical(m_parent, tr("Dump Guest Memory"), tr("Could not write SPU local store:\n%0").arg(error_path));
		return;
	}

	QJsonArray spu_json;
	for (const auto& dump : spu_dumps)
	{
		const QString filename = QStringLiteral("spu_%1_ls.bin").arg(dump.id, 8, 16, QLatin1Char('0'));
		QJsonObject item;
		item.insert(QStringLiteral("file"), filename);
		item.insert(QStringLiteral("id"), hex_u64(dump.id));
		item.insert(QStringLiteral("lv2_id"), hex_u64(dump.lv2_id));
		item.insert(QStringLiteral("index"), static_cast<int>(dump.index));
		item.insert(QStringLiteral("pc"), hex_u64(dump.pc, 5));
		item.insert(QStringLiteral("vm_offset"), hex_u64(dump.vm_offset));
		item.insert(QStringLiteral("type"), static_cast<int>(dump.type));
		item.insert(QStringLiteral("name"), QString::fromStdString(dump.name));
		item.insert(QStringLiteral("size"), static_cast<int>(SPU_LS_SIZE));
		spu_json.append(item);
	}

	QJsonArray region_json;
	for (const auto& region : regions)
	{
		QJsonObject item;
		item.insert(QStringLiteral("start"), hex_u64(region.start));
		item.insert(QStringLiteral("end_exclusive"), hex_u64(static_cast<u64>(region.start) + region.size));
		item.insert(QStringLiteral("size"), static_cast<double>(region.size));
		item.insert(QStringLiteral("file_offset"), hex_u64(region.start));
		item.insert(QStringLiteral("raw_page_flags"), static_cast<int>(region.flags));
		item.insert(QStringLiteral("readable"), !!(region.flags & vm::page_readable));
		item.insert(QStringLiteral("writable"), !!(region.flags & vm::page_writable));
		item.insert(QStringLiteral("executable"), !!(region.flags & vm::page_executable));
		region_json.append(item);
	}

	QJsonObject manifest;
	manifest.insert(QStringLiteral("format"), QStringLiteral("rpcs3-guest-memory-dump"));
	manifest.insert(QStringLiteral("version"), 1);
	manifest.insert(QStringLiteral("created_utc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
	manifest.insert(QStringLiteral("title"), QString::fromStdString(Emu.GetTitle()));
	manifest.insert(QStringLiteral("title_id"), QString::fromStdString(Emu.GetTitleID()));
	manifest.insert(QStringLiteral("address_space_file"), QStringLiteral("guest_memory.bin"));
	manifest.insert(QStringLiteral("address_space_size"), hex_u64(guest_address_space_size, 9));
	manifest.insert(QStringLiteral("page_size"), static_cast<int>(guest_page_size));
	manifest.insert(QStringLiteral("allocated_bytes"), static_cast<double>(guest_bytes));
	manifest.insert(QStringLiteral("file_offset_equals_guest_address"), true);
	manifest.insert(QStringLiteral("sparse_image"), sparse_image);
	manifest.insert(QStringLiteral("regions"), region_json);
	manifest.insert(QStringLiteral("spu_local_stores"), spu_json);

	QFile manifest_file(output.filePath(QStringLiteral("manifest.json")));
	if (!manifest_file.open(QIODevice::WriteOnly | QIODevice::Truncate) || manifest_file.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented)) < 0)
	{
		QMessageBox::critical(m_parent, tr("Dump Guest Memory"), tr("Could not write the dump manifest."));
		return;
	}

	manifest_file.close();
	cleanup.keep = true;
	progress.SetValue(1000);
	gui_log.success("Guest memory dump complete: %s (%u regions, %u SPU local stores, %u bytes allocated)", output.absolutePath(), regions.size(), spu_dumps.size(), guest_bytes);

	if (resume.enabled)
	{
		Emu.Resume();
		resume.enabled = false;
	}

	QMessageBox::information(m_parent, tr("Dump Guest Memory"), tr("Guest memory dump completed.\n\n%0\n\nThe 4 GB guest_memory.bin file is sparse: its file offset equals the PS3 guest address, while only allocated pages consume disk space.").arg(output.absolutePath()));

	if (m_delete_later)
	{
		deleteLater();
	}
}
