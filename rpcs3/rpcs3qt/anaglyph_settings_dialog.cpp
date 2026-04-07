#include "stdafx.h"
#include "anaglyph_settings_dialog.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>

color_wedge_widget::color_wedge_widget(QWidget* parent)
	: QWidget(parent)
{
	setObjectName("color_wedge_widget");
	setMinimumSize(300, 300);
}

color_wedge_widget::~color_wedge_widget()
{
	stereo_config::s_live_preview_enabled = false;
}

void color_wedge_widget::set_option(stereo_render_mode_options option)
{
	m_stereo_config.set_stereo_mode(option);
	update();
}

void color_wedge_widget::set_custom_matrices(const stereo_config::stereo_matrices& matrices)
{
	m_stereo_config.set_custom_matrices(matrices);
	update();
}

QVector3D color_wedge_widget::apply_matrix(const QVector3D& left, const QVector3D& right, const stereo_config::stereo_matrices& m)
{
	const mat3f& ml = m.left;
	const mat3f& mr = m.right;

	QVector3D out;
	out.setX(ml[0].x * left.x()  + ml[1].x * left.y()  + ml[2].x * left.z() +
	         mr[0].x * right.x() + mr[1].x * right.y() + mr[2].x * right.z());

	out.setY(ml[0].y * left.x()  + ml[1].y * left.y()  + ml[2].y * left.z() +
	         mr[0].y * right.x() + mr[1].y * right.y() + mr[2].y * right.z());

	out.setZ(ml[0].z * left.x()  + ml[1].z * left.y()  + ml[2].z * left.z() +
	         mr[0].z * right.x() + mr[1].z * right.y() + mr[2].z * right.z());

	out.setX(std::clamp(out.x(), 0.0f, 1.0f));
	out.setY(std::clamp(out.y(), 0.0f, 1.0f));
	out.setZ(std::clamp(out.z(), 0.0f, 1.0f));

	return out;
}

QVector3D color_wedge_widget::apply_anaglyph_matrix(const QVector3D& left, const QVector3D& right)
{
	if (m_stereo_config.stereo_mode() == stereo_render_mode_options::disabled)
	{
		return left;
	}

	return apply_matrix(left, right, m_stereo_config.matrices());
}

void color_wedge_widget::paintEvent(QPaintEvent* /*event*/)
{
	const int w = width();
	const int h = height();

	if (m_img.width() != w || m_img.height() != h)
	{
		m_img = QImage(w, h, QImage::Format_RGB32);
	}

	QVector3D out;

	// Loop over pixels
	for (int y = 0; y < h; ++y)
	{
		const float v = float(y) / (h - 1);

		for (int x = 0; x < w; ++x)
		{
			const float u = float(x) / (w - 1);

			const QVector3D left(u, v, 1.0f - u);
			const QVector3D right = left;

			const QVector3D out = apply_anaglyph_matrix(left, right);

			m_img.setPixel(x, y, qRgb(int(out.x() * 255), int(out.y() * 255), int(out.z() * 255)));
		}
	}

	// Paint the image
	QPainter p(this);
	p.drawImage(0, 0, m_img);
}

