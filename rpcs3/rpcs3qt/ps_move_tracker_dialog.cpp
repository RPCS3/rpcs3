#include "stdafx.h"
#include "ps_move_tracker_dialog.h"
#include "ui_ps_move_tracker_dialog.h"
#include "Emu/Cell/Modules/cellCamera.h"
#include "qt_camera_handler.h"
#include "Input/ps_move_handler.h"
#include "Input/ps_move_config.h"
#include "Input/ps_move_tracker.h"

#include <QCheckBox>
#include <QComboBox>
#include <QImage>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QMessageBox>
#include <QMouseEvent>

LOG_CHANNEL(ps_move);

extern u32 get_buffer_size_by_format(s32, s32, s32);

static constexpr int radius_range = 1000;
static const constexpr f64 min_radius_conversion = radius_range / g_cfg_move.min_radius.max;
static const constexpr f64 max_radius_conversion = radius_range / g_cfg_move.max_radius.max;

extern void qt_events_aware_op(int repeat_duration_ms, std::function<bool()> wrapped_op);

ps_move_tracker_dialog::ps_move_tracker_dialog(QWidget* parent)
	: QDialog(parent)
	, ui(new Ui::ps_move_tracker_dialog)
{
	ui->setupUi(this);

	if (!g_cfg_move.load())
	{
		ps_move.notice("Could not load PS Move config. Using defaults.");
	}

	setAttribute(Qt::WA_DeleteOnClose);

	connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button)
	{
		if (button == ui->buttonBox->button(QDialogButtonBox::Save))
		{
			g_cfg_move.save();
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
		{
			g_cfg_move.save();
		}
		else if (button == ui->buttonBox->button(QDialogButtonBox::Cancel))
		{
			if (!g_cfg_move.load())
			{
				ps_move.notice("Could not load PS Move config. Using defaults.");
			}
		}
	});

	m_format = CELL_CAMERA_RGBA;
	ui->inputFormatCombo->addItem(tr("RGBA"), static_cast<int>(CELL_CAMERA_RGBA));
	ui->inputFormatCombo->addItem(tr("RAW8"), static_cast<int>(CELL_CAMERA_RAW8));
	ui->inputFormatCombo->setCurrentIndex(ui->inputFormatCombo->findData(m_format));

	connect(ui->inputFormatCombo, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		if (index < 0) return;
		if (const auto qvar = ui->inputFormatCombo->currentData(); qvar.canConvert<int>())
		{
			m_format = qvar.toInt();
			reset_camera();
		}
	});

	ui->viewCombo->addItem(tr("Image"), static_cast<int>(view_mode::image));
	ui->viewCombo->addItem(tr("Grayscale"), static_cast<int>(view_mode::grayscale));
	ui->viewCombo->addItem(tr("HSV Hue"), static_cast<int>(view_mode::hsv_hue));
	ui->viewCombo->addItem(tr("HSV Saturation"), static_cast<int>(view_mode::hsv_saturation));
	ui->viewCombo->addItem(tr("HSV Value"), static_cast<int>(view_mode::hsv_value));
	ui->viewCombo->addItem(tr("Binary"), static_cast<int>(view_mode::binary));
	ui->viewCombo->addItem(tr("Contours"), static_cast<int>(view_mode::contours));
	ui->viewCombo->setCurrentIndex(ui->viewCombo->findData(static_cast<int>(m_view_mode.load())));

	connect(ui->viewCombo, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		if (index < 0) return;
		if (const auto qvar = ui->viewCombo->currentData(); qvar.canConvert<int>())
		{
			m_view_mode = static_cast<view_mode>(qvar.toInt());
		}
	});

	ui->histoCombo->addItem(tr("Hues"), static_cast<int>(histo_mode::unfiltered_hues));
	ui->histoCombo->setCurrentIndex(ui->histoCombo->findData(static_cast<int>(m_histo_mode)));

	connect(ui->histoCombo, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		if (index < 0) return;
		if (const auto qvar = ui->histoCombo->currentData(); qvar.canConvert<int>())
		{
			m_histo_mode = static_cast<histo_mode>(qvar.toInt());
		}
	});

	ui->brightnessSlider->setRange(g_cfg_move.default_brightness.min, g_cfg_move.default_brightness.max);
	connect(ui->brightnessSlider, &QSlider::valueChanged, this, [this](int value)
	{
		const u32 brightness = std::clamp<u32>(value, g_cfg_move.default_brightness.min, g_cfg_move.default_brightness.max);
		g_cfg_move.default_brightness.set(brightness);
		update_default_brightness(false);
	});

	ui->hueSlider->setRange(g_cfg_move.move1.hue.min, g_cfg_move.move1.hue.max);
	connect(ui->hueSlider, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const u16 hue = std::clamp<u16>(value, config->hue.min, config->hue.max);
		config->hue.set(hue);
		update_hue(false, true, false);
	});

	ui->hueThresholdSlider->setRange(g_cfg_move.move1.hue_threshold.min, g_cfg_move.move1.hue_threshold.max);
	connect(ui->hueThresholdSlider, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const u16 hue_threshold = std::clamp<u16>(value, config->hue_threshold.min, config->hue_threshold.max);
		config->hue_threshold.set(hue_threshold);
		update_hue_threshold();
	});

	ui->saturationThresholdSlider->setRange(g_cfg_move.move1.saturation_threshold.min, g_cfg_move.move1.saturation_threshold.max);
	connect(ui->saturationThresholdSlider, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const u16 saturation_threshold = std::clamp<u16>(value, config->saturation_threshold.min, config->saturation_threshold.max);
		config->saturation_threshold.set(saturation_threshold);
		update_saturation_threshold();
	});

	ui->minRadiusSlider->setRange(0, radius_range);
	connect(ui->minRadiusSlider, &QSlider::valueChanged, this, [this](int value)
	{
		const f32 min_radius = std::clamp(value / min_radius_conversion, g_cfg_move.min_radius.min, g_cfg_move.min_radius.max);
		g_cfg_move.min_radius.set(min_radius);
		update_min_radius();
	});

	ui->maxRadiusSlider->setRange(0, radius_range);
	connect(ui->maxRadiusSlider, &QSlider::valueChanged, this, [this](int value)
	{
		const f32 max_radius = std::clamp(value / max_radius_conversion, g_cfg_move.max_radius.min, g_cfg_move.max_radius.max);
		g_cfg_move.max_radius.set(max_radius);
		update_max_radius();
	});

	ui->colorSliderR->setRange(g_cfg_move.move1.r.min, g_cfg_move.move1.r.max);
	ui->colorSliderG->setRange(g_cfg_move.move1.g.min, g_cfg_move.move1.g.max);
	ui->colorSliderB->setRange(g_cfg_move.move1.b.min, g_cfg_move.move1.b.max);
	connect(ui->colorSliderR, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const u8 r = std::clamp<u8>(value, config->r.min, config->r.max);
		if (config->r.get() != r) config->hue_calibrated.set(false);
		config->r.set(r);
		update_color(false, true);
	});
	connect(ui->colorSliderG, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const u8 g = std::clamp<u8>(value, config->g.min, config->g.max);
		if (config->g.get() != g) config->hue_calibrated.set(false);
		config->g.set(g);
		update_color(false, true);
	});
	connect(ui->colorSliderB, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const u8 b = std::clamp<u8>(value, config->b.min, config->b.max);
		if (config->b.get() != b) config->hue_calibrated.set(false);
		config->b.set(b);
		update_color(false, true);
	});

	connect(ui->calibrateButton, &QPushButton::clicked, this, [this]()
	{
		ps_move.notice("Starting blink calibration of PS Move #%d", m_index + 1);
		ui->calibrateButton->setEnabled(false);
		m_calibration_requested = true;
	});

	connect(ui->filterSmallContoursBox, &QCheckBox::toggled, [this](bool checked)
	{
		m_filter_small_contours = checked;
	});
	ui->filterSmallContoursBox->setChecked(m_filter_small_contours);

	connect(ui->freezeFrameBox, &QCheckBox::toggled, [this](bool checked)
	{
		m_freeze_frame = checked;
	});
	ui->freezeFrameBox->setChecked(m_freeze_frame);

	connect(ui->showAllContoursBox, &QCheckBox::toggled, [this](bool checked)
	{
		m_show_all_contours = checked;
	});
	ui->showAllContoursBox->setChecked(m_show_all_contours);

	connect(ui->drawContoursBox, &QCheckBox::toggled, [this](bool checked)
	{
		m_draw_contours = checked;
	});
	ui->drawContoursBox->setChecked(m_draw_contours);

	connect(ui->drawOverlaysBox, &QCheckBox::toggled, [this](bool checked)
	{
		m_draw_overlays = checked;
	});
	ui->drawOverlaysBox->setChecked(m_draw_overlays);

	for (u32 index = 0; index < CELL_GEM_MAX_NUM; index++)
	{
		ui->comboSelectDevice->addItem(tr("PS Move #%0").arg(index + 1), index);
	}
	ui->comboSelectDevice->setCurrentIndex(ui->comboSelectDevice->findData(m_index.load()));

	connect(ui->comboSelectDevice, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		if (index < 0) return;
		if (const auto qvar = ui->comboSelectDevice->currentData(); qvar.canConvert<int>())
		{
			m_index = qvar.toInt();
			update_default_brightness(true);
			update_color(true, false);
			update_hue(true, false, true);
			update_hue_threshold(true);
			update_saturation_threshold(true);
		}
	});

	m_ps_move_tracker = std::make_unique<ps_move_tracker<true>>();

	m_update_timer = new QTimer(this);
	connect(m_update_timer, &QTimer::timeout, this, [this]()
	{
		std::lock_guard lock(m_image_mutex);

		if (m_image.isNull())
			return;

		QPixmap histogram;

		switch (m_histo_mode)
		{
		case histo_mode::unfiltered_hues:
		{
			histogram = get_histogram(false, width);
			break;
		}
		}

		ui->imageLabel->setPixmap(QPixmap::fromImage(m_image));
		ui->histogramLabel->setPixmap(histogram);
	});

	reset_camera();

	m_input_thread = std::make_unique<named_thread<pad_thread>>(thread(), window(), "");
	qt_events_aware_op(0, [](){ return !!pad::g_started; });

	adjustSize();

	update_default_brightness(true);
	update_color(true, false);
	update_hue(true, false, true);
	update_hue_threshold(true);
	update_saturation_threshold(true);
	update_min_radius(true);
	update_max_radius(true);

	ui->imageLabel->installEventFilter(this);
}

