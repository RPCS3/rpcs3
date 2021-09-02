#include "ui_patch_creator_dialog.h"
#include "patch_creator_dialog.h"
#include "table_item_delegate.h"
#include "qt_utils.h"
#include "Utilities/Config.h"

#include <QCompleter>
#include <QFontDatabase>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>

LOG_CHANNEL(patch_log, "PAT");

constexpr auto qstr = QString::fromStdString;
inline std::string sstr(const QString& _in) { return _in.toStdString(); }

Q_DECLARE_METATYPE(patch_type)

enum patch_column : int
{
	type = 0,
	offset,
	value,
	comment
};

patch_creator_dialog::patch_creator_dialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::patch_creator_dialog)
	, mMonoFont(QFontDatabase::systemFont(QFontDatabase::FixedFont))
	, mValidColor(gui::utils::get_label_color("log_level_success"))
	, mInvalidColor(gui::utils::get_label_color("log_level_error"))
{
	ui->setupUi(this);
	ui->patchEdit->setFont(mMonoFont);
	ui->addPatchOffsetEdit->setFont(mMonoFont);
	ui->addPatchValueEdit->setFont(mMonoFont);
	ui->instructionTable->setFont(mMonoFont);
	ui->instructionTable->setItemDelegate(new table_item_delegate(this, false));
	ui->instructionTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Fixed);
	ui->instructionTable->installEventFilter(this);
	ui->versionMinorSpinBox->setValue(0);
	ui->versionMajorSpinBox->setValue(0);

	QMenuBar* menu_bar = new QMenuBar(this);
	QMenu* file_menu = menu_bar->addMenu(tr("File"));
	QAction* export_act = file_menu->addAction(tr("&Export Patch"));
	export_act->setShortcut(QKeySequence("Ctrl+E"));
	export_act->installEventFilter(this);
	layout()->setMenuBar(menu_bar);

	connect(export_act, &QAction::triggered, this, &patch_creator_dialog::export_patch);
	connect(ui->hashEdit, &QLineEdit::textChanged, this, &patch_creator_dialog::generate_yml);
	connect(ui->authorEdit, &QLineEdit::textChanged, this, &patch_creator_dialog::generate_yml);
	connect(ui->patchNameEdit, &QLineEdit::textChanged, this, &patch_creator_dialog::generate_yml);
	connect(ui->gameEdit, &QLineEdit::textChanged, this, &patch_creator_dialog::generate_yml);
	connect(ui->gameVersionEdit, &QLineEdit::textChanged, this, &patch_creator_dialog::generate_yml);
	connect(ui->serialEdit, &QLineEdit::textChanged, this, &patch_creator_dialog::generate_yml);
	connect(ui->notesEdit, &QLineEdit::textChanged, this, &patch_creator_dialog::generate_yml);
	connect(ui->groupEdit, &QLineEdit::textChanged, this, &patch_creator_dialog::generate_yml);
	connect(ui->versionMinorSpinBox, &QSpinBox::textChanged, this, &patch_creator_dialog::generate_yml);
	connect(ui->versionMajorSpinBox, &QSpinBox::textChanged, this, &patch_creator_dialog::generate_yml);
	connect(ui->instructionTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem*){ generate_yml(); });
	connect(ui->instructionTable, &QTableWidget::customContextMenuRequested, this, &patch_creator_dialog::show_table_menu);
	connect(ui->addPatchButton, &QAbstractButton::clicked, this, [this]() { add_instruction(ui->instructionTable->rowCount()); });

	init_patch_type_bombo_box(ui->addPatchTypeComboBox, patch_type::be32, false);

	generate_yml();
}

patch_creator_dialog::~patch_creator_dialog()
{
}

