#include "stdafx.h"
#include "recording_settings_dialog.h"
#include "ui_recording_settings_dialog.h"

#include <QPushButton>

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

LOG_CHANNEL(cfg_log, "CFG");

static std::vector<const AVCodec*> get_video_codecs(const AVOutputFormat* fmt)
{
	std::vector<const AVCodec*> codecs;

	void* opaque = nullptr;
	while (const AVCodec* codec = av_codec_iterate(&opaque))
	{
		if (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL)
			continue;

		if (codec->type != AVMediaType::AVMEDIA_TYPE_VIDEO)
			continue;

		switch (codec->id)
		{
		case AV_CODEC_ID_H264:
		case AV_CODEC_ID_HEVC:
		case AV_CODEC_ID_MPEG4:
		case AV_CODEC_ID_AV1:
			break;
		default:
			continue;
		}

		if (!av_codec_is_encoder(codec))
			continue;

		if (avformat_query_codec(fmt, codec->id, FF_COMPLIANCE_NORMAL) != 1)
			continue;

		codecs.push_back(codec);
	}

	return codecs;
}

static std::vector<const AVCodec*> get_audio_codecs(const AVOutputFormat* fmt)
{
	std::vector<const AVCodec*> codecs;

	void* opaque = nullptr;
	while (const AVCodec* codec = av_codec_iterate(&opaque))
	{
		if (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL)
			continue;

		if (codec->type != AVMediaType::AVMEDIA_TYPE_AUDIO)
			continue;

		if (!av_codec_is_encoder(codec))
			continue;

		if (avformat_query_codec(fmt, codec->id, FF_COMPLIANCE_NORMAL) != 1)
			continue;

		codecs.push_back(codec);
	}

	return codecs;
}

