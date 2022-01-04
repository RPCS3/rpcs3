#include "stdafx.h"
#include "camera_settings_dialog.h"
#include "ui_camera_settings_dialog.h"
#include "Emu/Io/camera_config.h"

#include <QCameraInfo>
#include <QMessageBox>
#include <QPushButton>

LOG_CHANNEL(camera_log, "Camera");

template <>
void fmt_class_string<QVideoFrame::PixelFormat>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](QVideoFrame::PixelFormat value)
	{
		switch (value)
		{
		case QVideoFrame::Format_Invalid: return "Invalid";
		case QVideoFrame::Format_ARGB32: return "ARGB32";
		case QVideoFrame::Format_ARGB32_Premultiplied: return "ARGB32_Premultiplied";
		case QVideoFrame::Format_RGB32: return "RGB32";
		case QVideoFrame::Format_RGB24: return "RGB24";
		case QVideoFrame::Format_RGB565: return "RGB565";
		case QVideoFrame::Format_RGB555: return "RGB555";
		case QVideoFrame::Format_ARGB8565_Premultiplied: return "ARGB8565_Premultiplied";
		case QVideoFrame::Format_BGRA32: return "BGRA32";
		case QVideoFrame::Format_BGRA32_Premultiplied: return "BGRA32_Premultiplied";
		case QVideoFrame::Format_BGR32: return "BGR32";
		case QVideoFrame::Format_BGR24: return "BGR24";
		case QVideoFrame::Format_BGR565: return "BGR565";
		case QVideoFrame::Format_BGR555: return "BGR555";
		case QVideoFrame::Format_BGRA5658_Premultiplied: return "BGRA5658_Premultiplied";
		case QVideoFrame::Format_AYUV444: return "AYUV444";
		case QVideoFrame::Format_AYUV444_Premultiplied: return "AYUV444_Premultiplied";
		case QVideoFrame::Format_YUV444: return "YUV444";
		case QVideoFrame::Format_YUV420P: return "YUV420P";
		case QVideoFrame::Format_YV12: return "YV12";
		case QVideoFrame::Format_UYVY: return "UYVY";
		case QVideoFrame::Format_YUYV: return "YUYV";
		case QVideoFrame::Format_NV12: return "NV12";
		case QVideoFrame::Format_NV21: return "NV21";
		case QVideoFrame::Format_IMC1: return "IMC1";
		case QVideoFrame::Format_IMC2: return "IMC2";
		case QVideoFrame::Format_IMC3: return "IMC3";
		case QVideoFrame::Format_IMC4: return "IMC4";
		case QVideoFrame::Format_Y8: return "Y8";
		case QVideoFrame::Format_Y16: return "Y16";
		case QVideoFrame::Format_Jpeg: return "Jpeg";
		case QVideoFrame::Format_CameraRaw: return "CameraRaw";
		case QVideoFrame::Format_AdobeDng: return "AdobeDng";
		case QVideoFrame::Format_ABGR32: return "ABGR32";
		case QVideoFrame::Format_YUV422P: return "YUV422P";
		case QVideoFrame::Format_User: return "User";
		}

		return unknown;
	});
}

Q_DECLARE_METATYPE(QCameraInfo);