ps_move_tracker_dialog::~ps_move_tracker_dialog()
{
	m_update_timer->stop();

	if (m_tracker_thread)
	{
		m_stop_threads = 1;
		m_tracker_thread->wait();
	}

	// Don't leave the LED turned off
	abort_calibration();

	if (m_camera_handler)
	{
		m_camera_handler->close_camera();
	}

	// Join thread
	m_input_thread.reset();
}

bool ps_move_tracker_dialog::eventFilter(QObject* object, QEvent* event)
{
	if (event && object == ui->imageLabel && event->type() == QEvent::Type::MouseButtonRelease && m_view_mode == view_mode::contours)
	{
		const QMouseEvent* me = static_cast<QMouseEvent*>(event);

		if (me->button() == Qt::MouseButton::LeftButton)
		{
			ps_move.notice("Selecting color with mouse click.");

			f32 r = 0.0f;
			f32 g = 0.0f;
			f32 b = 0.0f;
			u32 count = 0;

			{
				std::lock_guard lock(m_image_mutex);

				const auto pos = me->position().toPoint();
				const QPixmap pixmap = ui->imageLabel->pixmap();

				if (pixmap.rect().contains(pos))
				{
					const QImage image = pixmap.toImage();
					constexpr int radius = 3;

					for (int y = pos.y() - radius; y <= pos.y() + radius; ++y)
					{
						for (int x = pos.x() - radius; x <= pos.x() + radius; ++x)
						{
							if (!image.rect().contains(x, y))
								continue;

							const QColor color = image.pixelColor(x, y);

							r += color.redF();
							g += color.greenF();
							b += color.blueF();
							count++;
						}
					}
				}
			}

			if (count > 0)
			{
				r /= count;
				g /= count;
				b /= count;

				ps_move.notice("Selected new color with mouse click (r=%f, g=%f, b=%f)", r, g, b);

				cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
				const auto [hue, saturation, value] = ps_move_tracker<true>::rgb_to_hsv(r, g, b);
				config->hue.set(std::clamp<u32>(hue, config->hue.min, config->hue.max));
				update_hue(true, false, false);
			}
		}
	}

	return QDialog::eventFilter(object, event);
}