recording_settings_dialog::recording_settings_dialog(QWidget* parent)
	: QDialog(parent), ui(new Ui::recording_settings_dialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	if (!g_cfg_recording.load())
	{
		cfg_log.notice("Could not load recording config. Using defaults.");
	}

	ui->combo_presets->addItem(tr("720p 30fps"),  static_cast<int>(quality_preset::_720p_30));
	ui->combo_presets->addItem(tr("720p 60fps"),  static_cast<int>(quality_preset::_720p_60));
	ui->combo_presets->addItem(tr("1080p 30fps"), static_cast<int>(quality_preset::_1080p_30));
	ui->combo_presets->addItem(tr("1080p 60fps"), static_cast<int>(quality_preset::_1080p_60));
	ui->combo_presets->addItem(tr("1440p 30fps"), static_cast<int>(quality_preset::_1440p_30));
	ui->combo_presets->addItem(tr("1440p 60fps"), static_cast<int>(quality_preset::_1440p_60));
	ui->combo_presets->addItem(tr("2160p 30fps"), static_cast<int>(quality_preset::_2160p_30));
	ui->combo_presets->addItem(tr("2160p 60fps"), static_cast<int>(quality_preset::_2160p_60));
	ui->combo_presets->addItem(tr("Custom"),      static_cast<int>(quality_preset::custom));
	connect(ui->combo_presets, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		const QVariant var = ui->combo_presets->itemData(index);
		if (var.canConvert<int>())
		{
			const quality_preset preset = static_cast<quality_preset>(var.toInt());
			select_preset(preset, g_cfg_recording);
			update_ui();
		}
	});

	ui->combo_resolution->addItem("360p",  QVariant::fromValue(QPair<int, int>(640, 360)));
	ui->combo_resolution->addItem("480p",  QVariant::fromValue(QPair<int, int>(854, 480)));
	ui->combo_resolution->addItem("720p",  QVariant::fromValue(QPair<int, int>(1280, 720)));
	ui->combo_resolution->addItem("1080p", QVariant::fromValue(QPair<int, int>(1920, 1080)));
	ui->combo_resolution->addItem("1440p", QVariant::fromValue(QPair<int, int>(2560, 1440)));
	ui->combo_resolution->addItem("2160p", QVariant::fromValue(QPair<int, int>(3840, 2160)));
	connect(ui->combo_resolution, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		const QVariant var = ui->combo_resolution->itemData(index);
		if (var.canConvert<QPair<int, int>>())
		{
			const QPair<int, int> size = var.value<QPair<int, int>>();
			g_cfg_recording.video.width.set(size.first);
			g_cfg_recording.video.height.set(size.second);
			update_preset();
		}
	});

	const AVOutputFormat* fmt = av_guess_format("mp4", nullptr, nullptr);
	m_video_codecs = get_video_codecs(fmt);
	m_audio_codecs = get_audio_codecs(fmt);

	for (const AVCodec* codec : m_video_codecs)
	{
		if (!codec) continue;

		const std::string name = codec->long_name ? codec->long_name : avcodec_get_name(codec->id);
		ui->combo_video_codec->addItem(QString::fromStdString(name), static_cast<int>(codec->id));
	}

	for (const AVCodec* codec : m_audio_codecs)
	{
		if (!codec) continue;

		const std::string name = codec->long_name ? codec->long_name : avcodec_get_name(codec->id);
		ui->combo_audio_codec->addItem(QString::fromStdString(name), static_cast<int>(codec->id));
	}

	connect(ui->combo_video_codec, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		const QVariant var = ui->combo_video_codec->itemData(index);
		if (var.canConvert<int>())
		{
			const int codec_id = var.toInt();
			g_cfg_recording.video.video_codec.set(codec_id);
			update_preset();
		}
	});

	connect(ui->combo_audio_codec, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		const QVariant var = ui->combo_audio_codec->itemData(index);
		if (var.canConvert<int>())
		{
			const int codec_id = var.toInt();
			g_cfg_recording.audio.audio_codec.set(codec_id);
			update_preset();
		}
	});

	ui->combo_framerate->addItem("30", 30);
	ui->combo_framerate->addItem("60", 60);
	connect(ui->combo_framerate, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		const QVariant var = ui->combo_framerate->itemData(index);
		if (var.canConvert<int>())
		{
			const int fps = var.toInt();
			g_cfg_recording.video.framerate.set(fps);
			update_preset();
		}
	});

	ui->spinbox_video_bitrate->setSingleStep(1);
	ui->spinbox_video_bitrate->setMinimum(g_cfg_recording.video.video_bps.min);
	ui->spinbox_video_bitrate->setMaximum(g_cfg_recording.video.video_bps.max);
	connect(ui->spinbox_video_bitrate, &QSpinBox::valueChanged, this, [this](int value)
	{
		g_cfg_recording.video.video_bps.set(value);
		update_preset();
	});

	ui->spinbox_audio_bitrate->setSingleStep(1);
	ui->spinbox_audio_bitrate->setMinimum(g_cfg_recording.audio.audio_bps.min);
	ui->spinbox_audio_bitrate->setMaximum(g_cfg_recording.audio.audio_bps.max);
	connect(ui->spinbox_audio_bitrate, &QSpinBox::valueChanged, this, [this](int value)
	{
		g_cfg_recording.audio.audio_bps.set(value);
		update_preset();
	});

	ui->spinbox_gop_size->setSingleStep(1);
	ui->spinbox_gop_size->setMinimum(g_cfg_recording.video.gop_size.min);
	ui->spinbox_gop_size->setMaximum(g_cfg_recording.video.gop_size.max);
	connect(ui->spinbox_gop_size, &QSpinBox::valueChanged, this, [this](int value)
	{
		g_cfg_recording.video.gop_size.set(value);
		update_preset();
	});

	ui->spinbox_max_b_frames->setSingleStep(1);
	ui->spinbox_max_b_frames->setMinimum(g_cfg_recording.video.max_b_frames.min);
	ui->spinbox_max_b_frames->setMaximum(g_cfg_recording.video.max_b_frames.max);
	connect(ui->spinbox_max_b_frames, &QSpinBox::valueChanged, this, [this](int value)
	{
		g_cfg_recording.video.max_b_frames.set(value);
		update_preset();
	});

	connect(ui->buttonBox, &QDialogButtonBox::clicked, [this](QAbstractButton* button)
	{
		if (button == ui->buttonBox->button(QDialogButtonBox::Save))
		{
			g_cfg_recording.save();
			accept();
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::Cancel))
		{
			reject();
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults))
		{
			g_cfg_recording.from_default();
			update_ui();
			update_preset();
		}
	});

	connect(this, &QDialog::rejected, this, []()
	{
		if (!g_cfg_recording.load())
		{
			cfg_log.notice("Could not load recording config. Using defaults.");
		}
	});

	update_ui();
	update_preset();
}

