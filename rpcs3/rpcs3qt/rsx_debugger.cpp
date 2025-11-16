#include "rsx_debugger.h"
#include "gui_settings.h"
#include "qt_utils.h"
#include "table_item_delegate.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/gcm_printing.h"
#include "Emu/RSX/Common/BufferUtils.h"
#include "Emu/RSX/Program/CgBinaryProgram.h"
#include "Utilities/File.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QFont>
#include <QFontDatabase>
#include <QPixmap>
#include <QPushButton>
#include <QFileDialog>
#include <QKeyEvent>
#include <QTimer>
#include <QMenu>
#include <QBuffer>

#include <span>

enum GCMEnumTypes
{
	CELL_GCM_ENUM,
	CELL_GCM_PRIMITIVE_ENUM,
};

LOG_CHANNEL(rsx_debugger, "RSX Debugger");

namespace utils
{
	template <typename T, typename U>
	[[nodiscard]] auto bless(const std::span<U>& span)
	{
		return std::span<T>(bless<T>(span.data()), sizeof(U) * span.size() / sizeof(T));
	}
}

rsx_debugger::rsx_debugger(std::shared_ptr<gui_settings> gui_settings, QWidget* parent)
	: QDialog(parent)
	, m_gui_settings(std::move(gui_settings))
{
	setWindowTitle(tr("RSX Debugger"));
	setObjectName("rsx_debugger");
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(Qt::Window);

	// Fonts and Colors
	QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	mono.setPointSize(8);
	QLabel l("000000000"); // hacky way to get the lineedit to resize properly
	l.setFont(mono);

	// Controls: Breaks
	QPushButton* b_break_frame = new QPushButton(tr("Frame"));
	QPushButton* b_break_text  = new QPushButton(tr("Texture"));
	QPushButton* b_break_draw  = new QPushButton(tr("Draw"));
	QPushButton* b_break_prim  = new QPushButton(tr("Primitive"));
	QPushButton* b_break_inst  = new QPushButton(tr("Command"));
	b_break_frame->setAutoDefault(false);
	b_break_text->setAutoDefault(false);
	b_break_draw->setAutoDefault(false);
	b_break_prim->setAutoDefault(false);
	b_break_inst->setAutoDefault(false);

	QHBoxLayout* hbox_controls_breaks = new QHBoxLayout();
	hbox_controls_breaks->addWidget(b_break_frame);
	hbox_controls_breaks->addWidget(b_break_text);
	hbox_controls_breaks->addWidget(b_break_draw);
	hbox_controls_breaks->addWidget(b_break_prim);
	hbox_controls_breaks->addWidget(b_break_inst);

	QGroupBox* gb_controls_breaks = new QGroupBox(tr("Break on:"));
	gb_controls_breaks->setLayout(hbox_controls_breaks);

	// TODO: This feature is not yet implemented
	b_break_frame->setEnabled(false);
	b_break_text->setEnabled(false);
	b_break_draw->setEnabled(false);
	b_break_prim->setEnabled(false);
	b_break_inst->setEnabled(false);

	QHBoxLayout* hbox_controls = new QHBoxLayout();
	hbox_controls->addWidget(gb_controls_breaks);
	hbox_controls->addStretch(1);

	m_tw_rsx = new QTabWidget();

	// adds a tab containing a list to the tabwidget
	const auto add_rsx_tab = [this, &mono](const QString& tabname, int columns)
	{
		QTableWidget* table = new QTableWidget();
		table->setItemDelegate(new table_item_delegate);
		table->setFont(mono);
		table->setGridStyle(Qt::NoPen);
		table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
		table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
		table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		table->setSelectionBehavior(QAbstractItemView::SelectRows);
		table->verticalHeader()->setVisible(false);
		table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
		table->verticalHeader()->setDefaultSectionSize(16);
		table->horizontalHeader()->setStretchLastSection(true);
		table->setColumnCount(columns);
		m_tw_rsx->addTab(table, tabname);
		return table;
	};

	m_list_captured_frame      = add_rsx_tab(tr("Captured Frame"), 1);
	m_list_captured_draw_calls = add_rsx_tab(tr("Captured Draw Calls"), 1);

	m_list_captured_frame->setHorizontalHeaderLabels(QStringList() << tr("Column"));
	m_list_captured_frame->setColumnWidth(0, 540);

	m_list_captured_draw_calls->setHorizontalHeaderLabels(QStringList() << tr("Draw calls"));
	m_list_captured_draw_calls->setColumnWidth(0, 540);

	// Tools: Tools = Controls + Notebook Tabs
	QVBoxLayout* vbox_tools = new QVBoxLayout();
	vbox_tools->addLayout(hbox_controls);
	vbox_tools->addWidget(m_tw_rsx);

	m_list_index_buffer = new QListWidget();
	m_list_index_buffer->setFont(mono);

	// Panels for displaying the buffers
	m_buffer_colorA  = new Buffer(false, 0, tr("Color Buffer A"), this);
	m_buffer_colorB  = new Buffer(false, 1, tr("Color Buffer B"), this);
	m_buffer_colorC  = new Buffer(false, 2, tr("Color Buffer C"), this);
	m_buffer_colorD  = new Buffer(false, 3, tr("Color Buffer D"), this);
	m_buffer_depth   = new Buffer(false, 4, tr("Depth Buffer"), this);
	m_buffer_stencil = new Buffer(false, 4, tr("Stencil Buffer"), this);
	m_buffer_tex     = new Buffer(true, 4, tr("Texture"), this);

	for (Buffer* buf :
	{
		m_buffer_colorA, m_buffer_colorB, m_buffer_colorC, m_buffer_colorD
		, m_buffer_depth, m_buffer_stencil, m_buffer_tex
	})
	{
		buf->setContextMenuPolicy(Qt::CustomContextMenu);

		connect(buf, &QWidget::customContextMenuRequested, buf, &Buffer::ShowContextMenu);
	}

	QGroupBox* tex_idx_group = new QGroupBox(tr("Texture Index or Address / Format Override"));
	QVBoxLayout* vlayout_tex_idx = new QVBoxLayout(this);
	QHBoxLayout* hbox_idx_line = new QHBoxLayout(this);
	QLineEdit* tex_idx_line = new QLineEdit(this);
	tex_idx_line->setPlaceholderText("00000000");
	tex_idx_line->setFont(mono);
	tex_idx_line->setMaxLength(18);
	tex_idx_line->setFixedWidth(75);
	tex_idx_line->setFocus();
	tex_idx_line->setValidator(new QRegularExpressionValidator(QRegularExpression("^(0[xX])?0*[a-fA-F0-9]{0,8}$"), this));

	QLineEdit* tex_fmt_override_line = new QLineEdit(this);
	tex_fmt_override_line->setPlaceholderText("00");
	tex_fmt_override_line->setFont(mono);
	tex_fmt_override_line->setMaxLength(18);
	tex_fmt_override_line->setFixedWidth(75);
	tex_fmt_override_line->setFocus();
	tex_fmt_override_line->setValidator(new QRegularExpressionValidator(QRegularExpression("^(0[xX])?0*[a-fA-F0-9]{0,2}$"), this));

	hbox_idx_line->addWidget(tex_idx_line);
	hbox_idx_line->addWidget(tex_fmt_override_line);
	vlayout_tex_idx->addLayout(hbox_idx_line);

	m_enabled_textures_label = new QLabel(enabled_textures_text, this);

	vlayout_tex_idx->addWidget(m_enabled_textures_label);
	tex_idx_group->setLayout(vlayout_tex_idx);

	// Merge and display everything
	QVBoxLayout* vbox_buffers1 = new QVBoxLayout();
	vbox_buffers1->addWidget(m_buffer_colorA);
	vbox_buffers1->addWidget(m_buffer_colorC);
	vbox_buffers1->addWidget(m_buffer_depth);
	vbox_buffers1->addWidget(m_buffer_tex);
	vbox_buffers1->addStretch();

	QVBoxLayout* vbox_buffers2 = new QVBoxLayout();
	vbox_buffers2->addWidget(m_buffer_colorB);
	vbox_buffers2->addWidget(m_buffer_colorD);
	vbox_buffers2->addWidget(m_buffer_stencil);
	vbox_buffers2->addWidget(tex_idx_group);
	vbox_buffers2->addStretch();

	QHBoxLayout* buffer_layout = new QHBoxLayout();
	buffer_layout->addLayout(vbox_buffers1);
	buffer_layout->addLayout(vbox_buffers2);
	buffer_layout->addStretch();

	QWidget* buffers = new QWidget();
	buffers->setLayout(buffer_layout);

	auto xp_layout = new QHBoxLayout();
	m_transform_disasm = new QTextEdit(this);
	m_transform_disasm->setReadOnly(true);
	m_transform_disasm->setWordWrapMode(QTextOption::NoWrap);
	m_transform_disasm->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	xp_layout->addWidget(m_transform_disasm);

	auto xp_tab = new QWidget();
	xp_tab->setLayout(xp_layout);

	auto fp_layout = new QHBoxLayout();
	m_fragment_disasm = new QTextEdit(this);
	m_fragment_disasm->setReadOnly(true);
	m_fragment_disasm->setWordWrapMode(QTextOption::NoWrap);
	m_fragment_disasm->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	fp_layout->addWidget(m_fragment_disasm);

	auto fp_tab = new QWidget();
	fp_tab->setLayout(fp_layout);

	QTabWidget* state_rsx = new QTabWidget();
	state_rsx->addTab(buffers, tr("RTTs and DS"));
	state_rsx->addTab(xp_tab, tr("Vertex Program"));
	state_rsx->addTab(fp_tab, tr("Fragment Program"));
	state_rsx->addTab(m_list_index_buffer, tr("Index Buffer"));

	QHBoxLayout* main_layout = new QHBoxLayout();
	main_layout->addLayout(vbox_tools, 1);
	main_layout->addWidget(state_rsx, 1);
	setLayout(main_layout);

	connect(m_list_captured_draw_calls, &QTableWidget::itemClicked, this, &rsx_debugger::OnClickDrawCalls);

	connect(tex_idx_line, &QLineEdit::textChanged, [this](const QString& text)
	{
		bool ok = false;
		const u32 addr = (text.startsWith("0x", Qt::CaseInsensitive) ? text.right(text.size() - 2) : text).toULong(&ok, 16);
		if (ok)
		{
			m_cur_texture = addr;
		}
	});

	connect(tex_fmt_override_line, &QLineEdit::textChanged, [this](const QString& text)
	{
		bool ok = false;
		const u32 fmt = (text.startsWith("0x", Qt::CaseInsensitive) ? text.right(text.size() - 2) : text).toULong(&ok, 16);
		if (ok)
		{
			m_texture_format_override = fmt;
		}
	});

	// Restore header states
	QVariantMap states = m_gui_settings->GetValue(gui::rsx_states).toMap();
	for (int i = 0; i < m_tw_rsx->count(); i++)
		(static_cast<QTableWidget*>(m_tw_rsx->widget(i)))->horizontalHeader()->restoreState(states[QString::number(i)].toByteArray());

	// Fill the frame
	for (u32 i = 0; i < frame_debug.command_queue.size(); i++)
		m_list_captured_frame->insertRow(i);

	restoreGeometry(m_gui_settings->GetValue(gui::rsx_geometry).toByteArray());

	// Check for updates every ~100 ms
	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &rsx_debugger::UpdateInformation);
	timer->start(100);
}