void ps_move_tracker_dialog::update_color(bool update_rgb_sliders, bool update_hue_slider)
{
	cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);

	ui->colorGb->setTitle(tr("Color: R=%0, G=%1, B=%2").arg(config->r.get()).arg(config->g.get()).arg(config->b.get()));

	if (update_rgb_sliders)
	{
		ui->colorSliderR->setValue(config->r.get());
		ui->colorSliderG->setValue(config->g.get());
		ui->colorSliderB->setValue(config->b.get());
	}
	else if (update_hue_slider)
	{
		const auto [hue, saturation, value] = ps_move_tracker<true>::rgb_to_hsv(config->r.get() / 255.0f, config->g.get() / 255.0f, config->b.get() / 255.0f);
		config->hue.set(std::clamp<u32>(hue, config->hue.min, config->hue.max));
		update_hue(true, false, true);
	}

	if (!m_input_thread)
	{
		return;
	}

	std::lock_guard lock(pad::g_pad_mutex);
	auto& handlers = m_input_thread->get_handlers();
	if (auto it = handlers.find(pad_handler::move); it != handlers.end())
	{
		for (auto& binding : it->second->bindings())
		{
			if (binding.device)
			{
				binding.device->color_override_active = true;
				binding.device->color_override.r = config->r.get();
				binding.device->color_override.g = config->g.get();
				binding.device->color_override.b = config->b.get();
			}
		}
	}
}

