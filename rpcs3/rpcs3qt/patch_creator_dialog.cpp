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
#include <QStringBuilder>

#include <regex>

LOG_CHANNEL(patch_log, "PAT");

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
	, mValidColor(gui::utils::get_label_color("log_level_success", Qt::darkGreen, Qt::green))
	, mInvalidColor(gui::utils::get_label_color("log_level_error", Qt::red, Qt::red))
	, m_offset_validator(new QRegularExpressionValidator(QRegularExpression("^(0[xX])?[a-fA-F0-9]{0,8}$"), this))
{
	ui->setupUi(this);
	ui->patchEdit->setFont(mMonoFont);
	ui->patchEdit->setAcceptRichText(true);
	ui->addPatchOffsetEdit->setFont(mMonoFont);
	ui->addPatchOffsetEdit->setClearButtonEnabled(true);
	ui->addPatchValueEdit->setFont(mMonoFont);
	ui->addPatchValueEdit->setClearButtonEnabled(true);
	ui->addPatchCommentEdit->setClearButtonEnabled(true);
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
	connect(ui->addPatchTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){ update_validator(index, ui->addPatchTypeComboBox, ui->addPatchOffsetEdit); });
	update_validator(ui->addPatchTypeComboBox->currentIndex(), ui->addPatchTypeComboBox, ui->addPatchOffsetEdit);

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
			types << QString::fromStdString(type);

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

QComboBox* patch_creator_dialog::create_patch_type_bombo_box(patch_type set_type) const
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

void patch_creator_dialog::update_validator(int index, QComboBox* combo_box, QLineEdit* line_edit) const
{
	if (index < 0 || !combo_box || !line_edit || !combo_box->itemData(index).canConvert<patch_type>())
	{
		return;
	}

	switch (combo_box->itemData(index).value<patch_type>())
	{
	case patch_type::move_file:
	case patch_type::hide_file:
		line_edit->setValidator(nullptr);
		break;
	default:
		line_edit->setValidator(m_offset_validator);
		break;
	}
}