void rsx_debugger::closeEvent(QCloseEvent* event)
{
	// Save header states and window geometry
	QVariantMap states;
	for (int i = 0; i < m_tw_rsx->count(); i++)
		states[QString::number(i)] = (static_cast<QTableWidget*>(m_tw_rsx->widget(i)))->horizontalHeader()->saveState();

	m_gui_settings->SetValue(gui::rsx_states, states, false);
	m_gui_settings->SetValue(gui::rsx_geometry, saveGeometry(), true);

	QDialog::closeEvent(event);
}

void rsx_debugger::keyPressEvent(QKeyEvent* event)
{
	if (isActiveWindow() && !event->isAutoRepeat())
	{
		switch (event->key())
		{
		case Qt::Key_F5: UpdateInformation(); break;
		default: break;
		}
	}

	QDialog::keyPressEvent(event);
}

bool rsx_debugger::eventFilter(QObject* object, QEvent* event)
{
	if (Buffer* buffer = qobject_cast<Buffer*>(object))
	{
		switch (event->type())
		{
		case QEvent::MouseButtonDblClick:
		{
			buffer->ShowWindowed();
			break;
		}
		default:
			break;
		}
	}

	return QDialog::eventFilter(object, event);
}

Buffer::Buffer(bool isTex, u32 id, const QString& name, QWidget* parent)
	: QGroupBox(name, parent)
	, m_id(id)
	, m_isTex(isTex)
{
	m_image_size = isTex ? Texture_Size : Panel_Size;

	m_canvas = new QLabel();
	m_canvas->setFixedSize(m_image_size);

	QHBoxLayout* layout = new QHBoxLayout();
	layout->setContentsMargins(1, 1, 1, 1);
	layout->addWidget(m_canvas);
	setLayout(layout);

	installEventFilter(parent);
}