void patch_creator_dialog::init_patch_type_bombo_box(QComboBox* combo_box, patch_type set_type, bool searchable)
{
	if (!combo_box) return;

	combo_box->clear();

	QStringList types;

	for (const std::string& type : cfg::try_to_enum_list(&fmt_class_string<patch_type>::format))
	{
		if (const patch_type t = patch_engine::get_patch_type(type); t != patch_type::invalid)
		{
			types << qstr(type);

			combo_box->addItem(types.last(), QVariant::fromValue<patch_type>(t));

			if (t == set_type)
			{
				combo_box->setCurrentText(types.last());
			}
		}
	}

	if (searchable)
	{
		QCompleter* completer = new QCompleter(types, combo_box);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		completer->setCompletionMode(QCompleter::PopupCompletion);
		completer->setFilterMode(Qt::MatchContains);

		combo_box->setCompleter(completer);
		combo_box->setEditable(true);
		combo_box->setInsertPolicy(QComboBox::NoInsert);
	}
}

QComboBox* patch_creator_dialog::create_patch_type_bombo_box(patch_type set_type)
{
	QComboBox* combo_box = new QComboBox;
	init_patch_type_bombo_box(combo_box, set_type, true);
	connect(combo_box, &QComboBox::currentTextChanged, this, &patch_creator_dialog::generate_yml);
	return combo_box;
}

void patch_creator_dialog::show_table_menu(const QPoint& pos)
{
	QMenu menu;

	QModelIndexList selection = ui->instructionTable->selectionModel()->selectedRows();

	if (selection.isEmpty())
	{
		QAction* act_add_instruction = menu.addAction(tr("&Add Instruction"));
		connect(act_add_instruction, &QAction::triggered, [this]()
		{
			add_instruction(ui->instructionTable->rowCount());
		});
	}
	else
	{
		if (selection.count() == 1)
		{
			QAction* act_add_instruction_above = menu.addAction(tr("&Add Instruction Above"));
			connect(act_add_instruction_above, &QAction::triggered, [this, row = selection.first().row()]()
			{
				add_instruction(row);
			});

			QAction* act_add_instruction_below = menu.addAction(tr("&Add Instruction Below"));
			connect(act_add_instruction_below, &QAction::triggered, [this, row = selection.first().row()]()
			{
				add_instruction(row + 1);
			});
		}

		const bool can_move_up = can_move_instructions(selection, move_direction::up);
		const bool can_move_down = can_move_instructions(selection, move_direction::down);

		if (can_move_up)
		{
			menu.addSeparator();

			QAction* act_move_instruction_up = menu.addAction(tr("&Move Instruction(s) Up"));
			connect(act_move_instruction_up, &QAction::triggered, [this, &selection]()
			{
				move_instructions(selection.first().row(), selection.count(), 1, move_direction::up);
			});
		}

		if (can_move_down)
		{
			if (!can_move_up)
				menu.addSeparator();

			QAction* act_move_instruction_down = menu.addAction(tr("&Move Instruction(s) Down"));
			connect(act_move_instruction_down, &QAction::triggered, [this, &selection]()
			{
				move_instructions(selection.first().row(), selection.count(), 1, move_direction::down);
			});
		}

		menu.addSeparator();

		QAction* act_remove_instruction = menu.addAction(tr("&Remove Instruction(s)"));
		connect(act_remove_instruction, &QAction::triggered, [this]()
		{
			remove_instructions();
		});
	}

	menu.addSeparator();

	QAction* act_clear_table = menu.addAction(tr("&Clear Table"));
	connect(act_clear_table, &QAction::triggered, [this]()
	{
		patch_log.notice("Patch Creator: Clearing instruction table...");
		ui->instructionTable->clearContents();
		ui->instructionTable->setRowCount(0);
		generate_yml();
	});

	menu.exec(ui->instructionTable->viewport()->mapToGlobal(pos));
}