anaglyph_settings_dialog::anaglyph_settings_dialog(QWidget* parent, std::shared_ptr<emu_settings> emu_settings)
	: QDialog(parent)
	, m_emu_settings(std::move(emu_settings))
{
	setWindowTitle(tr("Anaglyph Settings"));
	setObjectName("anaglyph_settings_dialog");
	setAttribute(Qt::WA_DeleteOnClose);

	stereo_render_mode_options current_mode = stereo_render_mode_options::disabled;
	if (u64 result {}; cfg::try_to_enum_value(&result, &fmt_class_string<stereo_render_mode_options>::format, m_emu_settings->GetSetting(emu_settings_type::StereoRenderMode)))
	{
		const auto mode = static_cast<stereo_render_mode_options>(static_cast<std::underlying_type_t<stereo_render_mode_options>>(result));
		switch (mode)
		{
		case stereo_render_mode_options::anaglyph_red_green:
		case stereo_render_mode_options::anaglyph_red_blue:
		case stereo_render_mode_options::anaglyph_red_cyan:
		case stereo_render_mode_options::anaglyph_magenta_cyan:
		case stereo_render_mode_options::anaglyph_trioscopic:
		case stereo_render_mode_options::anaglyph_amber_blue:
		case stereo_render_mode_options::anaglyph_custom:
			current_mode = mode;
			break;
		default:
			break;
		}
	}

	stereo_config::stereo_matrices custom_matrices {};
	stereo_config::convert_matrix(custom_matrices.left, m_emu_settings->GetMapSetting(emu_settings_type::CustomAnaglyphMatrixLeft));
	stereo_config::convert_matrix(custom_matrices.right, m_emu_settings->GetMapSetting(emu_settings_type::CustomAnaglyphMatrixRight));

	m_widget_reference = new color_wedge_widget(this);
	m_widget_reference->set_option(stereo_render_mode_options::disabled);

	m_widget = new color_wedge_widget(this);
	m_widget->set_option(current_mode);
	m_widget->set_custom_matrices(custom_matrices);

	QPushButton* apply_button = new QPushButton(tr("Apply Custom Matrices"));
	connect(apply_button, &QAbstractButton::clicked, this, [this]()
	{
		m_emu_settings->SetMapSetting(emu_settings_type::CustomAnaglyphMatrixLeft, m_widget->get_stereo_config().get_custom_matrix(true));
		m_emu_settings->SetMapSetting(emu_settings_type::CustomAnaglyphMatrixRight, m_widget->get_stereo_config().get_custom_matrix(false));
	});
	apply_button->setEnabled(current_mode == stereo_render_mode_options::anaglyph_custom);

	QCheckBox* live_preview_cb = new QCheckBox(tr("Live Preview"));
	connect(live_preview_cb, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state)
	{
		stereo_config::s_live_preview_enabled = state == Qt::CheckState::Checked;
	});

	QComboBox* combo = new QComboBox(this);
	combo->addItem(tr("Disabled"), static_cast<int>(stereo_render_mode_options::disabled));
	combo->addItem(tr("Red-Green"), static_cast<int>(stereo_render_mode_options::anaglyph_red_green));
	combo->addItem(tr("Red-Blue"), static_cast<int>(stereo_render_mode_options::anaglyph_red_blue));
	combo->addItem(tr("Red-Cyan"), static_cast<int>(stereo_render_mode_options::anaglyph_red_cyan));
	combo->addItem(tr("Magenta-Cyan"), static_cast<int>(stereo_render_mode_options::anaglyph_magenta_cyan));
	combo->addItem(tr("Trioscopic"), static_cast<int>(stereo_render_mode_options::anaglyph_trioscopic));
	combo->addItem(tr("Amber-Blue"), static_cast<int>(stereo_render_mode_options::anaglyph_amber_blue));
	combo->addItem(tr("Custom"), static_cast<int>(stereo_render_mode_options::anaglyph_custom));
	combo->setCurrentIndex(combo->findData(static_cast<int>(current_mode)));

	connect(combo, &QComboBox::currentIndexChanged, this, [this, combo, apply_button](int /*index*/)
	{
		apply_button->setEnabled(static_cast<stereo_render_mode_options>(combo->currentData().toInt()) == stereo_render_mode_options::anaglyph_custom);
		m_widget->set_option(static_cast<stereo_render_mode_options>(combo->currentData().toInt()));
		update_matrices();
	});

	m_matrix_left = create_matrix_table();
	m_matrix_right = create_matrix_table();

	update_matrices();

	const auto group_box = [](const QString& title, QWidget* widget)
	{
		QHBoxLayout* layout = new QHBoxLayout();
		layout->addWidget(widget);

		QGroupBox* gb = new QGroupBox(title);
		gb->setLayout(layout);
		return gb;
	};

	QHBoxLayout* lay_h_1 = new QHBoxLayout();
	lay_h_1->addWidget(combo);
	lay_h_1->addWidget(apply_button);
	lay_h_1->addWidget(live_preview_cb);
	lay_h_1->addSpacerItem(new QSpacerItem(50, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

	QHBoxLayout* lay_h_2 = new QHBoxLayout();
	lay_h_2->addWidget(group_box(tr("Left matrix"), m_matrix_left));
	lay_h_2->addWidget(group_box(tr("Right matrix"), m_matrix_right));

	QHBoxLayout* lay_h_3 = new QHBoxLayout();
	lay_h_3->addWidget(group_box(tr("Reference"), m_widget_reference));
	lay_h_3->addWidget(group_box(tr("Anaglyph"), m_widget));

	QVBoxLayout* lay = new QVBoxLayout();
	lay->addLayout(lay_h_1);
	lay->addLayout(lay_h_2);
	lay->addLayout(lay_h_3);

	setLayout(lay);
	adjustSize();
}

QTableWidget* anaglyph_settings_dialog::create_matrix_table()
{
	QTableWidget* table = new QTableWidget(3, 3, this);
	table->setVerticalHeaderLabels({tr("R in"), tr("G in"), tr("B in")});
	table->setHorizontalHeaderLabels({tr("R out"), tr("G out"), tr("B out")});
	for (int r = 0; r < 3; ++r)
	{
		for (int c = 0; c < 3; ++c)
		{
			table->setItem(r, c, new QTableWidgetItem("0.0"));
		}
	}
	connect(table, &QTableWidget::cellChanged, this, [this]()
	{
		apply_custom_matrix();
	});

	table->resizeColumnsToContents();
	table->resizeRowsToContents();
	return table;
}

void anaglyph_settings_dialog::read_matrix(mat3f& matrix, QTableWidget* table) const
{
	if (!table) return;

	for (int r = 0; r < 3; ++r)
	{
		for (int c = 0; c < 3; ++c)
		{
			bool ok = false;
			const float val = table->item(r, c)->text().toFloat(&ok);
			matrix[r].rgb[c] = ok ? val : 0.0f;
		}
	}
}

void anaglyph_settings_dialog::update_matrix(QTableWidget* table, const mat3f& matrix, bool is_custom)
{
	if (!table) return;

	for (int r = 0; r < 3; ++r)
	{
		for (int c = 0; c < 3; ++c)
		{
			if (QTableWidgetItem* item = table->item(r, c))
			{
				Qt::ItemFlags flags = item->flags();
				flags.setFlag(Qt::ItemIsEditable, is_custom);

				item->setFlags(flags);
				item->setText(QString::number(matrix[r].rgb[c]));
			}
		}
	}
}

void anaglyph_settings_dialog::update_matrices()
{
	if (!m_widget) return;

	const stereo_config& stereo_cfg = m_widget->get_stereo_config();
	const stereo_config::stereo_matrices& matrices = stereo_cfg.matrices();

	stereo_config::set_live_matrices(matrices);

	const bool is_custom = stereo_cfg.stereo_mode() == stereo_render_mode_options::anaglyph_custom;

	m_updating = true;
	update_matrix(m_matrix_left, matrices.left, is_custom);
	update_matrix(m_matrix_right, matrices.right, is_custom);
	m_updating = false;
}

void anaglyph_settings_dialog::apply_custom_matrix()
{
	if (!m_widget || m_updating) return;

	stereo_config::stereo_matrices m {};
	read_matrix(m.left, m_matrix_left);
	read_matrix(m.right, m_matrix_right);
	m_widget->set_custom_matrices(m);

	stereo_config::set_live_matrices(m);
}