void ps_move_tracker_dialog::update_default_brightness(bool update_slider)
{
	const u32 brightness = g_cfg_move.default_brightness.get();
	ui->brightnessGb->setTitle(tr("Default Orb Brightness: %0 %").arg(brightness));

	if (update_slider)
	{
		ui->brightnessSlider->setValue(brightness);
	}
}

void ps_move_tracker_dialog::update_hue(bool update_slider, bool update_rgb_sliders, bool no_signal)
{
	const cfg_ps_move* hue_config = ::at32(g_cfg_move.move, m_index);
	const u32 hue = hue_config->hue.get();

	if (hue_config->hue_calibrated)
	{
		ui->hueGb->setTitle(tr("Hue: %0 (calibrated: %1)").arg(hue).arg(hue_config->calibrated_hue.get()));
	}
	else
	{
		ui->hueGb->setTitle(tr("Hue: %0").arg(hue));
	}

	if (update_slider)
	{
		if (no_signal) ui->hueSlider->blockSignals(true);
		ui->hueSlider->setValue(hue);
		if (no_signal) ui->hueSlider->blockSignals(false);
	}
	else if (update_rgb_sliders)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const f32 default_brightness = g_cfg_move.default_brightness.get() / static_cast<f32>(g_cfg_move.default_brightness.max);
		const auto [r, g, b] = ps_move_tracker<true>::hsv_to_rgb(hue, 1.0f, 1.0f);
		const u8 new_r = static_cast<u8>(r * default_brightness);
		const u8 new_g = static_cast<u8>(g * default_brightness);
		const u8 new_b = static_cast<u8>(b * default_brightness);

		// The calibration is only valid for the color that was used during the calibration
		if (config->r.get() != new_r || config->g.get() != new_g || config->b.get() != new_b)
		{
			config->hue_calibrated.set(false);
			update_hue(false, false, true);
		}

		config->r.set(new_r);
		config->g.set(new_g);
		config->b.set(new_b);
		update_color(true, false);
	}
}

void ps_move_tracker_dialog::update_hue_threshold(bool update_slider)
{
	const u32 hue_threshold = ::at32(g_cfg_move.move, m_index)->hue_threshold.get();
	ui->hueThresholdGb->setTitle(tr("Hue Threshold: %0").arg(hue_threshold));

	if (update_slider)
	{
		ui->hueThresholdSlider->setValue(hue_threshold);
	}
}

void ps_move_tracker_dialog::update_saturation_threshold(bool update_slider)
{
	const u32 saturation_threshold = ::at32(g_cfg_move.move, m_index)->saturation_threshold.get();
	ui->saturationThresholdGb->setTitle(tr("Saturation Threshold: %0").arg(saturation_threshold));

	if (update_slider)
	{
		ui->saturationThresholdSlider->setValue(saturation_threshold);
	}
}

void ps_move_tracker_dialog::update_min_radius(bool update_slider)
{
	ui->minRadiusGb->setTitle(tr("Min Radius: %0 %").arg(g_cfg_move.min_radius.get()));

	if (update_slider)
	{
		ui->minRadiusSlider->setValue(g_cfg_move.min_radius * min_radius_conversion);
	}
}

void ps_move_tracker_dialog::update_max_radius(bool update_slider)
{
	ui->maxRadiusGb->setTitle(tr("Max Radius: %0 %").arg(g_cfg_move.max_radius.get()));

	if (update_slider)
	{
		ui->maxRadiusSlider->setValue(g_cfg_move.max_radius * max_radius_conversion);
	}
}

void ps_move_tracker_dialog::reset_camera()
{
	m_update_timer->stop();

	if (m_tracker_thread)
	{
		m_stop_threads = 1;
		m_tracker_thread->wait();
		m_stop_threads = 0;
	}

	abort_calibration();
	m_calibration_requested = false;
	ui->calibrateButton->setEnabled(true);

	std::lock_guard camera_lock(m_camera_handler_mutex);

	const u64 size = get_buffer_size_by_format(m_format, width, height);
	m_image_data_frozen.resize(size);
	m_image_data.resize(size);
	m_frame_number = 0;

	m_camera_handler = std::make_unique<qt_camera_handler>();
	m_camera_handler->set_resolution(width, height);
	m_camera_handler->set_format(m_format, size);
	m_camera_handler->set_mirrored(true);
	m_camera_handler->open_camera();
	m_camera_handler->start_camera();

	m_update_timer->start(1000 / 60);

	m_tracker_thread.reset(QThread::create([this]()
	{
		thread_base::set_name("PS Move Tracker");

		while (!m_stop_threads)
		{
			process_camera_frame();
		}
	}));
	m_tracker_thread->start();
}

