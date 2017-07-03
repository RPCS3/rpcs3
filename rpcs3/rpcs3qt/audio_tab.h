#pragma once

#include "emu_settings.h"

#include <QWidget>

#include <memory>

class audio_tab : public QWidget
{
	Q_OBJECT

public:
	explicit audio_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent = 0);
};
