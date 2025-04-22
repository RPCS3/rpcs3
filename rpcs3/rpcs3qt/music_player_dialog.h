#pragma once

#include <QDialog>

class music_handler_base;

namespace Ui
{
	class music_player_dialog;
}

class music_player_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit music_player_dialog(QWidget* parent = nullptr);
	~music_player_dialog();

private:
	void select_file();

	std::unique_ptr<Ui::music_player_dialog> ui;
	std::unique_ptr<music_handler_base> m_handler;
	std::string m_file_path;
};
