#pragma once

#include "util/types.hpp"
#include "Emu/Io/recording_config.h"

#include <QDialog>

namespace Ui
{
	class recording_settings_dialog;
}

struct AVCodec;

class recording_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	recording_settings_dialog(QWidget* parent = nullptr);
	virtual ~recording_settings_dialog();

private:
	enum class quality_preset
	{
		_720p_30,
		_720p_60,
		_1080p_30,
		_1080p_60,
		_1440p_30,
		_1440p_60,
		_2160p_30,
		_2160p_60,
		custom
	};

	void update_preset();
	void update_ui();

	static void select_preset(quality_preset preset, cfg_recording& cfg);
	static quality_preset current_preset();

	Ui::recording_settings_dialog* ui;

	std::vector<const AVCodec*> m_video_codecs;
	std::vector<const AVCodec*> m_audio_codecs;
};
