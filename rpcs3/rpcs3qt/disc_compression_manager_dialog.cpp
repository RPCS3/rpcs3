#include "stdafx.h"
#include "disc_compression_manager_dialog.h"
#include "game_source_dialog.h"

#include "qt_utils.h"
#include "Loader/ISO.h"
#include "Loader/ZAR.h"
#include "Utilities/File.h"
#include "Utilities/Thread.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <QAbstractItemView>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMetaObject>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>

LOG_CHANNEL(zar_gui_log, "ZAR GUI");

namespace
{
	enum column : int
	{
		game = 0,
		format,
		input_size,
		key,
		output,
		status,
		count,
	};

	QString source_name(const QString& path)
	{
		const QFileInfo info(path);
		return info.isDir() ? info.fileName() : info.completeBaseName();
	}

	bool external_iso_key_not_required(iso_archive& archive)
	{
		std::unique_ptr<fs::file_base> eboot = archive.open("PS3_GAME/USRDIR/EBOOT.BIN");
		if (!eboot)
		{
			return false;
		}

		std::array<char, 3> magic{};
		return eboot->read(magic.data(), magic.size()) == magic.size() &&
			std::memcmp(magic.data(), "SCE", magic.size()) == 0;
	}
}

disc_compression_manager_dialog::disc_compression_manager_dialog(QWidget* parent)
	: QDialog(parent)
{
	setObjectName(QStringLiteral("disc_compression_manager_dialog"));
	setWindowTitle(tr("Disc Compression Manager"));
	resize(1120, 560);

	auto* main_layout = new QVBoxLayout(this);

	auto* description = new QLabel(tr("Create read-only ZArchive (.zar) files for PS3 disc games. Add games to the queue first, then press Compress. JB folders and ISO images are supported; existing ZArchive files are detected but are not recompressed."), this);
	description->setWordWrap(true);
	main_layout->addWidget(description);

	auto* output_layout = new QHBoxLayout();
	output_layout->addWidget(new QLabel(tr("Output directory:"), this));
	m_output_directory = new QLineEdit(this);
	m_output_directory->setPlaceholderText(tr("Same directory as each source game"));
	output_layout->addWidget(m_output_directory, 1);
	auto* browse_button = new QPushButton(tr("Browse..."), this);
	output_layout->addWidget(browse_button);
	main_layout->addLayout(output_layout);

	m_table = new QTableWidget(0, column::count, this);
	m_table->setHorizontalHeaderLabels({tr("Game"), tr("Format"), tr("Input Size"), tr("Key"), tr("Output"), tr("Status")});
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setAlternatingRowColors(true);
	m_table->verticalHeader()->setVisible(false);
	m_table->horizontalHeader()->setSectionResizeMode(column::game, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(column::format, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(column::input_size, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(column::key, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(column::output, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(column::status, QHeaderView::ResizeToContents);
	main_layout->addWidget(m_table, 1);

	m_current_label = new QLabel(tr("Ready"), this);
	main_layout->addWidget(m_current_label);
	m_progress = new QProgressBar(this);
	m_progress->setRange(0, 1000);
	m_progress->setValue(0);
	main_layout->addWidget(m_progress);

	auto* buttons = new QHBoxLayout();
	m_add_button = new QPushButton(tr("Add Games"), this);
	m_remove_button = new QPushButton(tr("Remove"), this);
	m_clear_button = new QPushButton(tr("Clear"), this);
	m_compress_button = new QPushButton(tr("Compress"), this);
	m_cancel_button = new QPushButton(tr("Cancel"), this);
	m_close_button = new QPushButton(tr("Close"), this);
	m_cancel_button->setEnabled(false);

	buttons->addWidget(m_add_button);
	buttons->addWidget(m_remove_button);
	buttons->addWidget(m_clear_button);
	buttons->addStretch(1);
	buttons->addWidget(m_compress_button);
	buttons->addWidget(m_cancel_button);
	buttons->addWidget(m_close_button);
	main_layout->addLayout(buttons);

	connect(m_add_button, &QPushButton::clicked, this, &disc_compression_manager_dialog::add_games);
	connect(m_remove_button, &QPushButton::clicked, this, &disc_compression_manager_dialog::remove_selected);
	connect(m_clear_button, &QPushButton::clicked, this, &disc_compression_manager_dialog::clear_queue);
	connect(m_compress_button, &QPushButton::clicked, this, &disc_compression_manager_dialog::start_compression);
	connect(m_cancel_button, &QPushButton::clicked, this, [this]()
	{
		m_cancel = true;
		m_cancel_button->setEnabled(false);
		m_current_label->setText(tr("Cancelling..."));
	});
	connect(m_close_button, &QPushButton::clicked, this, &QDialog::close);
	connect(browse_button, &QPushButton::clicked, this, &disc_compression_manager_dialog::choose_output_directory);
	connect(m_output_directory, &QLineEdit::editingFinished, this, [this]()
	{
		update_output_paths();
		refresh_table();
		refresh_buttons();
	});
	connect(m_table, &QTableWidget::itemSelectionChanged, this, &disc_compression_manager_dialog::refresh_buttons);

	refresh_buttons();
}

disc_compression_manager_dialog::~disc_compression_manager_dialog()
{
	m_cancel = true;
	if (m_thread && m_thread->isRunning())
	{
		m_thread->wait();
	}
}

QString disc_compression_manager_dialog::format_description(const iso_archive& archive) const
{
	switch (archive.source_type())
	{
	case iso_archive_source_type::decrypted_iso:
		return tr("Decrypted ISO");
	case iso_archive_source_type::encrypted_iso:
		return tr("Encrypted ISO");
	case iso_archive_source_type::zar_decrypted_iso:
		return tr("ZAR (Decrypted ISO)");
	case iso_archive_source_type::zar_encrypted_iso:
		return tr("ZAR (Encrypted ISO)");
	case iso_archive_source_type::zar_jb:
		return tr("ZAR (JB)");
	}

	return {};
}

void disc_compression_manager_dialog::closeEvent(QCloseEvent* event)
{
	if (!m_running)
	{
		event->accept();
		return;
	}

	m_cancel = true;
	m_cancel_button->setEnabled(false);
	m_current_label->setText(tr("Cancelling... Close the manager after the current operation stops."));
	event->ignore();
}

void disc_compression_manager_dialog::add_games()
{
	game_source_dialog dialog(this,
	{
		.caption = tr("Add PS3 Disc Games"),
		.accept_label = tr("Add Games"),
		.allow_multiple = true,
		.validate_file_sources = false,
	});
	if (dialog.exec() != QDialog::Accepted)
	{
		return;
	}

	for (const QString& source : dialog.selected_sources())
	{
		add_source(source);
	}

	update_output_paths();
	refresh_table();
	refresh_buttons();
}

void disc_compression_manager_dialog::add_source(const QString& source)
{
	const QFileInfo info(source);
	const QString absolute = QDir::cleanPath(info.absoluteFilePath());

	for (const queue_item& existing : m_items)
	{
		if (QDir::cleanPath(existing.source) == absolute)
		{
			return;
		}
	}

	queue_item item{};
	item.source = absolute;
	item.base_status = tr("Ready");

	if (info.isDir())
	{
		const QDir dir(absolute);
		if (!QFileInfo::exists(dir.filePath(QStringLiteral("PS3_DISC.SFB"))) || !QFileInfo::exists(dir.filePath(QStringLiteral("PS3_GAME/PARAM.SFO"))))
		{
			QMessageBox::warning(this, tr("Invalid JB Folder"), tr("The selected directory is not a PS3 disc JB folder.\n\nExpected PS3_DISC.SFB and PS3_GAME/PARAM.SFO:\n%1").arg(absolute));
			return;
		}

		item.format = tr("JB folder");
		item.input_size = fs::get_dir_size(absolute.toStdString());
		if (item.input_size == umax)
		{
			QMessageBox::warning(this, tr("Unreadable JB Folder"), tr("RPCS3 could not read the complete JB folder:\n%1").arg(absolute));
			return;
		}
		item.key_text = tr("Not required");
		item.compressible = true;
	}
	else if (info.suffix().compare(QStringLiteral("zar"), Qt::CaseInsensitive) == 0)
	{
		std::string error;
		const std::shared_ptr<zar_disc_container> container = zar_disc_container::open(absolute.toStdString(), &error);
		if (!container)
		{
			QMessageBox::warning(this, tr("Invalid ZArchive"), tr("The selected ZArchive is invalid or is not a PS3 disc game:\n%1\n\n%2").arg(absolute, QString::fromStdString(error)));
			return;
		}

		item.format = container->is_iso_layout() ? tr("ZArchive (ISO)") : tr("ZArchive (JB)");
		item.input_size = static_cast<u64>(info.size());
		item.base_status = tr("Already compressed");
		item.compressible = false;

		if (!container->is_iso_layout())
		{
			item.key_text = tr("Not required");
		}
		else if (!container->key_name().empty())
		{
			item.key_text = tr("Embedded: %1").arg(QString::fromStdString(container->key_name()));
		}
		else
		{
			std::string key_path;
			const iso_type_status key_status = iso_file_decryption::find_key(absolute.toStdString(), &key_path);
			if (key_status == iso_type_status::REDUMP_ISO)
			{
				item.key_text = tr("External: %1").arg(QFileInfo(QString::fromStdString(key_path)).fileName());
			}
			else if (key_status == iso_type_status::ERROR_PROCESSING_KEY)
			{
				item.key_text = tr("Invalid external: %1").arg(QFileInfo(QString::fromStdString(key_path)).fileName());
			}
			else
			{
				item.key_text = tr("Not found");
			}
		}
	}
	else if (info.suffix().compare(QStringLiteral("iso"), Qt::CaseInsensitive) == 0)
	{
		u64 size = 0;
		if (!is_iso_file(absolute.toStdString(), &size))
		{
			QMessageBox::warning(this, tr("Invalid ISO"), tr("The selected file is not a valid PS3 ISO image:\n%1").arg(absolute));
			return;
		}

		iso_archive archive(absolute.toStdString());
		if (!archive.is_valid() || !archive.is_file("PS3_DISC.SFB") || !archive.is_file("PS3_GAME/PARAM.SFO"))
		{
			QMessageBox::warning(this, tr("Invalid PS3 Disc Image"), tr("The selected ISO is not a valid PS3 disc game (missing PS3_DISC.SFB or PS3_GAME/PARAM.SFO):\n%1").arg(absolute));
			return;
		}

		item.format = format_description(archive);
		item.input_size = size;
		item.compressible = true;

		std::string key_path;
		const iso_type_status key_status = iso_file_decryption::find_key(absolute.toStdString(), &key_path);
		switch (key_status)
		{
		case iso_type_status::REDUMP_ISO:
			item.key_text = tr("Found: %1").arg(QFileInfo(QString::fromStdString(key_path)).fileName());
			break;
		case iso_type_status::ERROR_PROCESSING_KEY:
			item.key_text = tr("Invalid: %1").arg(QFileInfo(QString::fromStdString(key_path)).fileName());
			item.base_status = tr("Invalid key");
			item.compressible = false;
			break;
		default:
			item.key_text = external_iso_key_not_required(archive) ? tr("Not required") : tr("Not found (optional)");
			break;
		}
	}
	else
	{
		QMessageBox::warning(this, tr("Unsupported Game Format"), tr("Disc Compression Manager supports PS3 JB folders and ISO images. Existing ZArchive files can be added for detection, but are not recompressed."));
		return;
	}

	m_items.emplace_back(std::move(item));
}

void disc_compression_manager_dialog::remove_selected()
{
	QSet<int> rows;
	for (const QModelIndex& index : m_table->selectionModel()->selectedRows())
	{
		rows.insert(index.row());
	}

	for (int row = static_cast<int>(m_items.size()) - 1; row >= 0; row--)
	{
		if (rows.contains(row))
		{
			m_items.erase(m_items.begin() + row);
		}
	}

	update_output_paths();
	refresh_table();
	refresh_buttons();
}

void disc_compression_manager_dialog::clear_queue()
{
	m_items.clear();
	m_progress->setValue(0);
	m_current_label->setText(tr("Ready"));
	refresh_table();
	refresh_buttons();
}

void disc_compression_manager_dialog::choose_output_directory()
{
	QString initial = m_output_directory->text();
	if (initial.isEmpty() && !m_items.empty())
	{
		initial = QFileInfo(m_items.front().source).absolutePath();
	}
	if (initial.isEmpty())
	{
		initial = QDir::homePath();
	}
	const QString directory = QFileDialog::getExistingDirectory(this, tr("Select ZArchive Output Directory"), initial, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (directory.isEmpty())
	{
		return;
	}

	m_output_directory->setText(QDir::cleanPath(directory));
	update_output_paths();
	refresh_table();
	refresh_buttons();
}

QString disc_compression_manager_dialog::output_path_for(const queue_item& item) const
{
	const QFileInfo info(item.source);
	const QString stem = info.isDir() ? info.fileName() : info.completeBaseName();
	const QString parent = m_output_directory->text().trimmed().isEmpty() ? info.absolutePath() : QDir::cleanPath(m_output_directory->text().trimmed());
	return QDir(parent).filePath(stem + QStringLiteral(".zar"));
}

void disc_compression_manager_dialog::update_output_paths()
{
	QSet<QString> outputs;

	for (queue_item& item : m_items)
	{
		item.runnable = false;
		if (!item.compressible)
		{
			item.output.clear();
			continue;
		}

		item.output = QDir::cleanPath(output_path_for(item));
		const QString normalized = item.output.toLower();

		if (outputs.contains(normalized))
		{
			item.base_status = tr("Duplicate output path");
			continue;
		}
		outputs.insert(normalized);

		const QFileInfo output_info(item.output);
		const QFileInfo output_parent(output_info.absolutePath());
		if (!output_parent.exists() || !output_parent.isDir())
		{
			item.base_status = tr("Output directory does not exist");
			continue;
		}

		if (output_info.exists())
		{
			item.base_status = tr("Output already exists");
			continue;
		}

		item.base_status = tr("Ready");
		item.runnable = true;
	}
}

void disc_compression_manager_dialog::refresh_table()
{
	m_table->setRowCount(static_cast<int>(m_items.size()));

	for (int row = 0; row < static_cast<int>(m_items.size()); row++)
	{
		const queue_item& item = m_items[row];
		const QStringList values = {
			source_name(item.source),
			item.format,
			gui::utils::format_byte_size(static_cast<usz>(item.input_size)),
			item.key_text,
			item.output,
			item.base_status,
		};

		for (int col = 0; col < column::count; col++)
		{
			auto* table_item = m_table->item(row, col);
			if (!table_item)
			{
				table_item = new QTableWidgetItem();
				m_table->setItem(row, col, table_item);
			}
			table_item->setText(values[col]);
		}
		m_table->item(row, column::game)->setToolTip(item.source);
		m_table->item(row, column::output)->setToolTip(item.output);
	}
}

void disc_compression_manager_dialog::refresh_buttons()
{
	bool has_runnable = false;
	for (const queue_item& item : m_items)
	{
		has_runnable |= item.runnable;
	}

	const bool has_selection = m_table && !m_table->selectionModel()->selectedRows().isEmpty();
	m_add_button->setEnabled(!m_running);
	m_remove_button->setEnabled(!m_running && has_selection);
	m_clear_button->setEnabled(!m_running && !m_items.empty());
	m_compress_button->setEnabled(!m_running && has_runnable);
	m_cancel_button->setEnabled(m_running && !m_cancel.load());
	m_close_button->setEnabled(!m_running);
	m_output_directory->setEnabled(!m_running);
}

void disc_compression_manager_dialog::set_running(bool running)
{
	m_running = running;
	refresh_buttons();
}

void disc_compression_manager_dialog::start_compression()
{
	struct task
	{
		int row = -1;
		QString source;
		QString output;
		u64 input_size = 0;
	};

	std::vector<task> tasks;
	for (int row = 0; row < static_cast<int>(m_items.size()); row++)
	{
		if (m_items[row].runnable)
		{
			tasks.push_back({row, m_items[row].source, m_items[row].output, m_items[row].input_size});
		}
	}

	if (tasks.empty())
	{
		QMessageBox::information(this, tr("Disc Compression Manager"), tr("There are no games ready to compress."));
		return;
	}

	m_cancel = false;
	m_progress->setValue(0);
	set_running(true);

	m_thread = QThread::create([this, tasks = std::move(tasks)]()
	{
		thread_base::set_name("DiscCompressor");
		const double task_count = static_cast<double>(tasks.size());

		for (usz task_index = 0; task_index < tasks.size(); task_index++)
		{
			if (m_cancel.load())
			{
				break;
			}

			const task current = tasks[task_index];
			QMetaObject::invokeMethod(this, [this, current]()
			{
				m_items[current.row].base_status = tr("Compressing...");
				m_current_label->setText(tr("Compressing %1").arg(source_name(current.source)));
				refresh_table();
			}, Qt::QueuedConnection);

			const zar_compression_result result = compress_ps3_disc_to_zar(current.source.toStdString(), current.output.toStdString(),
				[this, task_index, task_count, name = source_name(current.source)](u64 processed, u64 total, const std::string& current_file)
				{
					if (m_cancel.load())
					{
						return false;
					}

					const double file_fraction = total ? static_cast<double>(processed) / static_cast<double>(total) : 0.0;
					const int progress_value = static_cast<int>(((static_cast<double>(task_index) + file_fraction) / task_count) * 1000.0);
					const QString current_entry = QString::fromStdString(current_file);
					QMetaObject::invokeMethod(this, [this, progress_value, name, current_entry]()
					{
						m_progress->setValue(std::clamp(progress_value, 0, 1000));
						m_current_label->setText(current_entry.isEmpty() ? tr("Compressing %1").arg(name) : tr("Compressing %1 - %2").arg(name, current_entry));
					}, Qt::QueuedConnection);
					return true;
				});

			QMetaObject::invokeMethod(this, [this, current, result]()
			{
				queue_item& item = m_items[current.row];
				if (result)
				{
					const double ratio = result.input_size ? (static_cast<double>(result.output_size) * 100.0 / static_cast<double>(result.input_size)) : 0.0;
					item.base_status = tr("Done - %1 (%2%)").arg(gui::utils::format_byte_size(static_cast<usz>(result.output_size))).arg(ratio, 0, 'f', 1);
					item.runnable = false;
					zar_gui_log.success("Compressed '%s' to '%s' (%llu -> %llu bytes)", current.source.toStdString(), current.output.toStdString(), result.input_size, result.output_size);
				}
				else if (result.status == zar_compression_status::cancelled)
				{
					item.base_status = tr("Cancelled");
				}
				else
				{
					item.base_status = tr("Error: %1").arg(QString::fromStdString(result.error));
					zar_gui_log.error("Failed to compress '%s': %s", current.source.toStdString(), result.error);
				}
				refresh_table();
			}, Qt::QueuedConnection);

			if (result.status == zar_compression_status::cancelled)
			{
				break;
			}
		}
	});

	connect(m_thread, &QThread::finished, this, [this]()
	{
		const bool cancelled = m_cancel.load();
		m_thread->deleteLater();
		m_thread = nullptr;
		set_running(false);
		if (!cancelled)
		{
			m_progress->setValue(1000);
			m_current_label->setText(tr("Compression queue finished"));
		}
		else
		{
			m_current_label->setText(tr("Compression cancelled"));
		}
		refresh_table();
		refresh_buttons();
	});

	m_thread->start();
}
