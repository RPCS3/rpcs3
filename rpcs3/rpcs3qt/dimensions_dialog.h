#pragma once

#include "util/types.hpp"

#include <QDialog>
#include <QLineEdit>
#include <QGridLayout>

class minifig_creator_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit minifig_creator_dialog(QWidget* parent);
	QString get_file_path() const;

protected:
	QString m_file_path;
};

class minifig_move_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit minifig_move_dialog(QWidget* parent, u8 old_index);
	u8 get_new_pad() const;
	u8 get_new_index() const;

protected:
	u8 m_pad = 0;
	u8 m_index = 0;

private:
	void add_minifig_position(QGridLayout* grid_panel, u8 index, u8 row, u8 column, u8 old_index);
};

class dimensions_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit dimensions_dialog(QWidget* parent);
	~dimensions_dialog();
	static dimensions_dialog* get_dlg(QWidget* parent);

	dimensions_dialog(dimensions_dialog const&) = delete;
	void operator=(dimensions_dialog const&) = delete;

protected:
	void clear_figure(u8 pad, u8 index);
	void create_figure(u8 pad, u8 index);
	void load_figure(u8 pad, u8 index);
	void move_figure(u8 pad, u8 index);
	void load_figure_path(u8 pad, u8 index, const QString& path);

protected:
	std::array<QLineEdit*, 7> m_edit_figures{};

private:
	void add_minifig_slot(QHBoxLayout* layout, u8 pad, u8 index);
	static dimensions_dialog* inst;
};
