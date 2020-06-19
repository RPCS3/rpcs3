#include <QPushButton>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QCheckBox>
#include <QMessageBox>

#include "ui_patch_manager_dialog.h"
#include "patch_manager_dialog.h"
#include "table_item_delegate.h"
#include "qt_utils.h"
#include "Utilities/File.h"
#include "util/logs.hpp"

LOG_CHANNEL(patch_log);

enum patch_column : int
{
	enabled,
	title,
	serials,
	description,
	patch_version,
	author,
	notes
};

enum patch_role : int
{
	hash_role = Qt::UserRole,
	description_role,
	persistance_role
};

patch_manager_dialog::patch_manager_dialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::patch_manager_dialog)
{
	ui->setupUi(this);
	setModal(true);

	// Load config for special settings
	patch_engine::load_config(m_legacy_patches_enabled);
	ui->cb_enable_legacy_patches->setChecked(m_legacy_patches_enabled);

	connect(ui->patch_filter, &QLineEdit::textChanged, this, &patch_manager_dialog::filter_patches);
	connect(ui->patch_tree, &QTreeWidget::currentItemChanged, this, &patch_manager_dialog::on_item_selected);
	connect(ui->patch_tree, &QTreeWidget::itemChanged, this, &patch_manager_dialog::on_item_changed);
	connect(ui->patch_tree, &QTreeWidget::customContextMenuRequested, this, &patch_manager_dialog::on_custom_context_menu_requested);
	connect(ui->pb_expand_all, &QAbstractButton::clicked, ui->patch_tree, &QTreeWidget::expandAll);
	connect(ui->pb_collapse_all, &QAbstractButton::clicked, ui->patch_tree, &QTreeWidget::collapseAll);
	connect(ui->cb_enable_legacy_patches, &QCheckBox::stateChanged, this, &patch_manager_dialog::on_legacy_patches_enabled);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);
	connect(ui->buttonBox, &QDialogButtonBox::clicked, [this](QAbstractButton* button)
	{
		if (button == ui->buttonBox->button(QDialogButtonBox::Save))
		{
			save_config();
			accept();
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
		{
			save_config();
		}
	});

	refresh();

	resize(QGuiApplication::primaryScreen()->availableSize() * 0.7);
}

patch_manager_dialog::~patch_manager_dialog()
{
	delete ui;
}

void patch_manager_dialog::refresh()
{
	load_patches();
	populate_tree();
}

void patch_manager_dialog::load_patches()
{
	m_map.clear();

	// Legacy path (in case someone puts it there)
	patch_engine::load(m_map, fs::get_config_dir() + "patch.yml");

	// New paths
	const std::string patches_path = fs::get_config_dir() + "patches/";
	const QStringList filters      = QStringList() << "*patch.yml";
	const QStringList path_list    = QDir(QString::fromStdString(patches_path)).entryList(filters);

	for (const auto& path : path_list)
	{
		patch_engine::load(m_map, patches_path + path.toStdString());
	}
}

static QList<QTreeWidgetItem*> find_children_by_data(QTreeWidgetItem* parent, const QList<QPair<int /*role*/, const QVariant& /*data*/>>& criteria)
{
	QList<QTreeWidgetItem*> list;

	if (parent)
	{
		for (int i = 0; i < parent->childCount(); i++)
		{
			if (auto item = parent->child(i))
			{
				bool match = true;

				for (const auto [role, data] : criteria)
				{
					if (item->data(0, role) != data)
					{
						match = false;
						break;
					}
				}

				if (match)
				{
					list << item;
				}
			}
		}
	}

	return list;
}

