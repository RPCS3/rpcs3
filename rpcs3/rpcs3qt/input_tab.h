#pragma once

#include "emu_settings.h"

#include <QWidget>

#include <memory>

class input_tab : public QWidget
{
	Q_OBJECT

public:
	explicit input_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent = 0);
};
