﻿#include <QPushButton>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QCheckBox>
#include <QMessageBox>
#include <QTimer>

#include "ui_patch_manager_dialog.h"
#include "patch_manager_dialog.h"
#include "table_item_delegate.h"
#include "gui_settings.h"
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
	title_role,
	serial_role,
	app_version_role,
	description_role,
	patch_group_role,
	persistance_role,
	node_level_role
};

enum node_level : int
{
	title_level,
	serial_level,
	patch_level
};

patch_manager_dialog::patch_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::unordered_map<std::string, std::set<std::string>> games, QWidget* parent)
	: QDialog(parent)
	, m_gui_settings(gui_settings)
	, m_owned_games(std::move(games))
	, ui(new Ui::patch_manager_dialog)
{
	ui->setupUi(this);
	setModal(true);

	// Load config for special settings
	patch_engine::load_config(m_legacy_patches_enabled);

	// Load gui settings
	m_show_owned_games_only = m_gui_settings->GetValue(gui::pm_show_owned).toBool();

	// Initialize gui controls
	ui->cb_enable_legacy_patches->setChecked(m_legacy_patches_enabled);
	ui->cb_owned_games_only->setChecked(m_show_owned_games_only);

	// Create connects
	connect(ui->patch_filter, &QLineEdit::textChanged, this, &patch_manager_dialog::filter_patches);
	connect(ui->patch_tree, &QTreeWidget::currentItemChanged, this, &patch_manager_dialog::on_item_selected);
	connect(ui->patch_tree, &QTreeWidget::itemChanged, this, &patch_manager_dialog::on_item_changed);
	connect(ui->patch_tree, &QTreeWidget::customContextMenuRequested, this, &patch_manager_dialog::on_custom_context_menu_requested);
	connect(ui->cb_enable_legacy_patches, &QCheckBox::stateChanged, this, &patch_manager_dialog::on_legacy_patches_enabled);
	connect(ui->cb_owned_games_only, &QCheckBox::stateChanged, this, &patch_manager_dialog::on_show_owned_games_only);
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
}

patch_manager_dialog::~patch_manager_dialog()
{
	// Save gui settings
	m_gui_settings->SetValue(gui::pm_geometry, saveGeometry());
	m_gui_settings->SetValue(gui::pm_splitter_state, ui->splitter->saveState());

	delete ui;
}

int patch_manager_dialog::exec()
{
	show();
	refresh(true);
	return QDialog::exec();
}

void patch_manager_dialog::refresh(bool restore_layout)
{
	load_patches(restore_layout);
	populate_tree();
	filter_patches(ui->patch_filter->text());

	if (restore_layout)
	{
		if (!restoreGeometry(m_gui_settings->GetValue(gui::pm_geometry).toByteArray()))
		{
			resize(QGuiApplication::primaryScreen()->availableSize() * 0.7);
		}

		if (!ui->splitter->restoreState(m_gui_settings->GetValue(gui::pm_splitter_state).toByteArray()))
		{
			const int width_left = ui->splitter->width() * 0.7;
			const int width_right = ui->splitter->width() - width_left;
			ui->splitter->setSizes({ width_left, width_right });
		}
	}
}