void patch_creator_dialog::add_instruction(int row)
{
	const QString type    = ui->addPatchTypeComboBox->currentText();
	const QString offset  = ui->addPatchOffsetEdit->text();
	const QString value   = ui->addPatchValueEdit->text();
	const QString comment = ui->addPatchCommentEdit->text();

	const patch_type t = patch_engine::get_patch_type(type.toStdString());
	QComboBox* combo_box = create_patch_type_bombo_box(t);

	ui->instructionTable->insertRow(std::max(0, std::min(row, ui->instructionTable->rowCount())));
	ui->instructionTable->setCellWidget(row, patch_column::type, combo_box);
	ui->instructionTable->setItem(row, patch_column::offset, new QTableWidgetItem(offset));
	ui->instructionTable->setItem(row, patch_column::value, new QTableWidgetItem(value));
	ui->instructionTable->setItem(row, patch_column::comment, new QTableWidgetItem(comment));

	patch_log.notice("Patch Creator: Inserted instruction [ %s, %s, %s ] at row %d", sstr(combo_box->currentText()), sstr(offset), sstr(value), row);
	generate_yml();
}

void patch_creator_dialog::remove_instructions()
{
	QModelIndexList selection(ui->instructionTable->selectionModel()->selectedRows());
	if (selection.empty())
		return;

	std::sort(selection.rbegin(), selection.rend());
	for (const QModelIndex& index : selection)
	{
		patch_log.notice("Patch Creator: Removing instruction in row %d...", index.row());
		ui->instructionTable->removeRow(index.row());
	}

	generate_yml();
}

void patch_creator_dialog::move_instructions(int src_row, int rows_to_move, int distance, move_direction dir)
{
	patch_log.notice("Patch Creator: Moving %d instruction(s) from row %d %s by %d...", rows_to_move, src_row, dir == move_direction::up ? "up" : "down", distance);

	if (src_row < 0 || src_row >= ui->instructionTable->rowCount() || distance < 1)
		return;

	rows_to_move = std::max(0, std::min(rows_to_move, ui->instructionTable->rowCount() - src_row));
	if (rows_to_move < 1)
		return;

	const int dst_row = std::max(0, std::min(ui->instructionTable->rowCount() - rows_to_move, dir == move_direction::up ? src_row - distance : src_row + distance));
	if (dir == move_direction::up ? dst_row >= src_row : dst_row <= src_row)
		return;

	const int friends_to_relocate = std::abs(dst_row - src_row);
	const int friends_src_row = dir == move_direction::up ? dst_row : src_row + rows_to_move;
	const int friends_dst_row = dir == move_direction::up ? dst_row + rows_to_move : src_row;

	std::vector<patch_type> moving_types(rows_to_move);
	std::vector<std::vector<QTableWidgetItem*>> moving_rows(rows_to_move);

	std::vector<patch_type> friend_types(friends_to_relocate);
	std::vector<std::vector<QTableWidgetItem*>> friend_rows(friends_to_relocate);

	const auto get_row_type = [this](int i) -> patch_type
	{
		if (const QComboBox* type_item = qobject_cast<QComboBox*>(ui->instructionTable->cellWidget(i, patch_column::type)))
			return type_item->currentData().value<patch_type>();
		return patch_type::invalid;
	};

	const auto set_row_type_widget = [this](int i, patch_type type) -> void
	{
		ui->instructionTable->setCellWidget(i, patch_column::type, create_patch_type_bombo_box(type));
	};

	for (int i = 0; i < rows_to_move; i++)
	{
		moving_types[i] = get_row_type(src_row + i);
		moving_rows[i].push_back(ui->instructionTable->takeItem(src_row + i, patch_column::type));
		moving_rows[i].push_back(ui->instructionTable->takeItem(src_row + i, patch_column::offset));
		moving_rows[i].push_back(ui->instructionTable->takeItem(src_row + i, patch_column::value));
		moving_rows[i].push_back(ui->instructionTable->takeItem(src_row + i, patch_column::comment));
	}

	for (int i = 0; i < friends_to_relocate; i++)
	{
		friend_types[i] = get_row_type(friends_src_row + i);
		friend_rows[i].push_back(ui->instructionTable->takeItem(friends_src_row + i, patch_column::type));
		friend_rows[i].push_back(ui->instructionTable->takeItem(friends_src_row + i, patch_column::offset));
		friend_rows[i].push_back(ui->instructionTable->takeItem(friends_src_row + i, patch_column::value));
		friend_rows[i].push_back(ui->instructionTable->takeItem(friends_src_row + i, patch_column::comment));
	}

	for (int i = 0; i < rows_to_move; i++)
	{
		int item_index = 0;
		ui->instructionTable->setCellWidget(dst_row + i, patch_column::type, create_patch_type_bombo_box(moving_types[i]));
		ui->instructionTable->setItem(dst_row + i, patch_column::type, moving_rows[i][item_index++]);
		ui->instructionTable->setItem(dst_row + i, patch_column::offset, moving_rows[i][item_index++]);
		ui->instructionTable->setItem(dst_row + i, patch_column::value, moving_rows[i][item_index++]);
		ui->instructionTable->setItem(dst_row + i, patch_column::comment, moving_rows[i][item_index++]);
	}

	for (int i = 0; i < friends_to_relocate; i++)
	{
		int item_index = 0;
		ui->instructionTable->setCellWidget(friends_dst_row + i, patch_column::type, create_patch_type_bombo_box(friend_types[i]));
		ui->instructionTable->setItem(friends_dst_row + i, patch_column::type, friend_rows[i][item_index++]);
		ui->instructionTable->setItem(friends_dst_row + i, patch_column::offset, friend_rows[i][item_index++]);
		ui->instructionTable->setItem(friends_dst_row + i, patch_column::value, friend_rows[i][item_index++]);
		ui->instructionTable->setItem(friends_dst_row + i, patch_column::comment, friend_rows[i][item_index++]);
	}

	ui->instructionTable->clearSelection();
	ui->instructionTable->setRangeSelected(QTableWidgetSelectionRange(dst_row, 0, dst_row + rows_to_move - 1, ui->instructionTable->columnCount() - 1), true);

	generate_yml();
}