void ps_move_tracker_dialog::process_camera_frame()
{
	std::lock_guard camera_lock(m_camera_handler_mutex);

	if (!m_camera_handler || m_camera_handler->get_state() == qt_camera_handler::camera_handler_state::closed)
	{
		// Wait some time
		std::this_thread::sleep_for(100us);
		return;
	}

	u32 width = 0;
	u32 height = 0;
	u64 frame_number = 0;
	u64 bytes_read = 0;

	const camera_handler_base::camera_handler_state state = m_camera_handler->get_image(m_image_data.data(), m_image_data.size(), width, height, frame_number, bytes_read);

	if (state != camera_handler_base::camera_handler_state::running || frame_number <= m_frame_number)
	{
		// Wait some time
		std::this_thread::sleep_for(100us);
		return;
	}

	m_frame_number = frame_number;

	const bool freeze_frame = m_freeze_frame;

	if (m_frame_frozen != freeze_frame)
	{
		m_frame_frozen = freeze_frame;

		if (m_frame_frozen)
		{
			std::memcpy(m_image_data_frozen.data(), m_image_data.data(), m_image_data.size());
		}
	}

	const u32 index = m_index;

	for (u32 i = 0; i < CELL_GEM_MAX_NUM; i++)
	{
		const cfg_ps_move* config = g_cfg_move.move[i];

		m_ps_move_tracker->set_active(i, index == i);
		m_ps_move_tracker->set_hue(i, config->hue_calibrated ? config->calibrated_hue.get() : config->hue.get());
		m_ps_move_tracker->set_hue_threshold(i, static_cast<u8>(config->hue_threshold.get()));
		m_ps_move_tracker->set_saturation_threshold(i, static_cast<u8>(config->saturation_threshold.get()));
		m_ps_move_tracker->set_value_threshold(i, static_cast<u8>(config->value_threshold.get()));
	}

	// Always use live images during the calibration
	const bool use_frozen_frame = m_frame_frozen && !m_calibration.active && !m_calibration_requested;

	m_ps_move_tracker->set_image_data(use_frozen_frame ? m_image_data_frozen.data() : m_image_data.data(), m_image_data.size(), width, height, m_camera_handler->format());
	m_ps_move_tracker->set_min_radius(static_cast<f32>(g_cfg_move.min_radius) / 100.0f);
	m_ps_move_tracker->set_max_radius(static_cast<f32>(g_cfg_move.max_radius) / 100.0f);
	m_ps_move_tracker->set_filter_small_contours(m_filter_small_contours);
	m_ps_move_tracker->set_show_all_contours(m_show_all_contours);
	m_ps_move_tracker->set_draw_contours(m_draw_contours);
	m_ps_move_tracker->set_draw_overlays(m_draw_overlays);
	m_ps_move_tracker->process_image();

	process_calibration();

	const std::vector<u8>* result = nullptr;
	QImage::Format format = QImage::Format::Format_Invalid;
	qsizetype bytes_per_line = 0;
	const view_mode _view_mode = m_view_mode;

	switch (_view_mode)
	{
	case view_mode::image:
	{
		result = &m_ps_move_tracker->rgba();
		format = QImage::Format::Format_RGBA8888;
		bytes_per_line = width * 4ULL;
		break;
	}
	case view_mode::grayscale:
	{
		result = &m_ps_move_tracker->gray();
		format = QImage::Format::Format_Grayscale8;
		bytes_per_line = width;
		break;
	}
	case view_mode::hsv_hue:
	case view_mode::hsv_saturation:
	case view_mode::hsv_value:
	{
		const u32 value_index = static_cast<u32>(_view_mode) - static_cast<u32>(view_mode::hsv_hue);
		const std::vector<u8>& hsv = m_ps_move_tracker->hsv();
		static std::vector<u8> hsv_single;
		hsv_single.resize(hsv.size() / 3);
		for (usz i = 0; i < hsv_single.size(); i++)
		{
			hsv_single[i] = hsv[i * 3 + value_index];
		}
		result = &hsv_single;
		format = QImage::Format::Format_Grayscale8;
		bytes_per_line = width;
		break;
	}
	case view_mode::binary:
	{
		result = &m_ps_move_tracker->binary(index);
		format = QImage::Format::Format_Grayscale8;
		bytes_per_line = width;
		break;
	}
	case view_mode::contours:
	{
		result = &m_ps_move_tracker->rgba_contours();
		format = QImage::Format::Format_RGBA8888;
		bytes_per_line = width * 4ULL;
		break;
	}
	}

	QImage image = QImage(result->data(), width, height, bytes_per_line, format, nullptr, nullptr).copy();

	const ps_move_sphere_color& sphere_color = m_ps_move_tracker->sphere_color(index);

	std::lock_guard lock(m_image_mutex);
	m_image = std::move(image);
	m_hues = m_ps_move_tracker->hues();
	m_measured_color.valid = sphere_color.valid;
	m_measured_color.hue = sphere_color.hue;
	m_measured_color.saturation = sphere_color.saturation;
	m_measured_color.value = sphere_color.value;
}

