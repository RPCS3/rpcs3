#pragma once

#include <optional>
#include "util/types.hpp"

#include <QDialog>
#include <QLineEdit>
#include <QVBoxLayout>

class figure_creator_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit figure_creator_dialog(QWidget* parent, u8 slot);
	QString get_file_path() const;

protected:
	QString m_file_path;

private:
	bool create_blank_figure(u32 character, u8 series);
};

class infinity_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit infinity_dialog(QWidget* parent);
	~infinity_dialog();
	static infinity_dialog* get_dlg(QWidget* parent);

	infinity_dialog(infinity_dialog const&) = delete;
	void operator=(infinity_dialog const&) = delete;

protected:
	void clear_figure(u8 slot);
	void create_figure(u8 slot);
	void load_figure(u8 slot);
	void load_figure_path(u8 slot, const QString& path);

protected:
	std::array<QLineEdit*, 9> m_edit_figures{};
	static std::array<std::optional<u32>, 9> figure_slots;

private:
	void add_figure_slot(QVBoxLayout* vbox_group, QString name, u8 slot);
	static infinity_dialog* inst;
};
