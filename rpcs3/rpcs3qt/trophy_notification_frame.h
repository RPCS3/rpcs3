﻿#pragma once

#include "Emu/Cell/Modules/sceNpTrophy.h"

#include <QWidget>

class trophy_notification_frame : public QWidget
{
public:
	trophy_notification_frame(const std::vector<uchar>& imgBuffer, const SceNpTrophyDetails& trophy, int height);
};