void ps_move_tracker_dialog::set_led_enabled(bool enabled)
{
	if (!m_input_thread)
	{
		return;
	}

	const cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);

	std::lock_guard lock(pad::g_pad_mutex);
	auto& handlers = m_input_thread->get_handlers();
	if (auto it = handlers.find(pad_handler::move); it != handlers.end())
	{
		for (auto& binding : it->second->bindings())
		{
			if (binding.device)
			{
				binding.device->color_override_active = true;
				binding.device->color_override.r = enabled ? config->r.get() : 0;
				binding.device->color_override.g = enabled ? config->g.get() : 0;
				binding.device->color_override.b = enabled ? config->b.get() : 0;
			}
		}
	}
}

void ps_move_tracker_dialog::abort_calibration()
{
	if (!m_calibration.active)
	{
		return;
	}

	ps_move.notice("Aborting blink calibration");

	m_calibration = {};
	set_led_enabled(true);
}

void ps_move_tracker_dialog::process_calibration()
{
	// Turn the LED off and on a couple of times and compare the images. The sphere is the area that blinks.
	constexpr u32 blink_count = 3;
	constexpr u32 min_frames_per_step = 3;
	constexpr auto min_step_duration = 300ms; // The PS Move handler limits LED updates to one every 120ms and cameras have some latency

	const auto now = steady_clock::now();

	if (!m_calibration.active)
	{
		if (!m_calibration_requested.exchange(false))
		{
			return;
		}

		m_calibration = {};
		m_calibration.active = true;
		m_calibration.index = m_index;
		m_calibration.step_start = now;

		const cfg_ps_move* config = ::at32(g_cfg_move.move, m_calibration.index);
		m_calibration.led_r = config->r.get();
		m_calibration.led_g = config->g.get();
		m_calibration.led_b = config->b.get();

		set_led_enabled(false);
		return;
	}

	// Wait until the LED has changed and the camera has caught up
	if (++m_calibration.frames < min_frames_per_step || now - m_calibration.step_start < min_step_duration)
	{
		return;
	}

	std::vector<std::vector<u8>>& images = m_calibration.led_on ? m_calibration.images_on : m_calibration.images_off;
	images.push_back(m_ps_move_tracker->rgba());

	if (m_calibration.images_on.size() < blink_count)
	{
		// Next step
		m_calibration.led_on = !m_calibration.led_on;
		m_calibration.frames = 0;
		m_calibration.step_start = now;
		set_led_enabled(m_calibration.led_on);
		return;
	}

	// Done
	ps_move_tracker_calibration calibration {};
	const ps_move_calibration_result result = ps_move_calibrate_sphere(m_calibration.images_on, m_calibration.images_off, width, height, calibration);
	const u32 index = m_calibration.index;
	const std::array<u8, 3> led_color = { m_calibration.led_r, m_calibration.led_g, m_calibration.led_b };

	m_calibration = {};
	set_led_enabled(true);

	ps_move.notice("Blink calibration of PS Move #%d finished: result=%d, hue=%f, saturation=%f, value=%f, hue_threshold=%f, min_saturation=%d, min_value=%d, background_ratio=%f",
		index + 1, static_cast<int>(result), calibration.color.hue, calibration.color.saturation, calibration.color.value,
		calibration.hue_threshold, calibration.min_saturation, calibration.min_value, calibration.background_ratio);

	QMetaObject::invokeMethod(this, [this, result, calibration, index, led_color]()
	{
		ui->calibrateButton->setEnabled(true);

		switch (result)
		{
		case ps_move_calibration_result::ok:
		{
			cfg_ps_move* config = ::at32(g_cfg_move.move, index);

			// The calibration is only valid for the color that was used during the calibration
			if (config->r.get() != led_color[0] || config->g.get() != led_color[1] || config->b.get() != led_color[2])
			{
				QMessageBox::warning(this, tr("Calibration failed"), tr("The color of the sphere was changed during the calibration."));
				break;
			}

			const ps_move_sphere_color& color = calibration.color;
			const u32 hue = static_cast<u32>(std::lround(color.hue)) % 360;
			const u32 hue_threshold = static_cast<u32>(std::lround(calibration.hue_threshold));

			// Store the calibration in the config. The configured hue and the color of the sphere stay the same.
			config->calibrated_hue.set(std::clamp<u32>(hue, config->calibrated_hue.min, config->calibrated_hue.max));
			config->hue_calibrated.set(true);
			config->hue_threshold.set(std::clamp<u32>(hue_threshold, config->hue_threshold.min, config->hue_threshold.max));
			config->saturation_threshold.set(std::clamp<u32>(calibration.min_saturation, config->saturation_threshold.min, config->saturation_threshold.max));
			config->value_threshold.set(std::clamp<u32>(calibration.min_value, config->value_threshold.min, config->value_threshold.max));

			if (index == m_index)
			{
				update_hue(true, false, true);
				update_hue_threshold(true);
				update_saturation_threshold(true);
			}

			QString text = tr("The camera sees the sphere of PS Move #%0 with:\nHue: %1\nSaturation: %2\nValue: %3\n\n"
				"Calibrated thresholds:\nHue: %4\nSaturation: %5\nValue: %6\n\n"
				"The calibrated hue and thresholds are used until the color of the sphere changes.\nSave the settings to keep them.")
				.arg(index + 1).arg(hue).arg(static_cast<int>(color.saturation)).arg(static_cast<int>(color.value))
				.arg(hue_threshold).arg(calibration.min_saturation).arg(calibration.min_value);

			// Warn if the background has a similar color
			constexpr f32 max_background_ratio = 0.005f;

			if (calibration.background_ratio > max_background_ratio)
			{
				text += tr("\n\nWarning: %0% of the background has a similar color. This may disturb the tracking.").arg(calibration.background_ratio * 100.0f, 0, 'f', 1);
			}

			QMessageBox::information(this, tr("Calibration finished"), text);
			break;
		}
		case ps_move_calibration_result::invalid_images:
			QMessageBox::warning(this, tr("Calibration failed"), tr("The camera images could not be used."));
			break;
		case ps_move_calibration_result::not_found:
			QMessageBox::warning(this, tr("Calibration failed"), tr("The sphere could not be found.\nMake sure that the PS Move is connected and that its sphere is visible to the camera."));
			break;
		case ps_move_calibration_result::not_round:
			QMessageBox::warning(this, tr("Calibration failed"), tr("The blinking area is not round.\nHold the PS Move still while it is blinking and make sure that nothing else changes in front of the camera."));
			break;
		case ps_move_calibration_result::no_color:
			QMessageBox::warning(this, tr("Calibration failed"), tr("The sphere has almost no color in the camera image.\nThe sphere may be too bright. Try to reduce the brightness of the sphere or move it further away from the camera."));
			break;
		}
	}, Qt::QueuedConnection);
}

