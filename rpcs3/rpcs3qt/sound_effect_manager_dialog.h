#pragma once

#include "Emu/RSX/Overlays/overlays.h"

#include <QDialog>
#include <QPushButton>

class sound_effect_manager_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit sound_effect_manager_dialog(QWidget* parent = nullptr);
	~sound_effect_manager_dialog();

private:
	void update_widgets();

	struct widget
	{
		QPushButton* button = nullptr;
		QPushButton* play_button = nullptr;
	};

	std::map<rsx::overlays::sound_effect, widget> m_widgets;
};
