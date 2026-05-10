#pragma once

#include "Utilities/bin_patch.h"

#include <QDialog>
#include <QComboBox>
#include <QKeyEvent>

namespace Ui
{
	class patch_creator_dialog;
}

class patch_creator_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit patch_creator_dialog(QWidget* parent = nullptr);
	~patch_creator_dialog();

private:
	std::unique_ptr<Ui::patch_creator_dialog> ui;
	QFont mMonoFont;
	QColor mValidColor;
	QColor mInvalidColor;
	bool m_valid = true; // Will be invalidated immediately
	QRegularExpressionValidator* m_offset_validator = nullptr;

	enum class move_direction
	{
		up,
		down
	};

	void add_instruction(int row);
	void remove_instructions();
	void move_instructions(int src_row, int rows_to_move, int distance, move_direction dir);
	bool can_move_instructions(QModelIndexList& selection, move_direction dir);

	static void init_patch_type_bombo_box(QComboBox* combo_box, patch_type set_type, bool searchable);
	QComboBox* create_patch_type_bombo_box(patch_type set_type) const;

	void update_validator(int index, QComboBox* combo_box, QLineEdit* line_edit) const;

private Q_SLOTS:
	void show_table_menu(const QPoint& pos);
	void validate(const QString& patch);
	void generate_yml(const QString& text = {});
	void export_patch();

protected:
	bool eventFilter(QObject* object, QEvent* event) override;
};
