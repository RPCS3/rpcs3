﻿#include "trophy_notification_helper.h"

#include "trophy_notification_frame.h"

#include "../Emu/System.h"
#include "../Emu/RSX/Overlays/overlays.h"

s32 trophy_notification_helper::ShowTrophyNotification(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer)
{
	if (auto manager = g_fxo->get<rsx::overlays::display_manager>())
	{
		return manager->create<rsx::overlays::trophy_notification>()->show(trophy, trophy_icon_buffer);
	}

	if (!Emu.HasGui())
	{
		return 0;
	}

	Emu.CallAfter([=]
	{
		trophy_notification_frame* trophy_notification = new trophy_notification_frame(trophy_icon_buffer, trophy, m_game_window->frameGeometry().height() / 10);

		// Move notification to upper lefthand corner
		trophy_notification->move(m_game_window->mapToGlobal(QPoint(0, 0)));
		trophy_notification->show();
	});

	return 0;
}
