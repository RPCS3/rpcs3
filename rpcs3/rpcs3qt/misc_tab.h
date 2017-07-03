#pragma once

#include "emu_settings.h"

#include <QWidget>

#include <memory>

class misc_tab : public QWidget
{
	Q_OBJECT

public:
	explicit misc_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent = 0);
};