bool patch_creator_dialog::can_move_instructions(QModelIndexList& selection, move_direction dir)
{
	if (selection.isEmpty())
		return false;

	std::sort(selection.begin(), selection.end());

	// Check if there are any gaps in the selection
	for (int i = 1, row = selection.first().row(); i < selection.count(); i++)
	{
		if (++row != selection[i].row())
			return false;
	}

	if (dir == move_direction::up)
		return selection.first().row() > 0;

	return selection.last().row() < ui->instructionTable->rowCount() - 1;
}

void patch_creator_dialog::validate()
{
	patch_engine::patch_map patches;
	const std::string content = ui->patchEdit->toPlainText().toStdString();
	const bool is_valid = patch_engine::load(patches, "From Patch Creator", content, true);

	if (is_valid != m_valid)
	{
		QPalette palette = ui->validLabel->palette();

		if (is_valid)
		{
			ui->validLabel->setText(tr("Valid Patch"));
			palette.setColor(ui->validLabel->foregroundRole(), mValidColor);
			patch_log.success("Patch Creator: Validation successful!");
		}
		else
		{
			ui->validLabel->setText(tr("Validation Failed"));
			palette.setColor(ui->validLabel->foregroundRole(), mInvalidColor);
			patch_log.error("Patch Creator: Validation failed!");
		}

		ui->validLabel->setPalette(palette);
		m_valid = is_valid;
	}
}

void patch_creator_dialog::export_patch()
{
	if (!m_valid)
	{
		QMessageBox::information(this, tr("Patch invalid!"), tr("The patch validation failed.\nThe export of invalid patches is not allowed."));
		return;
	}

	const QString file_path = QFileDialog::getSaveFileName(this, tr("Select Patch File"), qstr(patch_engine::get_patches_path()), tr("patch.yml files (*.yml);;All files (*.*)"));
	if (file_path.isEmpty())
	{
		return;
	}

	if (QFile patch_file(file_path); patch_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		patch_file.write(ui->patchEdit->toPlainText().toUtf8());
		patch_file.close();
		patch_log.success("Exported patch to file '%s'", sstr(file_path));
	}
	else
	{
		patch_log.fatal("Failed to export patch to file '%s'", sstr(file_path));
	}
}