camera_settings_dialog::camera_settings_dialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::camera_settings_dialog)
{
	ui->setupUi(this);

	load_config();

	for (const QCameraInfo& camera_info : QCameraInfo::availableCameras())
	{
		if (camera_info.isNull()) continue;
		ui->combo_camera->addItem(camera_info.description(), QVariant::fromValue(camera_info));
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
	if (index < 0 || !ui->combo_camera->itemData(index).canConvert<QCameraInfo>())
	{
		ui->combo_settings->clear();
		return;
	}

	const QCameraInfo camera_info = ui->combo_camera->itemData(index).value<QCameraInfo>();

	if (camera_info.isNull())
	{
		ui->combo_settings->clear();
		return;
	}

	m_camera.reset(new QCamera(camera_info));
	m_camera->setViewfinder(ui->viewfinder);

	if (!m_camera->isAvailable())
	{
		ui->combo_settings->clear();
		QMessageBox::warning(this, tr("Camera not available"), tr("The selected camera is not available.\nIt might be blocked by another application."));
		return;
	}

	m_camera->load();

	ui->combo_settings->blockSignals(true);
	ui->combo_settings->clear();

	QList<QCameraViewfinderSettings> settings = m_camera->supportedViewfinderSettings();
	std::sort(settings.begin(), settings.end(), [](const QCameraViewfinderSettings& l, const QCameraViewfinderSettings& r) -> bool
	{
		if (l.resolution().width() > r.resolution().width()) return true;
		if (l.resolution().width() < r.resolution().width()) return false;
		if (l.resolution().height() > r.resolution().height()) return true;
		if (l.resolution().height() < r.resolution().height()) return false;
		if (l.minimumFrameRate() > r.minimumFrameRate()) return true;
		if (l.minimumFrameRate() < r.minimumFrameRate()) return false;
		if (l.maximumFrameRate() > r.maximumFrameRate()) return true;
		if (l.maximumFrameRate() < r.maximumFrameRate()) return false;
		if (l.pixelFormat() > r.pixelFormat()) return true;
		if (l.pixelFormat() < r.pixelFormat()) return false;
		if (l.pixelAspectRatio().width() > r.pixelAspectRatio().width()) return true;
		if (l.pixelAspectRatio().width() < r.pixelAspectRatio().width()) return false;
		if (l.pixelAspectRatio().height() > r.pixelAspectRatio().height()) return true;
		if (l.pixelAspectRatio().height() < r.pixelAspectRatio().height()) return false;
		return false;
	});

	for (const QCameraViewfinderSettings& setting : settings)
	{
		if (setting.isNull()) continue;
		const QString description = tr("%0x%1, %2-%3 FPS, Format=%4, PixelAspectRatio=%5x%6")
			.arg(setting.resolution().width())
			.arg(setting.resolution().height())
			.arg(setting.minimumFrameRate())
			.arg(setting.maximumFrameRate())
			.arg(QString::fromStdString(fmt::format("%s", setting.pixelFormat())))
			.arg(setting.pixelAspectRatio().width())
			.arg(setting.pixelAspectRatio().height());
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
		const std::string key = camera_info.deviceName().toStdString();
		cfg_camera::camera_setting cfg_setting = g_cfg_camera.get_camera_setting(key, success);

		if (success)
		{
			camera_log.notice("Found config entry for camera \"%s\"", key);

			// Convert to Qt data
			QCameraViewfinderSettings setting;
			setting.setResolution(cfg_setting.width, cfg_setting.height);
			setting.setMinimumFrameRate(cfg_setting.min_fps);
			setting.setMaximumFrameRate(cfg_setting.max_fps);
			setting.setPixelFormat(static_cast<QVideoFrame::PixelFormat>(cfg_setting.format));
			setting.setPixelAspectRatio(cfg_setting.pixel_aspect_width, cfg_setting.pixel_aspect_height);

			// Select matching drowdown entry
			const double epsilon = 0.001;

			for (int i = 0; i < ui->combo_settings->count(); i++)
			{
				const QCameraViewfinderSettings tmp = ui->combo_settings->itemData(i).value<QCameraViewfinderSettings>();

				if (tmp.resolution().width() == setting.resolution().width() &&
					tmp.resolution().height() == setting.resolution().height() &&
					tmp.minimumFrameRate() >= (setting.minimumFrameRate() - epsilon) &&
					tmp.minimumFrameRate() <= (setting.minimumFrameRate() + epsilon) &&
					tmp.maximumFrameRate() >= (setting.maximumFrameRate() - epsilon) &&
					tmp.maximumFrameRate() <= (setting.maximumFrameRate() + epsilon) &&
					tmp.pixelFormat() == setting.pixelFormat() &&
					tmp.pixelAspectRatio().width() == setting.pixelAspectRatio().width() &&
					tmp.pixelAspectRatio().height() == setting.pixelAspectRatio().height())
				{
					index = i;
					break;
				}
			}
		}

		ui->combo_settings->setCurrentIndex(std::max<int>(0, index));
		ui->combo_settings->setEnabled(true);

		// Update config to match user interface outcome
		const QCameraViewfinderSettings setting = ui->combo_settings->currentData().value<QCameraViewfinderSettings>();
		cfg_setting.width = setting.resolution().width();
		cfg_setting.height = setting.resolution().height();
		cfg_setting.min_fps = setting.minimumFrameRate();
		cfg_setting.max_fps = setting.maximumFrameRate();
		cfg_setting.format = static_cast<int>(setting.pixelFormat());
		cfg_setting.pixel_aspect_width = setting.pixelAspectRatio().width();
		cfg_setting.pixel_aspect_height = setting.pixelAspectRatio().height();
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

	if (index >= 0 && ui->combo_settings->itemData(index).canConvert<QCameraViewfinderSettings>() && ui->combo_camera->currentData().canConvert<QCameraInfo>())
	{
		const QCameraViewfinderSettings setting = ui->combo_settings->itemData(index).value<QCameraViewfinderSettings>();
		if (!setting.isNull())
		{
			m_camera->setViewfinderSettings(setting);
		}

		cfg_camera::camera_setting cfg_setting;
		cfg_setting.width = setting.resolution().width();
		cfg_setting.height = setting.resolution().height();
		cfg_setting.min_fps = setting.minimumFrameRate();
		cfg_setting.max_fps = setting.maximumFrameRate();
		cfg_setting.format = static_cast<int>(setting.pixelFormat());
		cfg_setting.pixel_aspect_width = setting.pixelAspectRatio().width();
		cfg_setting.pixel_aspect_height = setting.pixelAspectRatio().height();
		g_cfg_camera.set_camera_setting(ui->combo_camera->currentData().value<QCameraInfo>().deviceName().toStdString(), cfg_setting);
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
