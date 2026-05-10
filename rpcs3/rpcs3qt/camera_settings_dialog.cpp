#include "stdafx.h"
#include "camera_settings_dialog.h"
#include "ui_camera_settings_dialog.h"
#include "permissions.h"
#include "Emu/Io/camera_config.h"
#include "Emu/System.h"
#include "Emu/system_config.h"

#include <QCameraDevice>
#include <QMediaDevices>
#include <QMessageBox>
#include <QPushButton>
#include <QVideoSink>

#ifdef HAVE_SDL3
#include "Input/sdl_instance.h"
#include "Input/sdl_camera_handler.h"
#endif

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

#ifdef HAVE_SDL3
static QString sdl_pixelformat_to_string(SDL_PixelFormat format)
{
	switch (format)
	{
	case SDL_PixelFormat::SDL_PIXELFORMAT_UNKNOWN: return "UNKNOWN";
	case SDL_PixelFormat::SDL_PIXELFORMAT_INDEX1LSB: return "INDEX1LSB";
	case SDL_PixelFormat::SDL_PIXELFORMAT_INDEX1MSB: return "INDEX1MSB";
	case SDL_PixelFormat::SDL_PIXELFORMAT_INDEX2LSB: return "INDEX2LSB";
	case SDL_PixelFormat::SDL_PIXELFORMAT_INDEX2MSB: return "INDEX2MSB";
	case SDL_PixelFormat::SDL_PIXELFORMAT_INDEX4LSB: return "INDEX4LSB";
	case SDL_PixelFormat::SDL_PIXELFORMAT_INDEX4MSB: return "INDEX4MSB";
	case SDL_PixelFormat::SDL_PIXELFORMAT_INDEX8: return "INDEX8";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGB332: return "RGB332";
	case SDL_PixelFormat::SDL_PIXELFORMAT_XRGB4444: return "XRGB4444";
	case SDL_PixelFormat::SDL_PIXELFORMAT_XBGR4444: return "XBGR4444";
	case SDL_PixelFormat::SDL_PIXELFORMAT_XRGB1555: return "XRGB1555";
	case SDL_PixelFormat::SDL_PIXELFORMAT_XBGR1555: return "XBGR1555";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ARGB4444: return "ARGB4444";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGBA4444: return "RGBA4444";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ABGR4444: return "ABGR4444";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGRA4444: return "BGRA4444";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ARGB1555: return "ARGB1555";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGBA5551: return "RGBA5551";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ABGR1555: return "ABGR1555";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGRA5551: return "BGRA5551";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGB565: return "RGB565";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGR565: return "BGR565";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGB24: return "RGB24";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGR24: return "BGR24";
	case SDL_PixelFormat::SDL_PIXELFORMAT_XRGB8888: return "XRGB8888";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGBX8888: return "RGBX8888";
	case SDL_PixelFormat::SDL_PIXELFORMAT_XBGR8888: return "XBGR8888";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGRX8888: return "BGRX8888";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ARGB8888: return "ARGB8888";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGBA8888: return "RGBA8888";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ABGR8888: return "ABGR8888";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGRA8888: return "BGRA8888";
	case SDL_PixelFormat::SDL_PIXELFORMAT_XRGB2101010: return "XRGB2101010";
	case SDL_PixelFormat::SDL_PIXELFORMAT_XBGR2101010: return "XBGR2101010";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ARGB2101010: return "ARGB2101010";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ABGR2101010: return "ABGR2101010";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGB48: return "RGB48";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGR48: return "BGR48";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGBA64: return "RGBA64";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ARGB64: return "ARGB64";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGRA64: return "BGRA64";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ABGR64: return "ABGR64";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGB48_FLOAT: return "RGB48_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGR48_FLOAT: return "BGR48_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGBA64_FLOAT: return "RGBA64_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ARGB64_FLOAT: return "ARGB64_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGRA64_FLOAT: return "BGRA64_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ABGR64_FLOAT: return "ABGR64_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGB96_FLOAT: return "RGB96_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGR96_FLOAT: return "BGR96_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_RGBA128_FLOAT: return "RGBA128_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ARGB128_FLOAT: return "ARGB128_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_BGRA128_FLOAT: return "BGRA128_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_ABGR128_FLOAT: return "ABGR128_FLOAT";
	case SDL_PixelFormat::SDL_PIXELFORMAT_YV12: return "YV12";
	case SDL_PixelFormat::SDL_PIXELFORMAT_IYUV: return "IYUV";
	case SDL_PixelFormat::SDL_PIXELFORMAT_YUY2: return "YUY2";
	case SDL_PixelFormat::SDL_PIXELFORMAT_UYVY: return "UYVY";
	case SDL_PixelFormat::SDL_PIXELFORMAT_YVYU: return "YVYU";
	case SDL_PixelFormat::SDL_PIXELFORMAT_NV12: return "NV12";
	case SDL_PixelFormat::SDL_PIXELFORMAT_NV21: return "NV21";
	case SDL_PixelFormat::SDL_PIXELFORMAT_P010: return "P010";
	case SDL_PixelFormat::SDL_PIXELFORMAT_EXTERNAL_OES: return "EXTERNAL_OES";
	case SDL_PixelFormat::SDL_PIXELFORMAT_MJPG: return "MJPG";
	default: return QObject::tr("Unknown: %0").arg(static_cast<int>(format));
	}
}
#endif

