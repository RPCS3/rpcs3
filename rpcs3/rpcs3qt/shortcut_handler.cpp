#include "stdafx.h"
#include "shortcut_handler.h"

LOG_CHANNEL(shortcut_log, "Shortcuts");

shortcut_handler::shortcut_handler(gui::shortcuts::shortcut_handler_id handler_id, QObject* parent, const std::shared_ptr<gui_settings>& gui_settings)
	: QObject(parent), m_handler_id(handler_id), m_gui_settings(gui_settings)
{
	// Initialize shortcuts
	shortcut_settings sc_settings{};

	for (const auto& [shortcut_key, info] : sc_settings.shortcut_map)
	{
		// Skip shortcuts that weren't meant for this handler
		if (handler_id != info.handler_id)
		{
			continue;
		}

		const QKeySequence key_sequence = sc_settings.get_key_sequence(info, gui_settings);
		QShortcut* shortcut = new QShortcut(key_sequence, parent);
		shortcut->setAutoRepeat(info.allow_auto_repeat);

		shortcut_key_info key_info{};
		key_info.shortcut = shortcut;
		key_info.info = info;
		key_info.key_sequence = key_sequence;

		m_shortcuts[shortcut_key] = key_info;

		connect(shortcut, &QShortcut::activated, this, [this, key = shortcut_key]()
		{
			handle_shortcut(key, m_shortcuts[key].key_sequence);
		});
		connect(shortcut, &QShortcut::activatedAmbiguously, this, [this, key = shortcut_key]()
		{
			// TODO: do not allow same shortcuts and remove this connect
			// activatedAmbiguously will trigger if you have the same key sequence for several shortcuts
			const QKeySequence& key_sequence = m_shortcuts[key].key_sequence;
			shortcut_log.error("%s: Shortcut activated ambiguously: %s (%s)", m_handler_id, key, key_sequence.toString());
			handle_shortcut(key, key_sequence);
		});
	}
}

void shortcut_handler::update()
{
	shortcut_log.notice("%s: Updating shortcuts", m_handler_id);

	shortcut_settings sc_settings{};

	for (const auto& [shortcut_key, info] : sc_settings.shortcut_map)
	{
		// Skip shortcuts that weren't meant for this handler
		if (m_handler_id != info.handler_id || !m_shortcuts.contains(shortcut_key))
		{
			continue;
		}

		const QKeySequence key_sequence = sc_settings.get_key_sequence(info, m_gui_settings);

		shortcut_key_info& key_info = m_shortcuts[shortcut_key];
		key_info.key_sequence = key_sequence;
		if (key_info.shortcut)
		{
			key_info.shortcut->setKey(key_sequence);
		}
	}
}

void shortcut_handler::handle_shortcut(gui::shortcuts::shortcut shortcut_key, const QKeySequence& key_sequence)
{
	shortcut_log.notice("%s: Shortcut pressed: %s (%s)", m_handler_id, shortcut_key, key_sequence.toString());

	Q_EMIT shortcut_activated(shortcut_key, key_sequence);
}
