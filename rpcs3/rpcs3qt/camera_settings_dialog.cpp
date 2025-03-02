#include "stdafx.h"
#include "camera_settings_dialog.h"
#include "ui_camera_settings_dialog.h"
#include "permissions.h"
#include "Emu/Io/camera_config.h"

#include <QCameraDevice>
#include <QMediaDevices>
#include <QMessageBox>
#include <QPushButton>

LOG_CHANNEL(camera_log, "Camera");

template <>
void fmt_class_string<QVideoFrameFormat::PixelFormat>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](QVideoFrameFormat::PixelFormat value)
	{
		switch (value)
		{
		case QVideoFrameFormat::Format_ARGB8888: return "ARGB8888";
		case QVideoFrameFormat::Format_ARGB8888_Premultiplied: return "ARGB8888_Premultiplied";
		case QVideoFrameFormat::Format_XRGB8888: return "XRGB8888";
		case QVideoFrameFormat::Format_BGRA8888: return "BGRA8888";
		case QVideoFrameFormat::Format_BGRA8888_Premultiplied: return "BGRA8888_Premultiplied";
		case QVideoFrameFormat::Format_BGRX8888: return "BGRX8888";
		case QVideoFrameFormat::Format_ABGR8888: return "ABGR8888";
		case QVideoFrameFormat::Format_XBGR8888: return "XBGR8888";
		case QVideoFrameFormat::Format_RGBA8888: return "RGBA8888";
		case QVideoFrameFormat::Format_RGBX8888: return "RGBX8888";
		case QVideoFrameFormat::Format_AYUV: return "AYUV";
		case QVideoFrameFormat::Format_AYUV_Premultiplied: return "AYUV_Premultiplied";
		case QVideoFrameFormat::Format_YUV420P: return "YUV420P";
		case QVideoFrameFormat::Format_YUV422P: return "YUV422P";
		case QVideoFrameFormat::Format_YV12: return "YV12";
		case QVideoFrameFormat::Format_UYVY: return "UYVY";
		case QVideoFrameFormat::Format_YUYV: return "YUYV";
		case QVideoFrameFormat::Format_NV12: return "NV12";
		case QVideoFrameFormat::Format_NV21: return "NV21";
		case QVideoFrameFormat::Format_IMC1: return "IMC1";
		case QVideoFrameFormat::Format_IMC2: return "IMC2";
		case QVideoFrameFormat::Format_IMC3: return "IMC3";
		case QVideoFrameFormat::Format_IMC4: return "IMC4";
		case QVideoFrameFormat::Format_Y8: return "Y8";
		case QVideoFrameFormat::Format_Y16: return "Y16";
		case QVideoFrameFormat::Format_P010: return "P010";
		case QVideoFrameFormat::Format_P016: return "P016";
		case QVideoFrameFormat::Format_SamplerExternalOES: return "SamplerExternalOES";
		case QVideoFrameFormat::Format_Jpeg: return "Jpeg";
		case QVideoFrameFormat::Format_SamplerRect: return "SamplerRect";
		default: return unknown;
		}
	});
}

Q_DECLARE_METATYPE(QCameraDevice);