Q_DECLARE_METATYPE(QCameraDevice);

camera_settings_dialog::camera_settings_dialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::camera_settings_dialog)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	load_config();

	ui->combo_handlers->addItem("Qt", QVariant::fromValue(static_cast<int>(camera_handler::qt)));
#ifdef HAVE_SDL3
	ui->combo_handlers->addItem("SDL", QVariant::fromValue(static_cast<int>(camera_handler::sdl)));
#endif

	connect(ui->combo_handlers, &QComboBox::currentIndexChanged, this, &camera_settings_dialog::handle_handler_change);
	connect(ui->combo_camera, &QComboBox::currentIndexChanged, this, &camera_settings_dialog::handle_camera_change);
	connect(ui->combo_settings, &QComboBox::currentIndexChanged, this, &camera_settings_dialog::handle_settings_change);
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

	const int handler_index = ui->combo_handlers->findData(static_cast<int>(g_cfg.io.camera.get()));
	ui->combo_handlers->setCurrentIndex(std::max(0, handler_index));
}

camera_settings_dialog::~camera_settings_dialog()
{
	reset_cameras();
}

void camera_settings_dialog::enable_combos()
{
	const bool is_enabled = ui->combo_camera->count() > 0;

	ui->combo_camera->setEnabled(is_enabled);
	ui->combo_settings->setEnabled(is_enabled);
	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(is_enabled);
	ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(is_enabled);

	if (is_enabled)
	{
		// TODO: show camera ID somewhere
		ui->combo_camera->setCurrentIndex(0);
	}
}

void camera_settings_dialog::reset_cameras()
{
	m_media_capture_session.reset();
	m_camera.reset();

#ifdef HAVE_SDL3
	m_video_frame_input.reset();

	if (m_sdl_thread)
	{
		auto& thread = *m_sdl_thread;
		thread = thread_state::aborting;
		thread();
		m_sdl_thread.reset();
	}

	if (m_sdl_camera)
	{
		SDL_CloseCamera(m_sdl_camera);
		m_sdl_camera = nullptr;
	}
#endif
}