QPixmap ps_move_tracker_dialog::get_histogram(bool ignore_zero, int width) const
{
	// Create image
	const int height = ui->histogramLabel->height();
	const f32 pixelsPerHue = width / static_cast<f32>(m_hues.size());

	// Paint background
	QPixmap histo(width, height);
	histo.fill(Qt::white);

	const cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
	const u16 hue = config->hue;
	const u16 hue_threshold = config->hue_threshold;

	// The tracker uses the calibrated hue instead of the configured hue if there is one
	sphere_color_info calibrated_color {};

	if (config->hue_calibrated)
	{
		calibrated_color.valid = true;
		calibrated_color.hue = config->calibrated_hue.get();
		calibrated_color.saturation = 255.0f;
		calibrated_color.value = 255.0f;
		calibrated_color.is_calibration = true;
		calibrated_color.hue_threshold = hue_threshold;
		calibrated_color.min_saturation = config->saturation_threshold.get();
		calibrated_color.min_value = config->value_threshold.get();
	}
	const int min_hue = hue - hue_threshold;
	const int max_hue = hue + hue_threshold;

	QPainter painter(&histo);

	// Paints a hue range around the given hue
	const auto paint_hue_range = [&](f32 center, f32 range, const QColor& color)
	{
		const f32 min_range_hue = center - range;
		const int range_width = std::max(1, static_cast<int>((range * 2.0f + 1.0f) * pixelsPerHue));

		painter.fillRect(static_cast<int>(min_range_hue * pixelsPerHue), 0, range_width, histo.height(), color);

		// Wrap around
		if (min_range_hue < 0.0f)
		{
			painter.fillRect(static_cast<int>((min_range_hue + 360.0f) * pixelsPerHue), 0, range_width, histo.height(), color);
		}
		else if (min_range_hue + range * 2.0f >= 360.0f)
		{
			painter.fillRect(static_cast<int>((min_range_hue - 360.0f) * pixelsPerHue), 0, range_width, histo.height(), color);
		}
	};

	// Paint hue threshold. The tracker uses the calibrated hue instead of the configured hue if there is one.
	if (calibrated_color.valid)
	{
		paint_hue_range(calibrated_color.hue, calibrated_color.hue_threshold, QColor(150, 180, 255));
	}
	else
	{
		const int hue_threshold_width = (hue_threshold * 2 + 1) * pixelsPerHue;
		painter.fillRect(min_hue * pixelsPerHue, 0, hue_threshold_width, histo.height(), Qt::lightGray);

		if (min_hue < 0)
		{
			painter.fillRect((min_hue + 360) * pixelsPerHue, 0, hue_threshold_width, histo.height(), Qt::lightGray);
		}
		else if (max_hue >= 360)
		{
			painter.fillRect(0, 0, (max_hue - 360 + 1) * pixelsPerHue, histo.height(), Qt::lightGray);
		}
	}

	const auto get_hue_bar_x = [pixelsPerHue](u16 hue)
	{
		const int x0 = hue * pixelsPerHue;
		const int x1 = std::max<int>(x0, x0 + pixelsPerHue - 1);
		return std::pair<int, int>(x0, x1);
	};

	// Paint the hue range of the measured sphere color
	if (m_measured_color.valid)
	{
		paint_hue_range(m_measured_color.hue, ps_move_tracker<true>::sphere_hue_range, QColor(100, 200, 100, 120));
	}

	// Paint target hue
	const auto [r, g, b] = ps_move_tracker<true>::hsv_to_rgb(hue, 1.0f, 1.0f);
	const auto [x0, x1] = get_hue_bar_x(hue);
	painter.setPen(QColor(r, g, b));
	painter.drawLine(x0, 0, x1, histo.height() - 1);

	// Paints a sphere color as a line at its hue and as a sample with text in the given row
	const auto paint_sphere_color = [&](int row, const sphere_color_info& color, const QString& name, const QColor& line_color)
	{
		constexpr int swatch_size = 16;
		const QRect swatch(4, 4 + row * (swatch_size + 4), swatch_size, swatch_size);
		QString text;

		if (color.valid)
		{
			const u16 color_hue = static_cast<u16>(std::lround(color.hue) % 360);
			const auto [cr, cg, cb] = ps_move_tracker<true>::hsv_to_rgb(color_hue, color.saturation / 255.0f, color.value / 255.0f);
			const auto [cx0, cx1] = get_hue_bar_x(color_hue);

			painter.setPen(QPen(line_color, 1, Qt::DashLine));
			painter.drawLine(cx0, 0, cx0, histo.height() - 1);

			painter.fillRect(swatch, QColor(cr, cg, cb));
			if (color.is_calibration)
			{
				text = tr("%0: H=%1 (hue +/-%2, S >= %3, V >= %4)").arg(name).arg(color_hue).arg(static_cast<int>(color.hue_threshold)).arg(color.min_saturation).arg(color.min_value);
			}
			else
			{
				text = tr("%0: H=%1 S=%2 V=%3").arg(name).arg(color_hue).arg(static_cast<int>(color.saturation)).arg(static_cast<int>(color.value));
			}
		}
		else
		{
			text = tr("%0: -").arg(name);
		}

		painter.setPen(Qt::black);
		painter.drawRect(swatch);
		painter.drawText(swatch.right() + 6, swatch.top(), width, swatch_size, Qt::AlignLeft | Qt::AlignVCenter, text);
	};

	// Paint the calibrated and measured colors. This is done last, so the histogram doesn't cover the text.
	const auto paint_measured_color = [&]()
	{
		paint_sphere_color(0, calibrated_color, tr("Calibrated"), Qt::darkBlue);
		paint_sphere_color(1, m_measured_color, tr("Measured"), Qt::darkGreen);
	};

	// Paint histogram
	painter.setPen(Qt::black);

	const u32 zero_offset = (ignore_zero ? 1 : 0);

	const auto max_elem = std::max_element(m_hues.begin() + zero_offset, m_hues.end());
	const u32 max_value = max_elem != m_hues.end() ? *max_elem : 0u;

	if (!max_value)
	{
		paint_measured_color();
		return histo;
	}

	for (int i = zero_offset; i < static_cast<int>(m_hues.size()); i++)
	{
		const int bar_height = (m_hues[i] / static_cast<float>(max_value)) * height;
		if (bar_height <= 0) continue;

		const auto [x0, x1] = get_hue_bar_x(i);
		painter.drawLine(x0, height - 1, x1, height - bar_height);
	}

	paint_measured_color();

	return histo;
}