camera_settings_dialog::camera_settings_dialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::camera_settings_dialog)
{
	ui->setupUi(this);

	load_config();

	for (const QCameraDevice& camera_info : QMediaDevices::videoInputs())
	{
		if (camera_info.isNull()) continue;
		ui->combo_camera->addItem(camera_info.description(), QVariant::fromValue(camera_info));
		camera_log.notice("Found camera: '%s'", camera_info.description());
	}

	connect(ui->combo_camera, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &camera_settings_dialog::handle_camera_change);
	connect(ui->combo_settings, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &camera_settings_dialog::handle_settings_change);
	connect(ui->buttonBox, &QDialogButtonBox::clicked, [this](QAbstractButton* button)
	{
		if (button == ui->buttonBox->button(QDialogButtonBox::Save))
		{
			save_config();
			accept();
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
		{
			save_config();
		}
	});

	if (ui->combo_camera->count() == 0)
	{
		ui->combo_camera->setEnabled(false);
		ui->combo_settings->setEnabled(false);
		ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
		ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	}
	else
	{
		// TODO: show camera ID somewhere
		ui->combo_camera->setCurrentIndex(0);
	}
}

camera_settings_dialog::~camera_settings_dialog()
{
}

void camera_settings_dialog::handle_camera_change(int index)
{
	if (index < 0 || !ui->combo_camera->itemData(index).canConvert<QCameraDevice>())
	{
		ui->combo_settings->clear();
		return;
	}

	const QCameraDevice camera_info = ui->combo_camera->itemData(index).value<QCameraDevice>();

	if (camera_info.isNull())
	{
		ui->combo_settings->clear();
		return;
	}

	m_camera.reset(new QCamera(camera_info));
	m_media_capture_session.reset(new QMediaCaptureSession(nullptr));
	m_media_capture_session->setCamera(m_camera.get());
	m_media_capture_session->setVideoSink(ui->videoWidget->videoSink());

	if (!m_camera->isAvailable())
	{
		ui->combo_settings->clear();
		QMessageBox::warning(this, tr("Camera not available"), tr("The selected camera is not available.\nIt might be blocked by another application."));
		return;
	}

	ui->combo_settings->blockSignals(true);
	ui->combo_settings->clear();

	QList<QCameraFormat> settings = camera_info.videoFormats();
	std::sort(settings.begin(), settings.end(), [](const QCameraFormat& l, const QCameraFormat& r) -> bool
	{
		if (l.resolution().width() > r.resolution().width()) return true;
		if (l.resolution().width() < r.resolution().width()) return false;
		if (l.resolution().height() > r.resolution().height()) return true;
		if (l.resolution().height() < r.resolution().height()) return false;
		if (l.minFrameRate() > r.minFrameRate()) return true;
		if (l.minFrameRate() < r.minFrameRate()) return false;
		if (l.maxFrameRate() > r.maxFrameRate()) return true;
		if (l.maxFrameRate() < r.maxFrameRate()) return false;
		if (l.pixelFormat() > r.pixelFormat()) return true;
		if (l.pixelFormat() < r.pixelFormat()) return false;
		return false;
	});

	for (const QCameraFormat& setting : settings)
	{
		if (setting.isNull()) continue;
		const QString description = tr("%0x%1, %2-%3 FPS, Format=%4")
			.arg(setting.resolution().width())
			.arg(setting.resolution().height())
			.arg(setting.minFrameRate())
			.arg(setting.maxFrameRate())
			.arg(QString::fromStdString(fmt::format("%s", setting.pixelFormat())));
		ui->combo_settings->addItem(description, QVariant::fromValue(setting));
	}
	ui->combo_settings->blockSignals(false);

	if (ui->combo_settings->count() == 0)
	{
		ui->combo_settings->setEnabled(false);
	}
	else
	{
		// Load selected settings from config file
		int index = 0;
		bool success = false;
		const std::string key = camera_info.id().toStdString();
		cfg_camera::camera_setting cfg_setting = g_cfg_camera.get_camera_setting(key, success);

		if (success)
		{
			camera_log.notice("Found config entry for camera \"%s\"", key);

			// Select matching drowdown entry
			const double epsilon = 0.001;

			for (int i = 0; i < ui->combo_settings->count(); i++)
			{
				const QCameraFormat tmp = ui->combo_settings->itemData(i).value<QCameraFormat>();

				if (tmp.resolution().width() == cfg_setting.width &&
					tmp.resolution().height() == cfg_setting.height &&
					tmp.minFrameRate() >= (cfg_setting.min_fps - epsilon) &&
					tmp.minFrameRate() <= (cfg_setting.min_fps + epsilon) &&
					tmp.maxFrameRate() >= (cfg_setting.max_fps - epsilon) &&
					tmp.maxFrameRate() <= (cfg_setting.max_fps + epsilon) &&
					tmp.pixelFormat() == static_cast<QVideoFrameFormat::PixelFormat>(cfg_setting.format))
				{
					index = i;
					break;
				}
			}
		}

		ui->combo_settings->setCurrentIndex(std::max<int>(0, index));
		ui->combo_settings->setEnabled(true);

		// Update config to match user interface outcome
		const QCameraFormat setting = ui->combo_settings->currentData().value<QCameraFormat>();
		cfg_setting.width = setting.resolution().width();
		cfg_setting.height = setting.resolution().height();
		cfg_setting.min_fps = setting.minFrameRate();
		cfg_setting.max_fps = setting.maxFrameRate();
		cfg_setting.format = static_cast<int>(setting.pixelFormat());
		g_cfg_camera.set_camera_setting(key, cfg_setting);
	}
}

void camera_settings_dialog::handle_settings_change(int index)
{
	if (!m_camera)
	{
		return;
	}

	if (!m_camera->isAvailable())
	{
		QMessageBox::warning(this, tr("Camera not available"), tr("The selected camera is not available.\nIt might be blocked by another application."));
		return;
	}

	if (!gui::utils::check_camera_permission(this,
		[this, index](){ handle_settings_change(index); },
		[this](){ QMessageBox::warning(this, tr("Camera permissions denied!"), tr("RPCS3 has no permissions to access cameras on this device.")); }))
	{
		return;
	}

	if (index >= 0 && ui->combo_settings->itemData(index).canConvert<QCameraFormat>() && ui->combo_camera->currentData().canConvert<QCameraDevice>())
	{
		const QCameraFormat setting = ui->combo_settings->itemData(index).value<QCameraFormat>();
		if (!setting.isNull())
		{
			m_camera->setCameraFormat(setting);
		}

		cfg_camera::camera_setting cfg_setting;
		cfg_setting.width = setting.resolution().width();
		cfg_setting.height = setting.resolution().height();
		cfg_setting.min_fps = setting.minFrameRate();
		cfg_setting.max_fps = setting.maxFrameRate();
		cfg_setting.format = static_cast<int>(setting.pixelFormat());
		g_cfg_camera.set_camera_setting(ui->combo_camera->currentData().value<QCameraDevice>().id().toStdString(), cfg_setting);
	}

	m_camera->start();
}

void camera_settings_dialog::load_config()
{
	if (!g_cfg_camera.load())
	{
		camera_log.notice("Could not load camera config. Using defaults.");
	}
}

void camera_settings_dialog::save_config()
{
	g_cfg_camera.save();
}