void camera_settings_dialog::handle_handler_change(int index)
{
	reset_cameras();

	if (index < 0 || !ui->combo_handlers->itemData(index).canConvert<int>())
	{
		ui->combo_settings->clear();
		ui->combo_camera->clear();
		enable_combos();
		return;
	}

	m_handler = static_cast<camera_handler>(ui->combo_handlers->itemData(index).value<int>());

	ui->combo_settings->blockSignals(true);
	ui->combo_camera->blockSignals(true);

	ui->combo_settings->clear();
	ui->combo_camera->clear();

	switch (m_handler)
	{
	case camera_handler::qt:
	{
		for (const QCameraDevice& camera_info : QMediaDevices::videoInputs())
		{
			if (camera_info.isNull()) continue;
			ui->combo_camera->addItem(camera_info.description(), QVariant::fromValue(camera_info));
			camera_log.notice("Found camera: '%s'", camera_info.description());
		}
		break;
	}
#ifdef HAVE_SDL3
	case camera_handler::sdl:
	{
		if (!sdl_instance::get_instance().initialize())
		{
			camera_log.error("Could not initialize SDL");
			break;
		}

		// Log camera drivers
		sdl_camera_handler::get_drivers();

		// Get cameras
		const std::map<SDL_CameraID, std::string> cameras = sdl_camera_handler::get_cameras();

		// Add cameras
		for (const auto& [camera_id, name] : cameras)
		{
			ui->combo_camera->addItem(QString::fromStdString(name), QVariant::fromValue(static_cast<u32>(camera_id)));
		}
		break;
	}
#endif
	default:
		fmt::throw_exception("Unexpected camera handler %d", static_cast<int>(m_handler));
	}

	ui->combo_settings->blockSignals(false);
	ui->combo_camera->blockSignals(false);

	enable_combos();
}

void camera_settings_dialog::handle_camera_change(int index)
{
	if (index < 0)
	{
		ui->combo_settings->clear();
		return;
	}

	reset_cameras();

	switch (m_handler)
	{
	case camera_handler::qt:
		handle_qt_camera_change(ui->combo_camera->itemData(index));
		break;
#ifdef HAVE_SDL3
	case camera_handler::sdl:
		handle_sdl_camera_change(ui->combo_camera->itemText(index), ui->combo_camera->itemData(index));
		break;
#endif
	default:
		fmt::throw_exception("Unexpected camera handler %d", static_cast<int>(m_handler));
	}
}

void camera_settings_dialog::handle_settings_change(int index)
{
	if (index < 0)
	{
		return;
	}

	if (!gui::utils::check_camera_permission(this,
		[this, index](){ handle_settings_change(index); },
		[this](){ QMessageBox::warning(this, tr("Camera permissions denied!"), tr("RPCS3 has no permissions to access cameras on this device.")); }))
	{
		return;
	}

	switch (m_handler)
	{
	case camera_handler::qt:
		handle_qt_settings_change(ui->combo_settings->itemData(index));
		break;
#ifdef HAVE_SDL3
	case camera_handler::sdl:
		handle_sdl_settings_change(ui->combo_settings->itemData(index));
		break;
#endif
	default:
		fmt::throw_exception("Unexpected camera handler %d", static_cast<int>(m_handler));
	}
}

void camera_settings_dialog::handle_qt_camera_change(const QVariant& item_data)
{
	if (!item_data.canConvert<QCameraDevice>())
	{
		ui->combo_settings->clear();
		return;
	}

	const QCameraDevice camera_info = item_data.value<QCameraDevice>();

	if (camera_info.isNull())
	{
		ui->combo_settings->clear();
		return;
	}

	m_camera = std::make_unique<QCamera>(camera_info);
	m_media_capture_session = std::make_unique<QMediaCaptureSession>(nullptr);
	m_media_capture_session->setCamera(m_camera.get());
	m_media_capture_session->setVideoOutput(ui->videoWidget);

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
		const cfg_camera::camera_setting cfg_setting = g_cfg_camera.get_camera_setting(fmt::format("%s", camera_handler::qt), key, success);

		if (success)
		{
			camera_log.notice("Found config entry for camera \"%s\"", key);

			// Select matching dropdown entry
			constexpr double epsilon = 0.001;

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
	}
}

void camera_settings_dialog::handle_qt_settings_change(const QVariant& item_data)
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

	if (item_data.canConvert<QCameraFormat>() && ui->combo_camera->currentData().canConvert<QCameraDevice>())
	{
		const QCameraFormat setting = item_data.value<QCameraFormat>();
		if (!setting.isNull())
		{
			m_camera->setCameraFormat(setting);
		}

		cfg_camera::camera_setting cfg_setting {};
		cfg_setting.width = setting.resolution().width();
		cfg_setting.height = setting.resolution().height();
		cfg_setting.min_fps = setting.minFrameRate();
		cfg_setting.max_fps = setting.maxFrameRate();
		cfg_setting.format = static_cast<int>(setting.pixelFormat());
		cfg_setting.colorspace = 0;
		g_cfg_camera.set_camera_setting(fmt::format("%s", camera_handler::qt), ui->combo_camera->currentData().value<QCameraDevice>().id().toStdString(), cfg_setting);
	}

	m_camera->start();
}