void patch_manager_dialog::load_patches(bool show_error)
{
	m_map.clear();

	// NOTE: Make sure these paths are loaded in the same order as they are applied on boot

	const std::string patches_path = patch_engine::get_patches_path();
	const QStringList filters      = QStringList() << "*_patch.yml";

	QStringList path_list;
	path_list << "patch.yml";
	path_list << "imported_patch.yml";
	path_list << QDir(QString::fromStdString(patches_path)).entryList(filters);
	path_list.removeDuplicates(); // make sure to load patch.yml and imported_patch.yml only once

	bool has_errors = false;

	for (const auto& path : path_list)
	{
		if (!patch_engine::load(m_map, patches_path + path.toStdString()))
		{
			has_errors = true;
		}
	}

	if (show_error && has_errors)
	{
		// Open a warning dialog after the patch manager was opened
		QTimer::singleShot(100, [this, patches_path]()
		{
			QMessageBox::warning(this, tr("Incompatible patches detected"),
				tr("Some of your patches are not compatible with the current version of RPCS3's Patch Manager.\n\nMake sure that all the patches located in \"%0\" contain the proper formatting that is required for the Patch Manager Version %1.")
				.arg(QString::fromStdString(patches_path)).arg(QString::fromStdString(patch_engine_version)));
		});
	}
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

			const QString q_patch_group = QString::fromStdString(patch.patch_group);

			for (const auto& [title, serials] : patch.titles)
			{
				if (serials.empty())
				{
					continue;
				}

				const QString q_title = QString::fromStdString(title);
				const QString visible_title = title == patch_key::all ? tr_all_titles : q_title;

				QTreeWidgetItem* title_level_item = nullptr;

				// Find top level item for this title
				if (const auto list = ui->patch_tree->findItems(visible_title, Qt::MatchFlag::MatchExactly, 0); list.size() > 0)
				{
					title_level_item = list[0];
				}

				// Add a top level item for this title if it doesn't exist yet
				if (!title_level_item)
				{
					title_level_item = new QTreeWidgetItem();
					title_level_item->setText(0, visible_title);
					title_level_item->setData(0, title_role, q_title);
					title_level_item->setData(0, node_level_role, node_level::title_level);

					ui->patch_tree->addTopLevelItem(title_level_item);
				}
				ASSERT(title_level_item);

				title_level_item->setData(0, persistance_role, true);

				for (const auto& [serial, app_versions] : serials)
				{
					if (app_versions.empty())
					{
						continue;
					}

					const QString q_serial = QString::fromStdString(serial);
					const QString visible_serial = serial == patch_key::all ? tr_all_serials : q_serial;

					for (const auto& [app_version, enabled] : app_versions)
					{
						const QString q_app_version = QString::fromStdString(app_version);
						const QString q_version_suffix = app_version == patch_key::all ? (QStringLiteral(" - ") + tr_all_versions) : (QStringLiteral(" v.") + q_app_version);
						const QString q_serial_and_version = visible_serial + q_version_suffix;

						// Find out if there is a node item for this serial
						QTreeWidgetItem* serial_level_item = gui::utils::find_child(title_level_item, q_serial_and_version);

						// Add a node item for this serial if it doesn't exist yet
						if (!serial_level_item)
						{
							serial_level_item = new QTreeWidgetItem();
							serial_level_item->setText(0, q_serial_and_version);
							serial_level_item->setData(0, title_role, q_title);
							serial_level_item->setData(0, serial_role, q_serial);
							serial_level_item->setData(0, app_version_role, q_app_version);
							serial_level_item->setData(0, node_level_role, node_level::serial_level);

							title_level_item->addChild(serial_level_item);
						}
						ASSERT(serial_level_item);

						serial_level_item->setData(0, persistance_role, true);

						// Add a checkable leaf item for this patch
						const QString q_description = QString::fromStdString(description);
						QString visible_description = q_description;

						const auto match_criteria = QList<QPair<int, QVariant>>() << QPair(description_role, q_description) << QPair(persistance_role, true);

						// Add counter to leafs if the name already exists due to different hashes of the same game (PPU, SPU, PRX, OVL)
						if (const auto matches = gui::utils::find_children_by_data(serial_level_item, match_criteria, false); matches.count() > 0)
						{
							if (auto only_match = matches.count() == 1 ? matches[0] : nullptr)
							{
								only_match->setText(0, q_description + QStringLiteral(" (01)"));
							}
							const int counter = matches.count() + 1;
							visible_description += QStringLiteral(" (");
							if (counter < 10) visible_description += '0';
							visible_description += QString::number(counter) + ')';
						}

						QTreeWidgetItem* patch_level_item = new QTreeWidgetItem();
						patch_level_item->setText(0, visible_description);
						patch_level_item->setCheckState(0, enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
						patch_level_item->setData(0, hash_role, q_hash);
						patch_level_item->setData(0, title_role, q_title);
						patch_level_item->setData(0, serial_role, q_serial);
						patch_level_item->setData(0, app_version_role, q_app_version);
						patch_level_item->setData(0, description_role, q_description);
						patch_level_item->setData(0, patch_group_role, q_patch_group);
						patch_level_item->setData(0, node_level_role, node_level::patch_level);
						patch_level_item->setData(0, persistance_role, true);

						serial_level_item->addChild(patch_level_item);
					}
				}
			}
		}
	}

	const auto match_criteria = QList<QPair<int, QVariant>>() << QPair(persistance_role, true);

	for (int i = ui->patch_tree->topLevelItemCount() - 1; i >= 0; i--)
	{
		if (auto title_level_item = ui->patch_tree->topLevelItem(i))
		{
			if (!title_level_item->data(0, persistance_role).toBool())
			{
				delete ui->patch_tree->takeTopLevelItem(i);
				continue;
			}

			gui::utils::remove_children(title_level_item, match_criteria, true);
		}
	}

	gui::utils::sort_tree(ui->patch_tree, Qt::SortOrder::AscendingOrder, true);

	// Move "All titles" to the top
	// NOTE: "All serials" is only allowed as the only node in "All titles", so there is no need to move it up
	// NOTE: "All versions" will be above valid numerical versions through sorting anyway
	if (const auto all_title_items = ui->patch_tree->findItems(tr_all_titles, Qt::MatchExactly); all_title_items.size() > 0)
	{
		const auto item = all_title_items[0];
		ASSERT(item && all_title_items.size() == 1);

		if (const int index = ui->patch_tree->indexOfTopLevelItem(item); index >= 0)
		{
			const bool all_titles_expanded = item->isExpanded();
			const auto all_serials_item = item->child(0);
			const bool all_serials_expanded = all_serials_item && all_serials_item->isExpanded();

			ui->patch_tree->takeTopLevelItem(index);
			ui->patch_tree->insertTopLevelItem(0, item);

			item->setExpanded(all_titles_expanded);

			if (all_serials_item)
			{
				all_serials_item->setExpanded(all_serials_expanded);
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
	show_matches = [this, &show_matches, search_text = term.toLower()](QTreeWidgetItem* item, bool parent_visible) -> int
	{
		if (!item) return 0;

		const node_level level = static_cast<node_level>(item->data(0, node_level_role).toInt());

		// Hide nodes that aren't in the game list
		if (m_show_owned_games_only)
		{
			if (level == node_level::serial_level)
			{
				const std::string serial = item->data(0, serial_role).toString().toStdString();
				const std::string app_version = item->data(0, app_version_role).toString().toStdString();

				if (serial != patch_key::all &&
					(m_owned_games.find(serial) == m_owned_games.end() || (app_version != patch_key::all && !m_owned_games.at(serial).contains(app_version))))
				{
					item->setHidden(true);
					return 0;
				}
			}
		}

		// Only try to match if the parent is not visible
		parent_visible = parent_visible || item->text(0).toLower().contains(search_text);
		int visible_items = 0;

		// Get the number of visible children recursively
		for (int i = 0; i < item->childCount(); i++)
		{
			visible_items += show_matches(item->child(i), parent_visible);
		}

		if (parent_visible)
		{
			// Only show the title node if it has visible children when filtering for game list entries
			if (!m_show_owned_games_only || level != node_level::title_level || visible_items > 0)
			{
				visible_items++;
			}
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

void patch_manager_dialog::update_patch_info(const patch_manager_dialog::gui_patch_info& info)
{
	ui->label_hash->setText(info.hash);
	ui->label_author->setText(info.author);
	ui->label_notes->setText(info.notes);
	ui->label_description->setText(info.description);
	ui->label_patch_version->setText(info.patch_version);
	ui->label_serial->setText(info.serial);
	ui->label_title->setText(info.title);
	ui->label_app_version->setText(info.app_version);
}

void patch_manager_dialog::on_item_selected(QTreeWidgetItem *current, QTreeWidgetItem * /*previous*/)
{
	if (!current)
	{
		// Clear patch info if no item is selected
		update_patch_info({});
		return;
	}

	const node_level level = static_cast<node_level>(current->data(0, node_level_role).toInt());

	patch_manager_dialog::gui_patch_info info{};

	switch (level)
	{
	case node_level::patch_level:
	{
		// Get patch identifiers stored in item data
		info.hash = current->data(0, hash_role).toString();
		const std::string hash = info.hash.toStdString();
		const std::string description = current->data(0, description_role).toString().toStdString();

		// Find the patch for this item and get its metadata
		if (m_map.find(hash) != m_map.end())
		{
			const auto& container = m_map.at(hash);

			if (!container.is_legacy && container.patch_info_map.find(description) != container.patch_info_map.end())
			{
				const auto& found_info = container.patch_info_map.at(description);
				info.author = QString::fromStdString(found_info.author);
				info.notes = QString::fromStdString(found_info.notes);
				info.description = QString::fromStdString(found_info.description);
				info.patch_version = QString::fromStdString(found_info.patch_version);
			}
		}
		[[fallthrough]];
	}
	case node_level::serial_level:
	{
		const QString serial = current->data(0, serial_role).toString();
		info.serial = serial.toStdString() == patch_key::all ? tr_all_serials : serial;

		const QString app_version = current->data(0, app_version_role).toString();
		info.app_version = app_version.toStdString() == patch_key::all ? tr_all_versions : app_version;

		[[fallthrough]];
	}
	case node_level::title_level:
	{
		const QString title = current->data(0, title_role).toString();
		info.title = title.toStdString() == patch_key::all ? tr_all_titles : title;

		[[fallthrough]];
	}
	default:
	{
		break;
	}
	}

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
	const node_level level = static_cast<node_level>(item->data(0, node_level_role).toInt());
	const std::string hash = item->data(0, hash_role).toString().toStdString();
	const std::string title = item->data(0, title_role).toString().toStdString();
	const std::string serial = item->data(0, serial_role).toString().toStdString();
	const std::string app_version = item->data(0, app_version_role).toString().toStdString();
	const std::string description = item->data(0, description_role).toString().toStdString();
	const std::string patch_group = item->data(0, patch_group_role).toString().toStdString();

	// Uncheck other patches with the same patch_group if this patch was enabled
	if (const auto node = item->parent(); node && enabled && !patch_group.empty() && level == node_level::patch_level)
	{
		for (int i = 0; i < node->childCount(); i++)
		{
			if (const auto other = node->child(i); other && other != item)
			{
				const std::string other_patch_group = other->data(0, patch_group_role).toString().toStdString();

				if (other_patch_group == patch_group)
				{
					other->setCheckState(0, Qt::CheckState::Unchecked);
				}
			}
		}
	}

	// Enable/disable the patch for this item and show its metadata
	if (m_map.find(hash) != m_map.end())
	{
		auto& container = m_map[hash];

		if (!container.is_legacy && container.patch_info_map.find(description) != container.patch_info_map.end())
		{
			m_map[hash].patch_info_map[description].titles[title][serial][app_version] = enabled;
			on_item_selected(item, nullptr);
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

	const node_level level = static_cast<node_level>(item->data(0, node_level_role).toInt());

	QMenu* menu = new QMenu(this);

	if (level == node_level::patch_level)
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

				QAction* open_filepath = new QAction(tr("Show Patch File"));
				menu->addAction(open_filepath);
				connect(open_filepath, &QAction::triggered, this, [info](bool)
				{
					gui::utils::open_dir(info.source_path);
				});

				menu->addSeparator();

				if (info.source_path == patch_engine::get_imported_patch_path())
				{
					QAction* remove_patch = new QAction(tr("Remove Patch"));
					menu->addAction(remove_patch);
					connect(remove_patch, &QAction::triggered, this, [info, this](bool)
					{
						const auto answer = QMessageBox::question(this, tr("Remove Patch?"),
							tr("Do you really want to remove the selected patch?\nThis action is immediate and irreversible!"));

						if (answer != QMessageBox::StandardButton::Yes)
						{
							return;
						}

						if (patch_engine::remove_patch(info))
						{
							patch_log.success("Successfully removed patch %s: %s from %s", info.hash, info.description, info.source_path);
							refresh(); // Refresh before showing the dialog
							QMessageBox::information(this, tr("Success"), tr("The patch was successfully removed!"));
						}
						else
						{
							patch_log.error("Could not remove patch %s: %s from %s", info.hash, info.description, info.source_path);
							refresh(); // Refresh before showing the dialog
							QMessageBox::critical(this, tr("Failure"), tr("The patch could not be removed!"));
						}
					});

					menu->addSeparator();
				}
			}
		}
	}

	if (item->childCount() > 0)
	{
		if (item->isExpanded())
		{
			QAction* collapse = new QAction(tr("Collapse"));
			menu->addAction(collapse);
			connect(collapse, &QAction::triggered, this, [&item](bool)
			{
				item->setExpanded(false);
			});

			if (level < (node_level::patch_level - 1))
			{
				menu->addSeparator();

				QAction* expand_children = new QAction(tr("Expand Children"));
				menu->addAction(expand_children);
				connect(expand_children, &QAction::triggered, this, [&item](bool)
				{
					for (int i = 0; i < item->childCount(); i++)
					{
						item->child(i)->setExpanded(true);
					}
				});

				QAction* collapse_children = new QAction(tr("Collapse Children"));
				menu->addAction(collapse_children);
				connect(collapse_children, &QAction::triggered, this, [&item](bool)
				{
					for (int i = 0; i < item->childCount(); i++)
					{
						item->child(i)->setExpanded(false);
					}
				});
			}
		}
		else
		{
			QAction* expand = new QAction(tr("Expand"));
			menu->addAction(expand);
			connect(expand, &QAction::triggered, this, [&item](bool)
			{
				item->setExpanded(true);
			});
		}

		menu->addSeparator();
	}

	QAction* expand_all = new QAction(tr("Expand All"));
	menu->addAction(expand_all);
	connect(expand_all, &QAction::triggered, ui->patch_tree, &QTreeWidget::expandAll);

	QAction* collapse_all = new QAction(tr("Collapse All"));
	menu->addAction(collapse_all);
	connect(collapse_all, &QAction::triggered, ui->patch_tree, &QTreeWidget::collapseAll);

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

	QMessageBox box(QMessageBox::Icon::Question, tr("Patch Manager"), tr("What do you want to do with the patch file?"), QMessageBox::StandardButton::Cancel, this);
	QPushButton* button_yes = box.addButton(tr("Import"), QMessageBox::YesRole);
	QPushButton* button_no = box.addButton(tr("Validate"), QMessageBox::NoRole);
	box.setDefaultButton(button_yes);
	box.exec();

	const bool do_import   = box.clickedButton() == button_yes;
	const bool do_validate = do_import || box.clickedButton() == button_no;

	if (!do_validate)
	{
		return;
	}

	for (const QString& drop_path : drop_paths)
	{
		const auto path = drop_path.toStdString();
		patch_engine::patch_map patches;
		std::stringstream log_message;

		if (patch_engine::load(patches, path, true, &log_message))
		{
			patch_log.success("Successfully validated patch file %s", path);

			if (do_import)
			{
				static const std::string imported_patch_yml_path = patch_engine::get_imported_patch_path();

				log_message.clear();

				size_t count = 0;
				size_t total = 0;

				if (patch_engine::import_patches(patches, imported_patch_yml_path, count, total, &log_message))
				{
					refresh();

					const std::string message = log_message.str();
					const QString msg = message.empty() ? "" : tr("\n\nLog:\n%0").arg(QString::fromStdString(message));

					if (count == 0)
					{
						QMessageBox::warning(this, tr("Nothing to import"), tr("None of the found %0 patches were imported.%1")
							.arg(total).arg(msg));
					}
					else
					{
						QMessageBox::information(this, tr("Import successful"), tr("Imported %0/%1 patches to:\n%2%3")
							.arg(count).arg(total).arg(QString::fromStdString(imported_patch_yml_path)).arg(msg));
					}
				}
				else
				{
					QMessageBox::critical(this, tr("Import failed"), tr("The patch file could not be imported.\n\nLog:\n%0").arg(QString::fromStdString(log_message.str())));
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
			QMessageBox::critical(this, tr("Validation failed"), tr("Errors were found in the patch file.\n\nLog:\n%0").arg(QString::fromStdString(log_message.str())));
		}
	}
}

void patch_manager_dialog::on_legacy_patches_enabled(int state)
{
	m_legacy_patches_enabled = state == Qt::CheckState::Checked;
}

void patch_manager_dialog::on_show_owned_games_only(int state)
{
	m_show_owned_games_only = state == Qt::CheckState::Checked;
	m_gui_settings->SetValue(gui::pm_show_owned, m_show_owned_games_only);
	filter_patches(ui->patch_filter->text());
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