recording_settings_dialog::~recording_settings_dialog()
{
}

void recording_settings_dialog::update_preset()
{
	const quality_preset preset = current_preset();
	ui->combo_presets->setCurrentIndex(ui->combo_presets->findData(static_cast<int>(preset)));
}

void recording_settings_dialog::update_ui()
{
	ui->combo_resolution->blockSignals(true);
	ui->combo_framerate->blockSignals(true);
	ui->combo_video_codec->blockSignals(true);
	ui->combo_audio_codec->blockSignals(true);
	ui->spinbox_video_bitrate->blockSignals(true);
	ui->spinbox_audio_bitrate->blockSignals(true);
	ui->spinbox_gop_size->blockSignals(true);
	ui->spinbox_max_b_frames->blockSignals(true);

	ui->combo_resolution->setCurrentIndex(ui->combo_resolution->findData(QVariant::fromValue(QPair<int, int>(g_cfg_recording.video.width.get(), g_cfg_recording.video.height.get()))));
	ui->combo_framerate->setCurrentIndex(ui->combo_framerate->findData(static_cast<int>(g_cfg_recording.video.framerate.get())));
	ui->combo_video_codec->setCurrentIndex(ui->combo_video_codec->findData(static_cast<int>(g_cfg_recording.video.video_codec.get())));
	ui->combo_audio_codec->setCurrentIndex(ui->combo_audio_codec->findData(static_cast<int>(g_cfg_recording.audio.audio_codec.get())));
	ui->spinbox_video_bitrate->setValue(g_cfg_recording.video.video_bps);
	ui->spinbox_audio_bitrate->setValue(g_cfg_recording.audio.audio_bps);
	ui->spinbox_gop_size->setValue(g_cfg_recording.video.gop_size);
	ui->spinbox_max_b_frames->setValue(g_cfg_recording.video.max_b_frames);

	ui->combo_resolution->blockSignals(false);
	ui->combo_framerate->blockSignals(false);
	ui->combo_video_codec->blockSignals(false);
	ui->combo_audio_codec->blockSignals(false);
	ui->spinbox_video_bitrate->blockSignals(false);
	ui->spinbox_audio_bitrate->blockSignals(false);
	ui->spinbox_gop_size->blockSignals(false);
	ui->spinbox_max_b_frames->blockSignals(false);

	const auto get_codec_name = [](const std::vector<const AVCodec*>& codecs, u32 id)
	{
		for (const AVCodec* codec : codecs)
		{
			if (codec && codec->id == static_cast<AVCodecID>(id))
			{
				const std::string name = codec->long_name ? codec->long_name : avcodec_get_name(codec->id);
				return name;
			}
		}
		return std::string();
	};

	ui->label_info_keys->setText(
		tr("Resolution:") + "\n" +
		tr("Framerate:") + "\n" +
		tr("Video Codec:") + "\n" +
		tr("Video Bitrate:") + "\n" +
		tr("Audio Codec:") + "\n" +
		tr("Audio Bitrate:") + "\n" +
		tr("Gop-Size:") + "\n" +
		tr("Max B-Frames:")
	);

	ui->label_info_values->setText(QString::fromStdString(
		fmt::format("%d x %d\n%d fps\n%s\n%d\n%s\n%d\n%d\n%d",
			g_cfg_recording.video.width.get(), g_cfg_recording.video.height.get(),
			g_cfg_recording.video.framerate.get(),
			get_codec_name(m_video_codecs, g_cfg_recording.video.video_codec.get()),
			g_cfg_recording.video.video_bps.get(),
			get_codec_name(m_audio_codecs, g_cfg_recording.audio.audio_codec.get()),
			g_cfg_recording.audio.audio_bps.get(),
			g_cfg_recording.video.gop_size.get(),
			g_cfg_recording.video.max_b_frames.get()
		)
	));
}