#ifdef HAVE_SDL3
void camera_settings_dialog::handle_sdl_camera_change(const QString& name, const QVariant& item_data)
{
	if (!item_data.canConvert<u32>())
	{
		ui->combo_settings->clear();
		return;
	}

	const u32 camera_id = item_data.value<u32>();

	if (!camera_id)
	{
		ui->combo_settings->clear();
		return;
	}

	ui->combo_settings->blockSignals(true);
	ui->combo_settings->clear();

	std::vector<SDL_CameraSpec> settings;

	int num_formats = 0;
	if (SDL_CameraSpec** specs = SDL_GetCameraSupportedFormats(camera_id, &num_formats))
	{
		if (num_formats <= 0)
		{
			camera_log.error("No SDL camera specs found");
		}
		else
		{
			for (int i = 0; i < num_formats; i++)
			{
				if (!specs[i]) continue;
				settings.push_back(*specs[i]);
			}
		}
		SDL_free(specs);
	}
	else
	{
		camera_log.error("No SDL camera specs found. SDL Error: %s", SDL_GetError());
	}

	std::sort(settings.begin(), settings.end(), [](const SDL_CameraSpec& l, const SDL_CameraSpec& r) -> bool
	{
		const f32 l_fps = l.framerate_numerator / static_cast<f32>(l.framerate_denominator);
		const f32 r_fps = r.framerate_numerator / static_cast<f32>(r.framerate_denominator);

		if (l.width > r.width) return true;
		if (l.width < r.width) return false;
		if (l.height > r.height) return true;
		if (l.height < r.height) return false;
		if (l_fps > r_fps) return true;
		if (l_fps < r_fps) return false;
		if (l.format > r.format) return true;
		if (l.format < r.format) return false;
		if (l.colorspace > r.colorspace) return true;
		if (l.colorspace < r.colorspace) return false;
		return false;
	});

	for (const SDL_CameraSpec& setting : settings)
	{
		const f32 fps = setting.framerate_numerator / static_cast<f32>(setting.framerate_denominator);
		const QString description = tr("%0x%1, %2 FPS, Format=%3")
			.arg(setting.width)
			.arg(setting.height)
			.arg(fps)
			.arg(sdl_pixelformat_to_string(setting.format));
		ui->combo_settings->addItem(description, QVariant::fromValue(setting));
	}
	ui->combo_settings->blockSignals(false);

	if (ui->combo_settings->count() == 0)
	{
		ui->combo_settings->setEnabled(false);
		return;
	}

	// Load selected settings from config file
	int index = 0;
	bool success = false;
	cfg_camera::camera_setting cfg_setting = g_cfg_camera.get_camera_setting(fmt::format("%s", camera_handler::sdl), name.toStdString(), success);

	if (success)
	{
		camera_log.notice("Found config entry for camera \"%s\"", name);

		// Select matching dropdown entry
		constexpr double epsilon = 0.001;

		for (int i = 0; i < ui->combo_settings->count(); i++)
		{
			const QVariant var = ui->combo_settings->itemData(i);
			if (!var.canConvert<SDL_CameraSpec>())
			{
				camera_log.error("Failed to convert itemData to SDL_CameraSpec");
				continue;
			}

			const SDL_CameraSpec tmp = var.value<SDL_CameraSpec>();
			const f32 fps = tmp.framerate_numerator / static_cast<f32>(tmp.framerate_denominator);

			if (tmp.width == cfg_setting.width &&
				tmp.height == cfg_setting.height &&
				fps >= (cfg_setting.min_fps - epsilon) &&
				fps <= (cfg_setting.min_fps + epsilon) &&
				fps >= (cfg_setting.max_fps - epsilon) &&
				fps <= (cfg_setting.max_fps + epsilon) &&
				tmp.format == static_cast<SDL_PixelFormat>(cfg_setting.format) &&
				tmp.colorspace == static_cast<SDL_Colorspace>(cfg_setting.colorspace))
			{
				index = i;
				break;
			}
		}
	}

	m_sdl_camera_id = camera_id;

	ui->combo_settings->setCurrentIndex(std::max<int>(0, index));
	ui->combo_settings->setEnabled(true);
}

