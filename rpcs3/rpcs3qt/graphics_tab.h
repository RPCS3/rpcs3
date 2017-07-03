#pragma once

#include "emu_settings.h"

#include <QWidget>

#include <memory>

class graphics_tab : public QWidget
{
	Q_OBJECT

public:
	explicit graphics_tab(std::shared_ptr<emu_settings> xemu_settings, Render_Creator r_Creator, QWidget *parent = 0);

private:
	std::shared_ptr<emu_settings> xemu_settings;
	QString m_oldRender = "";
	bool m_isD3D12 = false;
	bool m_isVulkan = false;
};