// Draws a formatted and buffered <image> inside the Buffer Widget
void Buffer::showImage(QImage&& image)
{
	if (image.isNull())
	{
		m_canvas->clear();
		return;
	}

	m_image = std::move(image);
	const QImage scaled = m_image.scaled(m_image_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_canvas->setPixmap(QPixmap::fromImage(scaled));

	QHBoxLayout* new_layout = new QHBoxLayout();
	new_layout->setContentsMargins(1, 1, 1, 1);
	new_layout->addWidget(m_canvas);
	delete layout();
	setLayout(new_layout);
}

void Buffer::ShowWindowed()
{
	//const auto render = rsx::get_current_renderer();
	if (m_image.isNull())
		return;

	// TODO: Is there any better way to choose the color buffers
	//if (0 <= m_id && m_id < 4)
	//{
	//	const auto buffers = render->display_buffers;
	//	u32 addr = rsx::constants::local_mem_base + buffers[m_id].offset;
	//	if (vm::check_addr(addr) && buffers[m_id].width && buffers[m_id].height)
	//		memory_viewer_panel::ShowImage(this, addr, 3, buffers[m_id].width, buffers[m_id].height, true);
	//	return;
	//}

	gui::utils::show_windowed_image(m_image.copy(), QString("%1 #%2").arg(title()).arg(m_window_counter++));

	//if (m_isTex)
	//{
	//	u8 location = render->textures[m_cur_texture].location();
	//	if (location <= 1 && vm::check_addr(rsx::get_address(render->textures[m_cur_texture].offset(), location))
	//		&& render->textures[m_cur_texture].width() && render->textures[m_cur_texture].height())
	//		memory_viewer_panel::ShowImage(this,
	//			rsx::get_address(render->textures[m_cur_texture].offset(), location), 1,
	//			render->textures[m_cur_texture].width(),
	//			render->textures[m_cur_texture].height(), false);
	//}
}

void Buffer::ShowContextMenu(const QPoint& pos)
{
	if (m_image.isNull())
		return;

	QMenu context_menu(this);

	QAction action(tr("Save Image At"), this);
	connect(&action, &QAction::triggered, this, [&]()
	{
		if (m_image.isNull())
			return;

		const QString path = QFileDialog::getSaveFileName(this, tr("Save Image"), "", "Image (*.png)");

		if (path.isEmpty())
			return;

		if (m_image.save(path, "PNG", 100))
		{
			rsx_debugger.success("Saved image to '%s'", path);
		}
		else
		{
			rsx_debugger.error("Failure to save image to '%s'", path);
		}
	});

	context_menu.addAction(&action);
	context_menu.exec(mapToGlobal(pos));
}

namespace
{
	f32 f16_to_f32(f16 val)
	{
		// See http://stackoverflow.com/a/26779139
		// The conversion doesn't handle NaN/Inf

		const u16 _u16 = static_cast<u16>(val);
		const u32 raw = ((_u16 & 0x8000) << 16) |             // Sign (just moved)
		                (((_u16 & 0x7c00) + 0x1C000) << 13) | // Exponent ( exp - 15 + 127)
		                ((_u16 & 0x03FF) << 13);              // Mantissa

		return std::bit_cast<f32>(raw);
	}

	std::array<u8, 3> get_value(std::span<const std::byte> orig_buffer, rsx::surface_color_format format, usz idx)
	{
		switch (format)
		{
		case rsx::surface_color_format::b8:
		{
			const u8 value = read_from_ptr<u8>(orig_buffer, idx);
			return{ value, value, value };
		}
		case rsx::surface_color_format::x32:
		{
			const be_t<u32> stored_val = read_from_ptr<be_t<u32>>(orig_buffer, idx);
			const u32 swapped_val = stored_val;
			const f32 float_val = std::bit_cast<f32>(swapped_val);
			const u8 val = float_val * 255.f;
			return{ val, val, val };
		}
		case rsx::surface_color_format::a8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		{
			const auto ptr = reinterpret_cast<const u8*>(orig_buffer.data());
			return{ ptr[1 + idx * 4], ptr[2 + idx * 4], ptr[3 + idx * 4] };
		}
		case rsx::surface_color_format::a8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		{
			const auto ptr = reinterpret_cast<const u8*>(orig_buffer.data());
			return{ ptr[3 + idx * 4], ptr[2 + idx * 4], ptr[1 + idx * 4] };
		}
		case rsx::surface_color_format::w16z16y16x16:
		{
			const auto ptr = utils::bless<const u16>(orig_buffer);
			const f16 h0 = static_cast<f16>(ptr[4 * idx]);
			const f16 h1 = static_cast<f16>(ptr[4 * idx + 1]);
			const f16 h2 = static_cast<f16>(ptr[4 * idx + 2]);
			const f32 f0 = f16_to_f32(h0);
			const f32 f1 = f16_to_f32(h1);
			const f32 f2 = f16_to_f32(h2);

			const u8 val0 = f0 * 255.f;
			const u8 val1 = f1 * 255.f;
			const u8 val2 = f2 * 255.f;
			return{ val0, val1, val2 };
		}
		case rsx::surface_color_format::g8b8:
		case rsx::surface_color_format::r5g6b5:
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		case rsx::surface_color_format::w32z32y32x32:
		default:
			fmt::throw_exception("Unsupported format for display");
		}
	}

	/**
	 * Fill a buffer that can be passed to QImage.
	 */
	void convert_to_QImage_buffer(rsx::surface_color_format format, std::span<const std::byte> orig_buffer, std::vector<u8>& buffer, usz width, usz height) noexcept
	{
		buffer.resize(width * height * 4);

		if (buffer.empty())
		{
			return;
		}

		for (u32 i = 0; i < width * height; i++)
		{
			// depending on original buffer, the colors may need to be reversed
			const auto &colors = get_value(orig_buffer, format, i);
			buffer[0 + i * 4] = colors[0];
			buffer[1 + i * 4] = colors[1];
			buffer[2 + i * 4] = colors[2];
			buffer[3 + i * 4] = 255;
		}
	}
}

void rsx_debugger::OnClickDrawCalls()
{
	const usz draw_id = m_list_captured_draw_calls->currentRow();

	const auto& draw_call = frame_debug.draw_calls[draw_id];

	Buffer* buffers[] =
	{
		m_buffer_colorA,
		m_buffer_colorB,
		m_buffer_colorC,
		m_buffer_colorD,
	};

	const u32 width = draw_call.state.surface_clip_width();
	const u32 height = draw_call.state.surface_clip_height();

	for (usz i = 0; i < 4; i++)
	{
		if (width && height && !draw_call.color_buffer[i].empty())
		{
			std::vector<u8>& buffer = buffers[i]->cache;
			convert_to_QImage_buffer(draw_call.state.surface_color(), draw_call.color_buffer[i], buffer, width, height);
			buffers[i]->showImage(QImage(buffer.data(), static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32));
		}
	}

	// Buffer Z
	{
		if (width && height && !draw_call.depth_stencil[0].empty())
		{
			const std::span<const std::byte> orig_buffer = draw_call.depth_stencil[0];
			m_buffer_depth->cache.resize(4ULL * width * height);
			const auto buffer = m_buffer_depth->cache.data();

			if (draw_call.state.surface_depth_fmt() == rsx::surface_depth_format::z24s8)
			{
				for (u32 row = 0; row < height; row++)
				{
					for (u32 col = 0; col < width; col++)
					{
						const u32 depth_val = utils::bless<const u32>(orig_buffer)[row * width + col];
						const u8 displayed_depth_val = 255 * depth_val / 0xFFFFFF;
						buffer[4 * col + 0 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 1 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 2 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 3 + width * row * 4] = 255;
					}
				}
			}
			else
			{
				for (u32 row = 0; row < height; row++)
				{
					for (u32 col = 0; col < width; col++)
					{
						const u16 depth_val = utils::bless<const u16>(orig_buffer)[row * width + col];
						const u8 displayed_depth_val = 255 * depth_val / 0xFFFF;
						buffer[4 * col + 0 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 1 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 2 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 3 + width * row * 4] = 255;
					}
				}
			}
			m_buffer_depth->showImage(QImage(buffer, static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32));
		}
	}

	// Buffer S
	{
		if (width && height && !draw_call.depth_stencil[1].empty())
		{
			const std::span<const std::byte> orig_buffer = draw_call.depth_stencil[1];
			std::vector<u8>& buffer = m_buffer_stencil->cache;
			buffer.resize(4ULL * width * height);

			for (u32 row = 0; row < height; row++)
			{
				for (u32 col = 0; col < width; col++)
				{
					const u8 stencil_val = reinterpret_cast<const u8*>(orig_buffer.data())[row * width + col];
					buffer[4 * col + 0 + width * row * 4] = stencil_val;
					buffer[4 * col + 1 + width * row * 4] = stencil_val;
					buffer[4 * col + 2 + width * row * 4] = stencil_val;
					buffer[4 * col + 3 + width * row * 4] = 255;
				}
			}
			m_buffer_stencil->showImage(QImage(buffer.data(), static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32));
		}
	}

	// Programs
	m_transform_disasm->clear();
	m_transform_disasm->setText(QString::fromStdString(frame_debug.draw_calls[draw_id].programs.first));
	m_fragment_disasm->clear();
	m_fragment_disasm->setText(QString::fromStdString(frame_debug.draw_calls[draw_id].programs.second));

	m_list_index_buffer->clear();
	//m_list_index_buffer->insertColumn(0, "Index", 0, 700);
	if (frame_debug.draw_calls[draw_id].state.index_type() == rsx::index_array_type::u16)
	{
		auto index_buffer = ref_ptr<u16[]>(frame_debug.draw_calls[draw_id].index);
		for (u32 i = 0; i < frame_debug.draw_calls[draw_id].vertex_count; ++i)
		{
			m_list_index_buffer->insertItem(i, QString::fromStdString(std::to_string(index_buffer[i])));
		}
	}
	if (frame_debug.draw_calls[draw_id].state.index_type() == rsx::index_array_type::u32)
	{
		auto index_buffer = ref_ptr<u32[]>(frame_debug.draw_calls[draw_id].index);
		for (u32 i = 0; i < frame_debug.draw_calls[draw_id].vertex_count; ++i)
		{
			m_list_index_buffer->insertItem(i, QString::fromStdString(std::to_string(index_buffer[i])));
		}
	}
}

void rsx_debugger::UpdateInformation() const
{
	GetMemory();
	GetBuffers();
	GetVertexProgram();
	GetFragmentProgram();
}

void rsx_debugger::GetMemory() const
{
	std::string dump;
	u32 cmd_i = 0;

	std::string str;

	for (const auto& command : frame_debug.command_queue)
	{
		str.clear();
		rsx::get_pretty_printing_function(command.first)(str, command.first, command.second);
		m_list_captured_frame->setItem(cmd_i++, 0, new QTableWidgetItem(QString::fromStdString(str)));

		dump += str;
		dump += '\n';
	}

	if (!dump.empty())
	{
		fs::write_file(fs::get_cache_dir() + "command_dump.log", fs::rewrite, dump);
	}

	for (u32 i = 0; i < frame_debug.draw_calls.size(); i++)
		m_list_captured_draw_calls->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(frame_debug.draw_calls[i].name)));
}