void recording_settings_dialog::select_preset(quality_preset preset, cfg_recording& cfg)
{
	if (preset == quality_preset::custom)
	{
		return;
	}

	cfg.audio.audio_codec.set(static_cast<u32>(AVCodecID::AV_CODEC_ID_AAC));
	cfg.audio.audio_bps.set(192'000); // 192 kbps

	cfg.video.video_codec.set(static_cast<u32>(AVCodecID::AV_CODEC_ID_MPEG4));
	cfg.video.pixel_format.set(static_cast<u32>(::AV_PIX_FMT_YUV420P));

	switch (preset)
	{
	case quality_preset::_720p_30:
	case quality_preset::_720p_60:
		cfg.video.width.set(1280);
		cfg.video.height.set(720);
		break;
	case quality_preset::_1080p_30:
	case quality_preset::_1080p_60:
		cfg.video.width.set(1920);
		cfg.video.height.set(1080);
		break;
	case quality_preset::_1440p_30:
	case quality_preset::_1440p_60:
		cfg.video.width.set(2560);
		cfg.video.height.set(1440);
		break;
	case quality_preset::_2160p_30:
	case quality_preset::_2160p_60:
		cfg.video.width.set(3840);
		cfg.video.height.set(2160);
		break;
	case quality_preset::custom:
		break;
	}

	switch (preset)
	{
	case quality_preset::_720p_30:
	case quality_preset::_1080p_30:
	case quality_preset::_1440p_30:
	case quality_preset::_2160p_30:
		cfg.video.framerate.set(30);
		break;
	case quality_preset::_720p_60:
	case quality_preset::_1080p_60:
	case quality_preset::_1440p_60:
	case quality_preset::_2160p_60:
		cfg.video.framerate.set(60);
		break;
	case quality_preset::custom:
		break;
	}

	switch (preset)
	{
	case quality_preset::_720p_30:
		cfg.video.video_bps.set(4'000'000);
		break;
	case quality_preset::_720p_60:
		cfg.video.video_bps.set(6'000'000);
		break;
	case quality_preset::_1080p_30:
		cfg.video.video_bps.set(8'000'000);
		break;
	case quality_preset::_1080p_60:
		cfg.video.video_bps.set(12'000'000);
		break;
	case quality_preset::_1440p_30:
		cfg.video.video_bps.set(16'000'000);
		break;
	case quality_preset::_1440p_60:
		cfg.video.video_bps.set(24'000'000);
		break;
	case quality_preset::_2160p_30:
		cfg.video.video_bps.set(40'000'000);
		break;
	case quality_preset::_2160p_60:
		cfg.video.video_bps.set(60'000'000);
		break;
	case quality_preset::custom:
		break;
	}

	cfg.video.gop_size.set(cfg.video.framerate.get());
	cfg.video.max_b_frames.set(2);
}

recording_settings_dialog::quality_preset recording_settings_dialog::current_preset()
{
	for (u32 i = 0; i < static_cast<u32>(quality_preset::custom); i++)
	{
		const quality_preset preset = static_cast<quality_preset>(i);

		cfg_recording cfg;
		select_preset(preset, cfg);

		if (g_cfg_recording.video.framerate.get() == cfg.video.framerate.get() &&
			g_cfg_recording.video.width.get() == cfg.video.width.get() &&
			g_cfg_recording.video.height.get() == cfg.video.height.get() &&
			g_cfg_recording.video.pixel_format.get() == cfg.video.pixel_format.get() &&
			g_cfg_recording.video.video_codec.get() == cfg.video.video_codec.get() &&
			g_cfg_recording.video.video_bps.get() == cfg.video.video_bps.get() &&
			g_cfg_recording.video.max_b_frames.get() == cfg.video.max_b_frames.get() &&
			g_cfg_recording.video.gop_size.get() == cfg.video.gop_size.get() &&
			g_cfg_recording.audio.audio_codec.get() == cfg.audio.audio_codec.get() &&
			g_cfg_recording.audio.audio_bps.get() == cfg.audio.audio_bps.get())
		{
			return preset;
		}
	}

	return quality_preset::custom;
}
