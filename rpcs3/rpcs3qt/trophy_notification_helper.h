#pragma once

#include "util/types.hpp"
#include "Emu/Cell/Modules/sceNpTrophy.h"

#include <QPointer>
#include <QWindow>
#include <vector>

class trophy_notification_helper : public TrophyNotificationBase
{
public:
	explicit trophy_notification_helper(QPointer<QWindow> game_window) : m_game_window(game_window) {}
	s32 ShowTrophyNotification(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer) override;
private:
	QPointer<QWindow> m_game_window; // Becomes nullptr if the window is destroyed.
};