void patch_creator_dialog::add_instruction(int row)
{
	const QString type    = ui->addPatchTypeComboBox->currentText();
	const QString offset  = ui->addPatchOffsetEdit->text();
	const QString value   = ui->addPatchValueEdit->text();
	const QString comment = ui->addPatchCommentEdit->text();

	const patch_type t = patch_engine::get_patch_type(type.toStdString());

	switch (t)
	{
	case patch_type::move_file:
	case patch_type::hide_file:
		break;
	default:
	{
		int pos = 0;
		QString text_to_validate = offset;
		if (m_offset_validator->validate(text_to_validate, pos) == QValidator::Invalid)
		{
			QMessageBox::information(this, tr("Offset invalid!"), tr("The patch offset is invalid.\nThe offset has to be a hexadecimal number with 8 digits at most."));
			return;
		}
		break;
	}
	}

	QComboBox* combo_box = create_patch_type_bombo_box(t);

	ui->instructionTable->insertRow(std::max(0, std::min(row, ui->instructionTable->rowCount())));
	ui->instructionTable->setCellWidget(row, patch_column::type, combo_box);
	ui->instructionTable->setItem(row, patch_column::offset, new QTableWidgetItem(offset));
	ui->instructionTable->setItem(row, patch_column::value, new QTableWidgetItem(value));
	ui->instructionTable->setItem(row, patch_column::comment, new QTableWidgetItem(comment));

	patch_log.notice("Patch Creator: Inserted instruction [ %s, %s, %s ] at row %d", combo_box->currentText(), offset, value, row);
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

void patch_creator_dialog::validate(const QString& patch)
{
	patch_engine::patch_map patches;
	const std::string content = patch.toStdString();
	std::stringstream messages;
	const bool is_valid = patch_engine::load(patches, "From Patch Creator", content, true, &messages);

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

	if (is_valid)
	{
		ui->patchEdit->setText(patch);
		return;
	}

	// Search for erronous yml node locations in log message
	static const std::regex r("(line )(\\d+)(, column )(\\d+)");
	std::smatch sm;
	std::set<int> faulty_lines;

	for (std::string err = messages.str(); !err.empty() && std::regex_search(err, sm, r) && sm.size() == 5; err = sm.suffix())
	{
		if (s64 row{}; try_to_int64(&row, sm[2].str(), 0, u32{umax}))
		{
			faulty_lines.insert(row);
		}
	}

	// Create html and colorize offending lines
	const QString font_start_tag = QStringLiteral("<font color = \"") % mInvalidColor.name() % QStringLiteral("\">");;
	static const QString font_end_tag = QStringLiteral("</font>");
	static const QString line_break_tag = QStringLiteral("<br/>");

	QStringList lines = patch.split("\n");
	QString new_text;

	for (int i = 0; i < lines.size(); i++)
	{
		// Escape each line and replace raw whitespace
		const QString line = lines[i].toHtmlEscaped().replace(" ", "&nbsp;");

		if (faulty_lines.empty() || faulty_lines.contains(i))
		{
			new_text += font_start_tag + line + font_end_tag + line_break_tag;
		}
		else
		{
			new_text += line + line_break_tag;
		}
	}

	ui->patchEdit->setHtml(new_text);
}

void patch_creator_dialog::export_patch()
{
	if (!m_valid)
	{
		QMessageBox::information(this, tr("Patch invalid!"), tr("The patch validation failed.\nThe export of invalid patches is not allowed."));
		return;
	}

	const QString file_path = QFileDialog::getSaveFileName(this, tr("Select Patch File"), QString::fromStdString(patch_engine::get_patches_path()), tr("patch.yml files (*.yml);;All files (*.*)"));
	if (file_path.isEmpty())
	{
		return;
	}

	if (QFile patch_file(file_path); patch_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		patch_file.write(ui->patchEdit->toPlainText().toUtf8());
		patch_file.close();
		patch_log.success("Exported patch to file '%s'", file_path);
	}
	else
	{
		patch_log.fatal("Failed to export patch to file '%s'", file_path);
	}
}

void patch_creator_dialog::generate_yml(const QString& /*text*/)
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << patch_key::version << patch_engine_version;
	out << YAML::Newline;
	out << ui->hashEdit->text().toStdString();
	{
		out << YAML::BeginMap;
		out << YAML::DoubleQuoted << ui->patchNameEdit->text().toStdString();
		{
			out << YAML::BeginMap;
			out << patch_key::games;
			{
				out << YAML::BeginMap;
				out << YAML::DoubleQuoted << ui->gameEdit->text().toStdString();
				{
					out << YAML::BeginMap;
					out << ui->serialEdit->text().simplified().toStdString();
					{
						out << YAML::Flow << fmt::split(ui->gameVersionEdit->text().toStdString(), { ",", " " });
					}
					out << YAML::EndMap;
				}
				out << YAML::EndMap;
			}
			out << patch_key::author << YAML::DoubleQuoted << ui->authorEdit->text().toStdString();
			out << patch_key::patch_version << fmt::format("%s.%s", ui->versionMajorSpinBox->text(), ui->versionMinorSpinBox->text());
			out << patch_key::group << YAML::DoubleQuoted << ui->groupEdit->text().toStdString();
			out << patch_key::notes << YAML::DoubleQuoted << ui->notesEdit->text().toStdString();
			out << patch_key::patch;
			{
				out << YAML::BeginSeq;
				for (int i = 0; i < ui->instructionTable->rowCount(); i++)
				{
					const QComboBox* type_item           = qobject_cast<QComboBox*>(ui->instructionTable->cellWidget(i, patch_column::type));
					const QTableWidgetItem* offset_item  = ui->instructionTable->item(i, patch_column::offset);
					const QTableWidgetItem* value_item   = ui->instructionTable->item(i, patch_column::value);
					const QTableWidgetItem* comment_item = ui->instructionTable->item(i, patch_column::comment);

					const std::string type    = type_item ? type_item->currentText().toStdString() : "";
					const std::string offset  = offset_item ? offset_item->text().toStdString() : "";
					const std::string value   = value_item ? value_item->text().toStdString() : "";
					const std::string comment = comment_item ? comment_item->text().toStdString() : "";

					if (patch_engine::get_patch_type(type) == patch_type::invalid)
					{
						ui->patchEdit->setText(tr("Instruction %0: Type '%1' is invalid!").arg(i + 1).arg(type_item ? type_item->currentText() : ""));
						return;
					}

					out << YAML::Flow << YAML::BeginSeq << type << offset << value << YAML::EndSeq;

					if (!comment.empty())
					{
						out << YAML::Comment(comment);
					}
				}
				out << YAML::EndSeq;
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap;
	}
	out << YAML::EndMap;

	const QString patch = QString::fromUtf8(out.c_str(), out.size());

	validate(patch);
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