void rsx_debugger::GetBuffers() const
{
	const auto render = rsx::get_current_renderer();
	if (!render || !render->is_initialized || !render->local_mem_size || !render->is_paused())
	{
		return;
	}

	const auto addrs = render->get_color_surface_addresses();

	// Color buffers
	for (u32 buffer_it = 0; buffer_it < addrs.size(); buffer_it++)
	{
		const u32 rsx_buffer_addr = addrs[buffer_it];
		const u32 width = rsx::method_registers.surface_clip_width();
		const u32 pitch = rsx::method_registers.surface_pitch(buffer_it);
		const u32 height = rsx::method_registers.surface_clip_height();

		Buffer* panel{};
		switch (buffer_it)
		{
		case 0:  panel = m_buffer_colorA; break;
		case 1:  panel = m_buffer_colorB; break;
		case 2:  panel = m_buffer_colorC; break;
		default: panel = m_buffer_colorD; break;
		}

		if (!height || !pitch)
		{
			panel->showImage(QImage());
			continue;
		}

		u32 bpp = 4;
		QImage::Format format = QImage::Format_RGB32;
		const auto fmt = rsx::method_registers.surface_color();
		bool bswap = true;
		u32 dst_bpp = 0;

		switch (fmt)
		{
		//case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		//case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::r5g6b5:
		{
			format = QImage::Format_RGB16;
			bpp = 2;
			break;
		}
		//case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		//case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::a8r8g8b8:
		{
			// For now ignore alpha channel because it has a tendency of being 0
			//format = QImage::Format_ARGB32;
			break;
		}
		case rsx::surface_color_format::b8:
		{
			format = QImage::Format_Grayscale8;
			bpp = 1;
			break;
		}
		case rsx::surface_color_format::g8b8:
		{
			bpp = 2;
			dst_bpp = 4;
			format = QImage::Format_RGBA8888;
			bswap = false;
			break;
		}
		//case rsx::surface_color_format::w16z16y16x16,
		//case rsx::surface_color_format::w32z32y32x32,
		//case rsx::surface_color_format::x32,
		//case rsx::surface_color_format::x8b8g8r8_z8b8g8r8,
		//case rsx::surface_color_format::x8b8g8r8_o8b8g8r8,
		case rsx::surface_color_format::a8b8g8r8:
		{
			format = QImage::Format_RGBA8888;
			break;
		}
		default:
		{
			break;
		}
		}

		// PS3 buffer size (for memory validation)
		const u32 src_mem_size = pitch * (height - 1) + width * bpp;

		if ((height > 1 && pitch < width * bpp) || !src_mem_size || !vm::check_addr(rsx_buffer_addr, vm::page_readable, src_mem_size))
		{
			panel->showImage(QImage());
			continue;
		}

		// Touch RSX memory to potentially flush GPU memory (must occur in named_thread)
		[[maybe_unused]] auto buffer_touch_1 = named_thread("RSX Buffer Touch"sv, [&]()
		{
			for (u32 page_start = rsx_buffer_addr & -4096; page_start < rsx_buffer_addr + src_mem_size; page_start += 4096)
			{
				static_cast<void>(vm::_ref<atomic_t<u8>>(page_start).load());
			}
		});

		bswap ^= std::endian::native == std::endian::big;

		if (!dst_bpp)
		{
			dst_bpp = bpp;
		}

		const auto rsx_buffer = vm::get_super_ptr<const u8>(rsx_buffer_addr);
		panel->cache.resize(std::max<usz>(panel->cache.size(), width * height * 16));
		const auto buffer = panel->cache.data();

		if (dst_bpp == bpp && pitch == width * bpp)
		{
			std::memcpy(buffer, rsx_buffer, src_mem_size);
		}
		else
		{
			for (u32 y = 0; y < height; y++)
			{
				const usz line_start = y * pitch;
				std::memcpy(buffer + y * width * dst_bpp, rsx_buffer + line_start, width * bpp);
			}
		}

		switch (fmt)
		{
		case rsx::surface_color_format::g8b8:
		{
			for (u32 y = 0; y < height; y++)
			{
				for (u32 x = 0; x < std::max(pitch, 1u) - 1; x += 2)
				{
					const usz line_start = y * pitch;

					std::memcpy(buffer + line_start * 2 + x * 2 + 1, buffer + line_start * 2 + x, 2);
					buffer[line_start * 2 + x * 2 + 0] = 0;
					buffer[line_start * 2 + x * 2 + 3] = 0xff;
				}
			}

			break;
		}
		case rsx::surface_color_format::a8b8g8r8:
		{
			format = QImage::Format_RGBA8888;
			bswap = true;
			break;
		}
		case rsx::surface_color_format::a8r8g8b8:
		{
			break;
		}
		default:
		{
			break;
		}
		}

		if (bswap && bpp != 1)
		{
			if (bpp == 2)
			{
				for (u32 i = 0; i < panel->cache.size(); i += 2)
				{
					u16 res{};
					std::memcpy(&res, buffer + i, 2);
					res = stx::se_storage<u16>::swap(res);
					std::memcpy(buffer + i, &res, 2);
				}
			}
			else if (ensure(bpp == 4))
			{
				for (u32 i = 0; i < panel->cache.size(); i += 4)
				{
					u32 res{};
					std::memcpy(&res, buffer + i, 4);
					res = stx::se_storage<u32>::swap(res);
					std::memcpy(buffer + i, &res, 4);
				}
			}
		}

		panel->showImage(QImage(panel->cache.data(), width, height, format));
	}

	// One iteration for depth, second for stencil
	for (u32 buffer_it = 0; buffer_it < 2; buffer_it++)
	{
		const u32 rsx_buffer_addr = render->get_zeta_surface_address();

		const u32 width  = rsx::method_registers.surface_clip_width();
		const u32 height = rsx::method_registers.surface_clip_height();
		const u32 pitch = rsx::method_registers.surface_z_pitch();

		u32 bpp = 4;
		QImage::Format format = QImage::Format_Grayscale16;
		const auto fmt = rsx::method_registers.surface_depth_fmt();
		bool has_stencil = false;
		bool is_stencil = buffer_it == 1;
		bool is_float = false;

		switch (fmt)
		{
		case rsx::surface_depth_format2::z16_float:
		{
			is_float = true;
			[[fallthrough]];
		}
		case rsx::surface_depth_format2::z16_uint:
		{
			if (is_stencil)
			{
				continue;
			}

			bpp = 2;
			break;
		}
		case rsx::surface_depth_format2::z24s8_float:
		{
			is_float = true;
			[[fallthrough]];
		}
		case rsx::surface_depth_format2::z24s8_uint:
		{
			format = QImage::Format_RGB16;
			has_stencil = true;
			break;
		}
		default:
		{
			fmt::throw_exception("Unreachable!");
			break;
		}
		}

		// PS3 buffer size (for memory validation)
		const u32 src_mem_size = pitch * (height - 1) + width * bpp;

		Buffer* panel{};
		switch (buffer_it)
		{
		case 0:  panel = m_buffer_depth; break;
		default: panel = m_buffer_stencil; break;
		}

		if ((height > 1 && pitch < width * bpp) || !height || !src_mem_size || !vm::check_addr(rsx_buffer_addr, vm::page_readable, src_mem_size))
		{
			panel->showImage(QImage());
			continue;
		}

		// Touch RSX memory to potentially flush GPU memory (must occur in named_thread)
		[[maybe_unused]] auto buffer_touch_2 = named_thread("RSX Buffer Touch"sv, [&]()
		{
			for (u32 page_start = rsx_buffer_addr & -4096; page_start < rsx_buffer_addr + src_mem_size; page_start += 4096)
			{
				static_cast<void>(vm::_ref<atomic_t<u8>>(page_start).load());
			}
		});

		const auto rsx_buffer = vm::get_super_ptr<const u8>(rsx_buffer_addr);
		panel->cache.resize(std::max<usz>(panel->cache.size(), width * height * 4));
		const auto buffer = panel->cache.data();

		if (is_stencil)
		{
			format = QImage::Format_Grayscale8;

			for (u32 y = 0; y < height; y++)
			{
				for (u32 x = 0; x < width; x++)
				{
					const usz line_start = y * pitch;
					std::memcpy(buffer + y * width + x, rsx_buffer + line_start + x * 4 + 3, 1);
				}
			}
		}
		else if (has_stencil)
		{
			format = QImage::Format_Grayscale16;

			if (false && is_float)
			{
				// TODO
			}
			else
			{
				for (u32 y = 0; y < height; y++)
				{
					for (u32 x = 0; x < width; x++)
					{
						const usz line_start = y * pitch;

						be_t<u32> data{};
						std::memcpy(&data, rsx_buffer + line_start + x * 4, 4);
						const u16 res = static_cast<u16>(u64{data / 256} * 0xFFFF / 0xFFFFFF);
						std::memcpy(buffer + y * width * 2 + x * 2, &res, 2);
					}
				}
			}
		}
		else
		{
			format = QImage::Format_Grayscale16;

			for (u32 y = 0; y < height; y++)
			{
				for (u32 x = 0; x < width; x++)
				{
					be_t<u16> data{};
					std::memcpy(&data, rsx_buffer + pitch * y + x * 2, 2);
					const u16 res = is_float ? static_cast<u16>(f16_to_f32(static_cast<f16>(+data))) : +data;
					std::memcpy(buffer + y * width * 2 + x * 2, &res, 2);
				}
			}
		}

		panel->showImage(QImage(panel->cache.data(), width, height, format));
	}

	auto& textures = rsx::method_registers.fragment_textures;

	u32 texture_addr = m_cur_texture;
	u32 tex_fmt_raw = 0;
	u32 tex_fmt = 0;

	u32 width = 1280;
	u32 height = 720;
	u32 pitch = 0;

	if (texture_addr < 16)
	{
		if (textures[texture_addr].enabled())
		{
			width = textures[texture_addr].width();
			height = textures[texture_addr].height();
			pitch = textures[texture_addr].pitch();
			tex_fmt_raw = textures[texture_addr].format();
			tex_fmt = tex_fmt_raw & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN); // TOOD: Support these flags
			texture_addr = rsx::get_address(textures[texture_addr].offset(), textures[texture_addr].location(), 1);
		}
	}

	QString text;

	for (u32 i = 0; i < textures.size(); i++)
	{
		// Technically it can also check if used by shader but idk
		if (textures[i].enabled() && textures[i].width())
		{
			if (!text.isEmpty())
			{
				text += QStringLiteral(", ");
			}

			text += QStringLiteral("%0").arg(i);
		}
	}

	m_enabled_textures_label->setText(enabled_textures_text + text);

	if (m_texture_format_override)
	{
		tex_fmt = m_texture_format_override;
	}

	u32 bytes_per_block = 4;
	u32 pixels_per_block = 1;
	bool bswap = true;

	// Naturally only a handful of common formats are implemented at the moment

	switch (tex_fmt)
	{
	case CELL_GCM_TEXTURE_B8:
	{
		bytes_per_block = 1;
		break;
	}
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	{
		bytes_per_block = 8;
		pixels_per_block = 16;
		break;
	}
	default:
	{
		break;
	}
	}

	const u32 line_bytes_count = width * bytes_per_block / pixels_per_block;

	if (!pitch)
	{
		pitch = line_bytes_count;
	}

	// PS3 buffer size (for memory validation)
	const u32 src_mem_size = pitch * (height - 1) + line_bytes_count;

	if (!src_mem_size || !height || !vm::check_addr(texture_addr, vm::page_readable, src_mem_size))
	{
		m_buffer_tex->showImage(QImage());
		return;
	}

	[[maybe_unused]] auto buffer_touch_3 = named_thread("RSX Buffer Touch"sv, [&]()
	{
		// Must touch every page
		for (u32 i = texture_addr & -4096; i < texture_addr + src_mem_size; i += 4096)
		{
			static_cast<void>(vm::_ref<atomic_t<u8>>(i).load());
		}
	});

	const auto rsx_buffer = vm::get_super_ptr<const u8>(texture_addr);
	m_buffer_tex->cache.resize(std::max<usz>(m_buffer_tex->cache.size(), width * height * 16 + pitch));

	const auto buffer = m_buffer_tex->cache.data();
	if (pixels_per_block != 1 || pitch == line_bytes_count)
	{
		std::memcpy(buffer, rsx_buffer, src_mem_size);
	}
	else
	{
		for (u32 y = 0; y < height; y++)
		{
			std::memcpy(buffer + y * line_bytes_count, rsx_buffer + y * pitch, line_bytes_count);
		}
	}

	switch (tex_fmt)
	{
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	{
		bswap = false;

		// Compressed texture to RGB
		for (usz i = 0; i < width * height / 16; i++)
		{
			le_t<u16> color0{}, color1{};
			std::memcpy(&color0, rsx_buffer + i * 8, 2);
			std::memcpy(&color1, rsx_buffer + i * 8 + 2, 2);

			be_t<u32> control{};
			std::memcpy(&control, rsx_buffer + i * 8 + 4, 4);

			for (u32 sub = 0; sub < 16; sub++)
			{
				const u32 code = (+control >> (sub * 2)) & 3;

				auto convert_to_rgb888 = [](u32 input) -> std::tuple<u32, u32, u32>
				{
					u32 r = ((input >> 11) % 32) * 255 / 32;
					u32 g = ((input >> 5) % 64) * 255 / 64;
					u32 b = (input % 32) * 255 / 32;
					return std::make_tuple(r, g, b);
				};

				const auto [r0, g0, b0] = convert_to_rgb888(color0);
				const auto [r1, g1, b1] = convert_to_rgb888(color1);

				auto compute_color = [&](u32 a0, u32 a1, u8 shift) -> u32
				{
					u32 a0_portion = 1;
					u32 a1_portion = 1;

					switch (code)
					{
					case 0:
					{
						return a0; // Color0 only
					}
					case 1:
					{
						return a1; // Color1 only
					}
					case 2:
					case 3:
					{
						if (a0 > a1)
						{
							if (code == 3)
							{
								a1_portion = 2;
							}
							else
							{
								a0_portion = 2;
							}
						}
						else if (code == 3)
						{
							return 0;
						} // otherwise is the default values

						break;
					}
					default:
					{
						fmt::throw_exception("Unreachable!");
						break;
					}
					}

					return ((a1 * a1_portion + a0 * a0_portion) / (a0_portion + a1_portion)) << shift;
				};

				const le_t<u32> dest_color = compute_color(r0, r1, 0) + compute_color(g0, g1, 8) + compute_color(b0, b1, 16) + (255 << 24);
				std::memcpy(buffer + ((i % (width / 4) * 4 + sub % 4) + ((i / (width / 4) * 4 + sub / 4) * width)) * sizeof(u32), &dest_color, 4);
			}
		}
		break;
	}
	default:
	{
		break;
	}
	}

	if (bswap && bytes_per_block != 1 && pixels_per_block == 1)
	{
		if (bytes_per_block == 2)
		{
			for (u32 i = 0; i < m_buffer_tex->cache.size(); i += 2)
			{
				u16 res{};
				std::memcpy(&res, buffer + i, 2);
				res = stx::se_storage<u16>::swap(res);
				std::memcpy(buffer + i, &res, 2);
			}
		}
		else if (ensure(bytes_per_block == 4))
		{
			for (u32 i = 0; i < m_buffer_tex->cache.size(); i += 4)
			{
				u32 res{};
				std::memcpy(&res, buffer + i, 4);
				res = stx::se_storage<u32>::swap(res);
				std::memcpy(buffer + i, &res, 4);
			}
		}
	}

	m_buffer_tex->showImage(QImage(m_buffer_tex->cache.data(), width, height, QImage::Format_RGB32));
}

