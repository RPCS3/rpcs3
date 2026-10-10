#pragma once

#include "flow_widget.h"
#include "gui_game_info.h"

#include <QComboBox>
#include <QDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QPixmap>
#include <QSize>
#include <QShowEvent>
#include <array>
#include <vector>
#include <map>

class screenshot_manager_dialog : public QDialog
{
	Q_OBJECT

public:
	screenshot_manager_dialog(const std::vector<game_info>& games, QWidget* parent = nullptr);
	~screenshot_manager_dialog();

Q_SIGNALS:
	void signal_entry_parsed(const QString& path);

private Q_SLOTS:
	void add_entry(const QString& path);
	void show_preview(const QString& path);

protected:
	void showEvent(QShowEvent* event) override;

private:
	void reload();

	enum class type_filter
	{
		all,
		rpcs3,
		cell
	};

	enum class sort_filter
	{
		game,
		date
	};

	std::vector<game_info> m_games;
	bool m_abort_parsing = false;
	const std::array<int, 1> m_parsing_threads{0};
	QFutureWatcher<void> m_parsing_watcher;
	flow_widget* m_flow_widget = nullptr;
	QComboBox* m_combo_sort_filter = nullptr;
	QComboBox* m_combo_game_filter = nullptr;
	QComboBox* m_combo_type_filter = nullptr;
	QSize m_icon_size;
	QPixmap m_placeholder;
	std::map<QString, std::vector<QFileInfo>> m_game_folders;
};