void patch_manager_dialog::populate_tree()
{
	// "Reset" currently used items. Items that aren't persisted will be removed later.
	// Using this logic instead of clearing the tree here should persist the expanded status of items.
	for (auto item : ui->patch_tree->findItems(".*", Qt::MatchFlag::MatchRegExp | Qt::MatchFlag::MatchRecursive))
	{
		if (item)
		{
			item->setData(0, persistance_role, false);
		}
	}

	for (const auto& [hash, container] : m_map)
	{
		// Don't show legacy patches, because you can't configure them anyway
		if (container.is_legacy)
		{
			continue;
		}

		const QString q_hash = QString::fromStdString(hash);

		// Add patch items
		for (const auto& [description, patch] : container.patch_info_map)
		{
			if (patch.is_legacy)
			{
				continue;
			}

			const QString q_title   = patch.title.empty() ? tr("Unknown Title") : QString::fromStdString(patch.title);
			const QString q_serials = patch.serials.empty() ? tr("Unknown Version") : QString::fromStdString(patch.serials);

			QTreeWidgetItem* title_level_item = nullptr;

			// Find top level item for this title
			if (const auto list = ui->patch_tree->findItems(q_title, Qt::MatchFlag::MatchExactly, 0); list.size() > 0)
			{
				title_level_item = list[0];
			}

			// Add a top level item for this title if it doesn't exist yet
			if (!title_level_item)
			{
				title_level_item = new QTreeWidgetItem();
				title_level_item->setText(0, q_title);
				title_level_item->setData(0, hash_role, q_hash);

				ui->patch_tree->addTopLevelItem(title_level_item);
			}
			assert(title_level_item);

			// Find out if there is a node item for this serial
			QTreeWidgetItem* serial_level_item = gui::utils::find_child(title_level_item, q_serials);

			// Add a node item for this serial if it doesn't exist yet
			if (!serial_level_item)
			{
				serial_level_item = new QTreeWidgetItem();
				serial_level_item->setText(0, q_serials);
				serial_level_item->setData(0, hash_role, q_hash);

				title_level_item->addChild(serial_level_item);
			}
			assert(serial_level_item);

			// Add a checkable leaf item for this patch
			const QString q_description = QString::fromStdString(description);
			QString visible_description = q_description;

			const auto match_criteria = QList<QPair<int, const QVariant&>>() << QPair(description_role, q_description) << QPair(persistance_role, true);

			// Add counter to leafs if the name already exists due to different hashes of the same game (PPU, SPU, PRX, OVL)
			if (const auto matches = find_children_by_data(serial_level_item, match_criteria); matches.count() > 0)
			{
				if (auto only_match = matches.count() == 1 ? matches[0] : nullptr)
				{
					only_match->setText(0, q_description + QStringLiteral(" (1)"));
				}
				visible_description += QStringLiteral(" (") + QString::number(matches.count() + 1) + QStringLiteral(")");
			}

			QTreeWidgetItem* patch_level_item = new QTreeWidgetItem();
			patch_level_item->setText(0, visible_description);
			patch_level_item->setCheckState(0, patch.enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
			patch_level_item->setData(0, hash_role, q_hash);
			patch_level_item->setData(0, description_role, q_description);

			serial_level_item->addChild(patch_level_item);

			// Persist items
			title_level_item->setData(0, persistance_role, true);
			serial_level_item->setData(0, persistance_role, true);
			patch_level_item->setData(0, persistance_role, true);
		}
	}

	ui->patch_tree->sortByColumn(0, Qt::SortOrder::AscendingOrder);

	for (int i = ui->patch_tree->topLevelItemCount() - 1; i >= 0; i--)
	{
		if (auto title_level_item = ui->patch_tree->topLevelItem(i))
		{
			if (!title_level_item->data(0, persistance_role).toBool())
			{
				delete ui->patch_tree->takeTopLevelItem(i);
				continue;
			}

			title_level_item->sortChildren(0, Qt::SortOrder::AscendingOrder);

			for (int j = title_level_item->childCount() - 1; j >= 0; j--)
			{
				if (auto serial_level_item = title_level_item->child(j))
				{
					if (!serial_level_item->data(0, persistance_role).toBool())
					{
						delete title_level_item->takeChild(j);
						continue;
					}

					for (int k = serial_level_item->childCount() - 1; k >= 0; k--)
					{
						if (auto leaf_item = serial_level_item->child(k))
						{
							if (!leaf_item->data(0, persistance_role).toBool())
							{
								delete serial_level_item->takeChild(k);
							}
						}
					}

					serial_level_item->sortChildren(0, Qt::SortOrder::AscendingOrder);
				}
			}
		}
	}
}

void patch_manager_dialog::save_config()
{
	patch_engine::save_config(m_map, m_legacy_patches_enabled);
}

void patch_manager_dialog::filter_patches(const QString& term)
{
	// Recursive function to show all matching items and their children.
	// @return number of visible children of item, including item
	std::function<int(QTreeWidgetItem*, bool)> show_matches;
	show_matches = [&show_matches, search_text = term.toLower()](QTreeWidgetItem* item, bool parent_visible) -> int
	{
		if (!item) return 0;

		// Only try to match if the parent is not visible
		parent_visible = parent_visible || item->text(0).toLower().contains(search_text);
		int visible_items = parent_visible ? 1 : 0;

		// Get the number of visible children recursively
		for (int i = 0; i < item->childCount(); i++)
		{
			visible_items += show_matches(item->child(i), parent_visible);
		}

		// Only show item if itself or any of its children is visible
		item->setHidden(visible_items <= 0);
		return visible_items;
	};

	// Go through each top level item and try to find matches
	for (auto top_level_item : ui->patch_tree->findItems(".*", Qt::MatchRegExp))
	{
		show_matches(top_level_item, false);
	}
}

void patch_manager_dialog::update_patch_info(const patch_engine::patch_info& info)
{
	ui->label_hash->setText(QString::fromStdString(info.hash));
	ui->label_author->setText(QString::fromStdString(info.author));
	ui->label_notes->setText(QString::fromStdString(info.notes));
	ui->label_description->setText(QString::fromStdString(info.description));
	ui->label_patch_version->setText(QString::fromStdString(info.patch_version));
	ui->label_serials->setText(QString::fromStdString(info.serials));
	ui->label_title->setText(QString::fromStdString(info.title));
}

void patch_manager_dialog::on_item_selected(QTreeWidgetItem *current, QTreeWidgetItem * /*previous*/)
{
	if (!current)
	{
		return;
	}

	// Get patch identifiers stored in item data
	const std::string hash = current->data(0, hash_role).toString().toStdString();
	const std::string description = current->data(0, description_role).toString().toStdString();

	if (m_map.find(hash) != m_map.end())
	{
		const auto& container = m_map.at(hash);

		// Find the patch for this item and show its metadata
		if (!container.is_legacy && container.patch_info_map.find(description) != container.patch_info_map.end())
		{
			update_patch_info(container.patch_info_map.at(description));
			return;
		}

		// Show shared info if no patch was found
		patch_engine::patch_info info{};
		info.hash    = hash;
		info.version = container.version;
		update_patch_info(info);
		return;
	}

	// Clear patch info if no info was found
	patch_engine::patch_info info{};
	update_patch_info(info);
}

void patch_manager_dialog::on_item_changed(QTreeWidgetItem *item, int /*column*/)
{
	if (!item)
	{
		return;
	}

	// Get checkstate of the item
	const bool enabled = item->checkState(0) == Qt::CheckState::Checked;

	// Get patch identifiers stored in item data
	const std::string hash = item->data(0, hash_role).toString().toStdString();
	const std::string description = item->data(0, description_role).toString().toStdString();

	// Enable/disable the patch for this item and show its metadata
	if (m_map.find(hash) != m_map.end())
	{
		auto& container = m_map[hash];

		if (!container.is_legacy && container.patch_info_map.find(description) != container.patch_info_map.end())
		{
			auto& patch = m_map[hash].patch_info_map[description];
			patch.enabled = enabled;
			update_patch_info(patch);
			return;
		}
	}
}

void patch_manager_dialog::on_custom_context_menu_requested(const QPoint &pos)
{
	QTreeWidgetItem* item = ui->patch_tree->itemAt(pos);

	if (!item)
	{
		return;
	}

	QMenu* menu = new QMenu(this);

	if (item->childCount() > 0)
	{
		QAction* collapse_children = new QAction("Collapse Children");
		menu->addAction(collapse_children);
		connect(collapse_children, &QAction::triggered, this, [&item](bool)
		{
			for (int i = 0; i < item->childCount(); i++)
			{
				item->child(i)->setExpanded(false);
			}
		});

		QAction* expand_children = new QAction("Expand Children");
		menu->addAction(expand_children);
		connect(expand_children, &QAction::triggered, this, [&item](bool)
		{
			for (int i = 0; i < item->childCount(); i++)
			{
				item->child(i)->setExpanded(true);
			}
		});
	}
	else
	{
		// Find the patch for this item and add menu items accordingly
		const std::string hash = item->data(0, hash_role).toString().toStdString();
		const std::string description = item->data(0, description_role).toString().toStdString();

		if (m_map.find(hash) != m_map.end())
		{
			const auto& container = m_map.at(hash);

			if (!container.is_legacy && container.patch_info_map.find(description) != container.patch_info_map.end())
			{
				const auto info = container.patch_info_map.at(description);

				QAction* open_filepath = new QAction("Show Patch File");
				menu->addAction(open_filepath);
				connect(open_filepath, &QAction::triggered, this, [info](bool)
				{
					gui::utils::open_dir(info.source_path);
				});
			}
		}
	}

	menu->exec(ui->patch_tree->viewport()->mapToGlobal(pos));
}

bool patch_manager_dialog::is_valid_file(const QMimeData& md, QStringList* drop_paths)
{
	const QList<QUrl> list = md.urls(); // Get list of all the dropped file urls

	if (list.size() != 1) // We only accept one file for now
	{
		return false;
	}

	for (auto&& url : list) // Check each file in url list for valid type
	{
		const QString path   = url.toLocalFile(); // Convert url to filepath
		const QFileInfo info = path;

		if (!info.fileName().endsWith("patch.yml"))
		{
			return false;
		}

		if (drop_paths)
		{
			drop_paths->append(path);
		}
	}

	return true;
}

void patch_manager_dialog::dropEvent(QDropEvent* event)
{
	QStringList drop_paths;

	if (!is_valid_file(*event->mimeData(), &drop_paths))
	{
		return;
	}

	QMessageBox box(QMessageBox::Icon::Question, tr("Patch Manager"), tr("What do you want to do with the patch file?"), QMessageBox::StandardButton::NoButton, this);
	QAbstractButton* button_yes = box.addButton(tr("Import"), QMessageBox::YesRole);
	QAbstractButton* button_no = box.addButton(tr("Validate"), QMessageBox::NoRole);
	box.exec();

	const bool do_import   = box.clickedButton() == button_yes;
	const bool do_validate = do_import || box.clickedButton() == button_no;

	if (!do_validate)
	{
		return;
	}

	for (const auto drop_path : drop_paths)
	{
		const auto path = drop_path.toStdString();
		patch_engine::patch_map patches;
		std::stringstream log_message;

		if (patch_engine::load(patches, path, true, &log_message))
		{
			patch_log.success("Successfully validated patch file %s", path);

			if (do_import)
			{
				static const std::string imported_patch_yml_path = fs::get_config_dir() + "patches/imported_patch.yml";

				log_message.clear();

				if (patch_engine::import_patches(patches, imported_patch_yml_path, &log_message))
				{
					refresh();
					QMessageBox::information(this, tr("Import successful"), tr("The patch file was imported to:\n%0").arg(QString::fromStdString(imported_patch_yml_path)));
				}
				else
				{
					QMessageBox::information(this, tr("Import failed"), tr("The patch file was not imported.\nPlease see the log for more information."));
				}
			}
			else
			{
				QMessageBox::information(this, tr("Validation successful"), tr("The patch file passed the validation."));
			}
		}
		else
		{
			patch_log.error("Errors found in patch file %s", path);
			QMessageBox::warning(this, tr("Validation failed"), tr("Errors were found in the patch file. Log:\n\n") + QString::fromStdString(log_message.str()));
		}
	}
}

void patch_manager_dialog::on_legacy_patches_enabled(int state)
{
	m_legacy_patches_enabled = state == Qt::CheckState::Checked;
}

void patch_manager_dialog::dragEnterEvent(QDragEnterEvent* event)
{
	if (is_valid_file(*event->mimeData()))
	{
		event->accept();
	}
}

void patch_manager_dialog::dragMoveEvent(QDragMoveEvent* event)
{
	if (is_valid_file(*event->mimeData()))
	{
		event->accept();
	}
}

void patch_manager_dialog::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}