void rsx_debugger::GetVertexProgram() const
{
	const auto render = rsx::get_current_renderer();
	if (!render || !render->is_initialized || !render->local_mem_size || !render->is_paused())
	{
		return;
	}

	RSXVertexProgram vp;
	vp.data.reserve(512 * 4);

	const u32 vp_entry = rsx::method_registers.transform_program_start();
	program_hash_util::vertex_program_utils::analyse_vertex_program
	(
		rsx::method_registers.transform_program.data(),  // Input raw block
		vp_entry,                                       // Address of entry point
		vp                                          // [out] Program object
	);

	const u32 ucode_len = static_cast<u32>(vp.data.size() * sizeof(u32));
	std::vector<u32> vp_blob = {
		7003u,             // Type
		6u,                // Revision
		56u + ucode_len,   // Total size
		0,                 // paramCount
		0,                 // paramArray
		32u,               // Program header offset
		ucode_len,         // Ucode length
		56u,               // Ucode start

		::size32(vp.data) / 4, // Instruction count
		0,                     // Slot
		16u,                   // Registers used
		rsx::method_registers.vertex_attrib_input_mask(),
		rsx::method_registers.vertex_attrib_output_mask(),
		rsx::method_registers.clip_planes_mask()
	};

	vp_blob.reserve(vp_blob.size() + vp.data.size());
	vp_blob.insert(vp_blob.end(), vp.data.begin(), vp.data.end());

	std::span<u32> vp_binary(vp_blob);
	CgBinaryDisasm vp_disasm(vp_binary);
	vp_disasm.BuildShaderBody(false);

	m_transform_disasm->clear();
	m_transform_disasm->setText(QString::fromStdString(vp_disasm.GetArbShader()));
}