void camera_settings_dialog::handle_sdl_settings_change(const QVariant& item_data)
{
	reset_cameras();

	if (item_data.canConvert<SDL_CameraSpec>())
	{
		// TODO: SDL converts the image for us. We would have to do this manually if we want to use other formats.
		const SDL_CameraSpec setting = item_data.value<SDL_CameraSpec>();
		const SDL_CameraSpec used_spec
		{
			.format = SDL_PixelFormat::SDL_PIXELFORMAT_RGBA32,
			.colorspace = SDL_Colorspace::SDL_COLORSPACE_RGB_DEFAULT,
			.width = setting.width,
			.height = setting.height,
			.framerate_numerator = setting.framerate_numerator,
			.framerate_denominator = setting.framerate_denominator
		};

		m_sdl_camera = SDL_OpenCamera(m_sdl_camera_id, &used_spec);

		m_video_frame_input = std::make_unique<QVideoFrameInput>();

		m_media_capture_session = std::make_unique<QMediaCaptureSession>(nullptr);
		m_media_capture_session->setVideoFrameInput(m_video_frame_input.get());
		m_media_capture_session->setVideoOutput(ui->videoWidget);

		connect(this, &camera_settings_dialog::sdl_frame_ready, m_video_frame_input.get(), [this]()
		{
			// It was observed that connecting sendVideoFrame directly can soft-lock the software.
			// So let's just create the video frame here and call it manually.
			std::unique_lock lock(m_sdl_image_mutex, std::defer_lock);
			if (lock.try_lock() && m_video_frame_input && !m_sdl_image.isNull())
			{
				const QVideoFrame video_frame(m_sdl_image);
				if (video_frame.isValid())
				{
					m_video_frame_input->sendVideoFrame(video_frame);
				}
			}
		});

		const f32 fps = setting.framerate_numerator / static_cast<f32>(setting.framerate_denominator);

		cfg_camera::camera_setting cfg_setting {};
		cfg_setting.width = setting.width;
		cfg_setting.height = setting.height;
		cfg_setting.min_fps = fps;
		cfg_setting.max_fps = fps;
		cfg_setting.format = static_cast<int>(setting.format);
		cfg_setting.colorspace = static_cast<int>(setting.colorspace);
		g_cfg_camera.set_camera_setting(fmt::format("%s", camera_handler::sdl), ui->combo_camera->currentText().toStdString(), cfg_setting);
	}

	if (!m_sdl_camera)
	{
		camera_log.error("Failed to open SDL camera %d. SDL Error: %s", m_sdl_camera_id, SDL_GetError());
		QMessageBox::warning(this, tr("Camera not available"), tr("The selected camera is not available.\nIt might be blocked by another application."));
		return;
	}

	m_sdl_thread = std::make_unique<named_thread<std::function<void()>>>("GUI SDL Capture Thread", [this](){ run_sdl(); });
}

void camera_settings_dialog::run_sdl()
{
	camera_log.notice("GUI SDL Capture Thread started");

	while (thread_ctrl::state() != thread_state::aborting)
	{
		// Copy latest image into out buffer.
		u64 timestamp_ns = 0;
		SDL_Surface* frame = SDL_AcquireCameraFrame(m_sdl_camera, &timestamp_ns);
		if (!frame)
		{
			// No new frame
			thread_ctrl::wait_for(1000);
			continue;
		}

		{
			// Map image
			const QImage::Format format = SDL_ISPIXELFORMAT_ALPHA(frame->format) ? QImage::Format_RGBA8888 : QImage::Format_RGB888;
			const QImage image = QImage(reinterpret_cast<const u8*>(frame->pixels), frame->w, frame->h, format);

			// Copy image to prevent memory access violations
			{
				std::lock_guard lock(m_sdl_image_mutex);
				m_sdl_image = image.copy();
			}

			// Notify UI
			Q_EMIT sdl_frame_ready();
		}

		SDL_ReleaseCameraFrame(m_sdl_camera, frame);
	}
}
#endif

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
