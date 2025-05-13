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

LOG_CHANNEL(ps_move);

extern u32 get_buffer_size_by_format(s32, s32, s32);

static constexpr bool tie_hue_to_color = true;
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
		else if (button == ui->buttonBox->button(QDialogButtonBox::Close))
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
	ui->viewCombo->setCurrentIndex(ui->viewCombo->findData(static_cast<int>(m_view_mode)));

	connect(ui->viewCombo, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		if (index < 0) return;
		if (const auto qvar = ui->viewCombo->currentData(); qvar.canConvert<int>())
		{
			m_view_mode = static_cast<view_mode>(qvar.toInt());
		}
	});

	ui->histoCombo->addItem(tr("Hues"), static_cast<int>(histo_mode::unfiltered_hues));
	ui->histoCombo->setCurrentIndex(ui->viewCombo->findData(static_cast<int>(m_histo_mode)));

	connect(ui->histoCombo, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		if (index < 0) return;
		if (const auto qvar = ui->histoCombo->currentData(); qvar.canConvert<int>())
		{
			m_histo_mode = static_cast<histo_mode>(qvar.toInt());
		}
	});

	connect(ui->hueSlider, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const u16 hue = std::clamp<u16>(value, config->hue.min, config->hue.max);
		config->hue.set(hue);
		update_hue();
	});
	ui->hueSlider->setRange(g_cfg_move.move1.hue.min, g_cfg_move.move1.hue.max);

	connect(ui->hueThresholdSlider, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const u16 hue_threshold = std::clamp<u16>(value, config->hue_threshold.min, config->hue_threshold.max);
		config->hue_threshold.set(hue_threshold);
		update_hue_threshold();
	});
	ui->hueThresholdSlider->setRange(g_cfg_move.move1.hue_threshold.min, g_cfg_move.move1.hue_threshold.max);

	connect(ui->saturationThresholdSlider, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const u16 saturation_threshold = std::clamp<u16>(value, config->saturation_threshold.min, config->saturation_threshold.max);
		config->saturation_threshold.set(saturation_threshold);
		update_saturation_threshold();
	});
	ui->saturationThresholdSlider->setRange(g_cfg_move.move1.saturation_threshold.min, g_cfg_move.move1.saturation_threshold.max);

	connect(ui->minRadiusSlider, &QSlider::valueChanged, this, [this](int value)
	{
		const f32 min_radius = std::clamp(value / min_radius_conversion, g_cfg_move.min_radius.min, g_cfg_move.min_radius.max);
		g_cfg_move.min_radius.set(min_radius);
		update_min_radius();
	});
	ui->minRadiusSlider->setRange(0, radius_range);

	connect(ui->maxRadiusSlider, &QSlider::valueChanged, this, [this](int value)
	{
		const f32 max_radius = std::clamp(value / max_radius_conversion, g_cfg_move.max_radius.min, g_cfg_move.max_radius.max);
		g_cfg_move.max_radius.set(max_radius);
		update_max_radius();
	});
	ui->maxRadiusSlider->setRange(0, radius_range);

	connect(ui->colorSliderR, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		config->r.set(std::clamp<u8>(value, config->r.min, config->r.max));
		update_color();
	});
	connect(ui->colorSliderG, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		config->g.set(std::clamp<u8>(value, config->g.min, config->g.max));
		update_color();
	});
	connect(ui->colorSliderB, &QSlider::valueChanged, this, [this](int value)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		config->b.set(std::clamp<u8>(value, config->b.min, config->b.max));
		update_color();
	});
	ui->colorSliderR->setRange(g_cfg_move.move1.r.min, g_cfg_move.move1.r.max);
	ui->colorSliderG->setRange(g_cfg_move.move1.g.min, g_cfg_move.move1.g.max);
	ui->colorSliderB->setRange(g_cfg_move.move1.b.min, g_cfg_move.move1.b.max);

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
	ui->comboSelectDevice->setCurrentIndex(ui->comboSelectDevice->findData(m_index));

	connect(ui->comboSelectDevice, &QComboBox::currentIndexChanged, this, [this](int index)
	{
		if (index < 0) return;
		if (const auto qvar = ui->comboSelectDevice->currentData(); qvar.canConvert<int>())
		{
			m_index = qvar.toInt();
			update_color(true);
			update_hue(true);
			update_hue_threshold(true);
			update_saturation_threshold(true);
		}
	});

	m_ps_move_tracker = std::make_unique<ps_move_tracker<true>>();

	m_update_timer = new QTimer(this);
	connect(m_update_timer, &QTimer::timeout, this, [this]()
	{
		std::lock_guard lock(m_image_mutex);

		if (m_image.isNull() || m_histogram.isNull())
			return;

		ui->imageLabel->setPixmap(m_image);
		ui->histogramLabel->setPixmap(m_histogram);
	});

	reset_camera();

	m_input_thread = std::make_unique<named_thread<pad_thread>>(thread(), window(), "");
	qt_events_aware_op(0, [](){ return !!pad::g_started; });

	adjustSize();

	update_color(true);
	update_hue(true);
	update_hue_threshold(true);
	update_saturation_threshold(true);
	update_min_radius(true);
	update_max_radius(true);

	if constexpr (!g_ps_move_tracking_supported)
	{
		QTimer::singleShot(1000, this, [this]()
		{
			QMessageBox::warning(this, QObject::tr("Tracking not supported!"), QObject::tr("The PS Move tracking is not yet supported on this operating system."));
		});
	}
}