void rsx_debugger::GetFragmentProgram() const
{
	const auto render = rsx::get_current_renderer();
	if (!render || !render->is_initialized || !render->local_mem_size || !render->is_paused())
	{
		return;
	}

	const auto [program_offset, program_location] = rsx::method_registers.shader_program_address();
	const auto address = rsx::get_address(program_offset, program_location, 4);
	if (!address)
	{
		m_fragment_disasm->clear();
		return;
	}

	// NOTE: Reading through super ptr while crash-safe means we're probably reading incorrect bytes, but should be fine in 99% of cases
	auto data_ptr = vm::get_super_ptr(address);
	const auto fp_metadata = program_hash_util::fragment_program_utils::analyse_fragment_program(data_ptr);

	const bool output_h0 = rsx::method_registers.shader_control() & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? false : true;
	const bool depth_replace = rsx::method_registers.shader_control() & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT ? true : false;
	const u32 flags = 16u /* Reg count */ | ((output_h0 /* 16-bit exports? */ ? 1u : 0u) << 8u) | ((depth_replace /* Export depth? */ ? 1u : 0u) << 16u) | (0u /* Uses KILL? */ << 24u);
	const auto ucode_len = fp_metadata.program_ucode_length;

	std::vector<u32> blob = {
		7004u,             // Type
		6u,                // Revision
		56u + ucode_len,   // Total size
		0,                 // paramCount
		0,                 // paramArray
		32u,               // Program header offset
		ucode_len,         // Ucode length
		56u,               // Ucode start

		ucode_len / 16,                                               // Instruction count
		rsx::method_registers.vertex_attrib_output_mask(),            // Slot
		0u,                                                           // Partial load
		0u | (rsx::method_registers.texcoord_control_mask() << 16u),  // Texcoord input mask | tex2d control
		0u | (flags << 16u),                                          // Centroid inputs (xor tex2d control) | flags (regs, 16-bit, fragDepth, KILL)

		0u,                                                           // Padding
	};

	// Copy in the program bytes, swapped
	const u32 start_offset_in_words = fp_metadata.program_start_offset / 4;
	const u32 ucode_length_in_words = fp_metadata.program_ucode_length / 4;
	blob.resize(blob.size() + ucode_length_in_words);
	copy_data_swap_u32(blob.data() + 14, utils::bless<u32>(data_ptr) + start_offset_in_words, ucode_length_in_words);
	//std::memcpy(blob.data() + 14, utils::bless<char>(data_ptr) + fp_metadata.program_start_offset, fp_metadata.program_ucode_length);

	std::span<u32> fp_binary(blob);
	CgBinaryDisasm fp_disasm(fp_binary);
	fp_disasm.BuildShaderBody(false);

	m_fragment_disasm->clear();
	m_fragment_disasm->setText(QString::fromStdString(fp_disasm.GetArbShader()));
}
