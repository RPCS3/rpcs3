#pragma once

#include "shortcut_settings.h"

#include <QShortcut>
#include <QWidget>

#include <map>

class gui_settings;

class shortcut_handler : public QObject
{
	Q_OBJECT

public:
	shortcut_handler(gui::shortcuts::shortcut_handler_id handler_id, QObject* parent, const std::shared_ptr<gui_settings>& gui_settings);

Q_SIGNALS:
	void shortcut_activated(gui::shortcuts::shortcut shortcut_key, const QKeySequence& key_sequence);

public Q_SLOTS:
	void update();

private:
	void handle_shortcut(gui::shortcuts::shortcut shortcut_key, const QKeySequence& key_sequence);
	QShortcut* make_shortcut(gui::shortcuts::shortcut key, const shortcut_info& info, const QKeySequence& key_sequence);

	gui::shortcuts::shortcut_handler_id m_handler_id;
	std::shared_ptr<gui_settings> m_gui_settings;

	struct shortcut_key_info
	{
		QShortcut* shortcut = nullptr;
		QKeySequence key_sequence{};
		shortcut_info info{};
	};
	std::map<gui::shortcuts::shortcut, shortcut_key_info> m_shortcuts;
};