ps_move_tracker_dialog::~ps_move_tracker_dialog()
{
	m_update_timer->stop();

	if (m_tracker_thread)
	{
		m_stop_threads = 1;
		m_tracker_thread->wait();
	}

	if (m_camera_handler)
	{
		m_camera_handler->close_camera();
	}

	// Join thread
	m_input_thread.reset();
}

void ps_move_tracker_dialog::update_color(bool update_sliders)
{
	cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);

	ui->colorGb->setTitle(tr("Color: R=%0, G=%1, B=%2").arg(config->r.get()).arg(config->g.get()).arg(config->b.get()));

	if (update_sliders)
	{
		ui->colorSliderR->setValue(config->r.get());
		ui->colorSliderG->setValue(config->g.get());
		ui->colorSliderB->setValue(config->b.get());
	}
	else if (tie_hue_to_color)
	{
		const auto [hue, saturation, value] = ps_move_tracker<true>::rgb_to_hsv(config->r.get() / 255.0f, config->g.get() / 255.0f, config->b.get() / 255.0f);
		config->hue.set(std::clamp<u32>(hue, config->hue.min, config->hue.max));
		update_hue(true);
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

void ps_move_tracker_dialog::update_hue(bool update_slider)
{
	const u32 hue = ::at32(g_cfg_move.move, m_index)->hue.get();
	ui->hueGb->setTitle(tr("Hue: %0").arg(hue));

	if (update_slider)
	{
		ui->hueSlider->setValue(hue);
	}
	else if (tie_hue_to_color)
	{
		cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
		const auto [r, g, b] = ps_move_tracker<true>::hsv_to_rgb(hue, 1.0f, 1.0f);
		config->r.set(r / 100);
		config->g.set(g / 100);
		config->b.set(b / 100);
		update_color(true);
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

	if (m_frame_frozen != m_freeze_frame)
	{
		m_frame_frozen = m_freeze_frame;

		if (m_frame_frozen)
		{
			std::memcpy(m_image_data_frozen.data(), m_image_data.data(), m_image_data.size());
		}
	}

	for (u32 index = 0; index < CELL_GEM_MAX_NUM; index++)
	{
		const cfg_ps_move* config = g_cfg_move.move[index];

		m_ps_move_tracker->set_active(index, m_index == index);
		m_ps_move_tracker->set_hue(index, config->hue);
		m_ps_move_tracker->set_hue_threshold(index, config->hue_threshold);
		m_ps_move_tracker->set_saturation_threshold(index, config->saturation_threshold);
	}

	m_ps_move_tracker->set_image_data(m_frame_frozen ? m_image_data_frozen.data() : m_image_data.data(), m_image_data.size(), width, height, m_camera_handler->format());
	m_ps_move_tracker->set_min_radius(static_cast<f32>(g_cfg_move.min_radius) / 100.0f);
	m_ps_move_tracker->set_max_radius(static_cast<f32>(g_cfg_move.max_radius) / 100.0f);
	m_ps_move_tracker->set_filter_small_contours(m_filter_small_contours);
	m_ps_move_tracker->set_show_all_contours(m_show_all_contours);
	m_ps_move_tracker->set_draw_contours(m_draw_contours);
	m_ps_move_tracker->set_draw_overlays(m_draw_overlays);
	m_ps_move_tracker->process_image();

	const std::vector<u8>* result = nullptr;
	QImage::Format format = QImage::Format::Format_Invalid;
	qsizetype bytes_per_line = 0;

	switch (m_view_mode)
	{
	case view_mode::image:
	{
		result = &m_ps_move_tracker->rgba();
		format = QImage::Format::Format_RGBA8888;
		bytes_per_line = width * 4;
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
		const int index = static_cast<int>(m_view_mode) - static_cast<int>(view_mode::hsv_hue);
		const std::vector<u8>& hsv = m_ps_move_tracker->hsv();
		static std::vector<u8> hsv_single;
		hsv_single.resize(hsv.size() / 3);
		for (int i = 0; i < static_cast<int>(hsv_single.size()); i++)
		{
			hsv_single[i] = hsv[i * 3 + index];
		}
		result = &hsv_single;
		format = QImage::Format::Format_Grayscale8;
		bytes_per_line = width;
		break;
	}
	case view_mode::binary:
	{
		result = &m_ps_move_tracker->binary(m_index);
		format = QImage::Format::Format_Grayscale8;
		bytes_per_line = width;
		break;
	}
	case view_mode::contours:
	{
		result = &m_ps_move_tracker->rgba_contours();
		format = QImage::Format::Format_RGBA8888;
		bytes_per_line = width * 4;
		break;
	}
	}

	QPixmap histogram;

	switch (m_histo_mode)
	{
	case histo_mode::unfiltered_hues:
	{
		histogram = get_histogram(m_ps_move_tracker->hues(), false);
		break;
	}
	}

	const QImage image(result->data(), width, height, bytes_per_line, format, nullptr, nullptr);

	std::lock_guard lock(m_image_mutex);
	m_image = QPixmap::fromImage(image);
	m_histogram = std::move(histogram);
}

QPixmap ps_move_tracker_dialog::get_histogram(const std::array<u32, 360>& hues, bool ignore_zero) const
{
	// Create image
	const int height = ui->histogramLabel->height();
	static QPixmap background = [&]()
	{
		// Paint background
		QPixmap pxmap(static_cast<int>(hues.size()), height);
		pxmap.fill(Qt::white);
		return pxmap;
	}();

	QPixmap histo = background;
	QPainter painter(&histo);

	const cfg_ps_move* config = ::at32(g_cfg_move.move, m_index);
	const u16 hue = config->hue;
	const u16 hue_threshold = config->hue_threshold;
	const int min_hue = hue - hue_threshold;
	const int max_hue = hue + hue_threshold;

	// Paint hue threshold
	painter.fillRect(min_hue, 0, hue_threshold * 2 + 1, histo.height(), Qt::lightGray);

	if (min_hue < 0)
	{
		painter.fillRect(min_hue + 360, 0, hue_threshold * 2 + 1, histo.height(), Qt::lightGray);
	}
	else if (max_hue >= 360)
	{
		painter.fillRect(0, 0, max_hue - 360, histo.height(), Qt::lightGray);
	}

	// Paint target hue
	const auto [r, g, b] = ps_move_tracker<true>::hsv_to_rgb(hue, 1.0f, 1.0f);
	painter.setPen(QColor(r, g, b));
	painter.drawLine(hue, 0, hue, histo.height() - 1);

	// Paint histogram
	painter.setPen(Qt::black);

	const u32 zero_offset = (ignore_zero ? 1 : 0);

	const auto max_elem = std::max_element(hues.begin() + zero_offset, hues.end());
	const u32 max_value = max_elem != hues.end() ? *max_elem : 0u;

	if (!max_value)
		return histo;

	for (int i = zero_offset; i < static_cast<int>(hues.size()); i++)
	{
		const int bar_height = (hues[i] / static_cast<float>(max_value)) * height;
		if (bar_height <= 0) continue;
		painter.drawLine(i, height - 1, i, height - bar_height);
	}

	return histo;
}