void patch_creator_dialog::generate_yml(const QString& /*text*/)
{
	QString patch;
	patch.append(QString("%0: %1\n").arg(qstr(patch_key::version)).arg(qstr(patch_engine_version)));
	patch.append("\n");
	patch.append(QString("%0:\n").arg(ui->hashEdit->text()));
	patch.append(QString("  \"%0\":\n").arg(ui->patchNameEdit->text()));
	patch.append(QString("    %0:\n").arg(qstr(patch_key::games)));
	patch.append(QString("      \"%0\":\n").arg(ui->gameEdit->text()));
	patch.append(QString("        %0: [ %1 ]\n").arg(ui->serialEdit->text()).arg(ui->gameVersionEdit->text()));
	patch.append(QString("    %0: \"%1\"\n").arg(qstr(patch_key::author)).arg(ui->authorEdit->text()));
	patch.append(QString("    %0: %1.%2\n").arg(qstr(patch_key::patch_version)).arg(ui->versionMajorSpinBox->text()).arg(ui->versionMinorSpinBox->text()));
	patch.append(QString("    %0: \"%1\"\n").arg(qstr(patch_key::group)).arg(ui->groupEdit->text()));
	patch.append(QString("    %0: \"%1\"\n").arg(qstr(patch_key::notes)).arg(ui->notesEdit->text()));
	patch.append(QString("    %0:\n").arg(qstr(patch_key::patch)));

	for (int i = 0; i < ui->instructionTable->rowCount(); i++)
	{
		const QComboBox* type_item           = qobject_cast<QComboBox*>(ui->instructionTable->cellWidget(i, patch_column::type));
		const QTableWidgetItem* offset_item  = ui->instructionTable->item(i, patch_column::offset);
		const QTableWidgetItem* value_item   = ui->instructionTable->item(i, patch_column::value);
		const QTableWidgetItem* comment_item = ui->instructionTable->item(i, patch_column::comment);

		const QString type    = type_item ? type_item->currentText() : "";
		const QString offset  = offset_item ? offset_item->text() : "";
		const QString value   = value_item ? value_item->text() : "";
		const QString comment = comment_item ? comment_item->text() : "";

		if (patch_engine::get_patch_type(type.toStdString()) == patch_type::invalid)
		{
			ui->patchEdit->setText(tr("Instruction %0: Type '%1' is invalid!").arg(i + 1).arg(type));
			return;
		}

		patch.append(QString("      - [ %0, %1, %2 ]%3\n").arg(type).arg(offset).arg(value).arg(comment.isEmpty() ? QStringLiteral("") : QString(" # %0").arg(comment)));
	}

	ui->patchEdit->setText(patch);
	validate();
}

bool patch_creator_dialog::eventFilter(QObject* object, QEvent* event)
{
	if (object != ui->instructionTable)
	{
		return QDialog::eventFilter(object, event);
	}

	if (event->type() == QEvent::KeyPress)
	{
		if (QKeyEvent* key_event = static_cast<QKeyEvent*>(event))
		{
			if (key_event->modifiers() == Qt::AltModifier)
			{
				switch (key_event->key())
				{
				case Qt::Key_Up:
				{
					QModelIndexList selection = ui->instructionTable->selectionModel()->selectedRows();
					if (can_move_instructions(selection, move_direction::up))
						move_instructions(selection.first().row(), selection.count(), 1, move_direction::up);
					return true;
				}
				case Qt::Key_Down:
				{
					QModelIndexList selection = ui->instructionTable->selectionModel()->selectedRows();
					if (can_move_instructions(selection, move_direction::down))
						move_instructions(selection.first().row(), selection.count(), 1, move_direction::down);
					return true;
				}
				default:
					break;
				}
			}
			else if (!key_event->isAutoRepeat())
			{
				switch (key_event->key())
				{
				case Qt::Key_Delete:
				{
					remove_instructions();
					return true;
				}
				default:
					break;
				}
			}
		}
	}

	return QDialog::eventFilter(object, event);
}
