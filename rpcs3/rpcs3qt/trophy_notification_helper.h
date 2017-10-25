#pragma once

#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"

#include <QWindow>

class trophy_notification_helper : public TrophyNotificationBase
{
public:
	trophy_notification_helper(QWindow* game_window) : m_game_window(game_window) { };
	s32 ShowTrophyNotification(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer) override;
private:
	QWindow* m_game_window;
};
