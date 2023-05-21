#include "trophy_notification_helper.h"
#include "trophy_notification_frame.h"

#include "../Emu/IdManager.h"
#include "../Emu/System.h"

#include "../Emu/RSX/Overlays/overlay_manager.h"
#include "../Emu/RSX/Overlays/overlay_trophy_notification.h"

#include "Utilities/File.h"

s32 trophy_notification_helper::ShowTrophyNotification(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer)
{
	if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
	{
		// Allow adding more than one trophy notification. The notification class manages scheduling
		auto popup = std::make_shared<rsx::overlays::trophy_notification>();
		return manager->add(popup, false)->show(trophy, trophy_icon_buffer);
	}

	if (!Emu.HasGui())
	{
		return 0;
	}

	Emu.CallFromMainThread([=, this]
	{
		trophy_notification_frame* trophy_notification = new trophy_notification_frame(trophy_icon_buffer, trophy, m_game_window->frameGeometry().height() / 10);

		// Move notification to upper lefthand corner
		trophy_notification->move(m_game_window->mapToGlobal(QPoint(0, 0)));
		trophy_notification->show();

		Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_trophy.wav");
	});

	return 0;
}
