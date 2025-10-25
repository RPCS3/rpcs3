#pragma once

#include <optional>
#include "util/types.hpp"

#include <QDialog>
#include <QLineEdit>

constexpr auto UI_FIG_NUM = 8;

class kamen_rider_creator_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit kamen_rider_creator_dialog(QWidget* parent);
	QString get_file_path() const;

protected:
	QString file_path;
};

class kamen_rider_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit kamen_rider_dialog(QWidget* parent);
	~kamen_rider_dialog();
	static kamen_rider_dialog* get_dlg(QWidget* parent);

	kamen_rider_dialog(kamen_rider_dialog const&) = delete;
	void operator=(kamen_rider_dialog const&) = delete;

protected:
	void clear_kamen_rider(u8 slot);
	void create_kamen_rider(u8 slot);
	void load_kamen_rider(u8 slot);
	void load_kamen_rider_path(u8 slot, const QString& path);

	void update_edits();

protected:
	QLineEdit* edit_kamen_riders[UI_FIG_NUM]{};
	static std::optional<std::tuple<u8, u8, u8>> figure_slots[UI_FIG_NUM];

private:
	static kamen_rider_dialog* inst;
};
