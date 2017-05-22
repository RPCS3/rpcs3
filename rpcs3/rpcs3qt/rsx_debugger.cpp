
#include "rsx_debugger.h"

enum GCMEnumTypes
{
	CELL_GCM_ENUM,
	CELL_GCM_PRIMITIVE_ENUM,
};

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

rsx_debugger::rsx_debugger(QWidget* parent) 
	: QDialog(parent)
	, m_item_count(37)
	, m_addr(0x0)
	, m_cur_texture(0)
	, exit(false)
{
	setWindowTitle(tr("RSX Debugger"));
	setAttribute(Qt::WA_DeleteOnClose);

	//Fonts and Colors
	pSize = 8;
	mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	mono.setPointSize(pSize);
	fontMetrics = new QFontMetrics(mono);
	palette_bg = new QPalette();
	palette_bg->setColor(backgroundRole(), QColor(240, 240, 240));
	QHBoxLayout* hbox_panel = new QHBoxLayout();

	//Tools
	QVBoxLayout* vbox_tools = new QVBoxLayout();

	// Controls
	QGroupBox* gb_controls = new QGroupBox(tr("RSX Debugger Controls"), this);
	QHBoxLayout* hbox_controls = new QHBoxLayout();

	// Controls: Address
	QGroupBox* gb_controls_addr = new QGroupBox(tr("Address:"), this);
	QHBoxLayout* hbox_controls_addr = new QHBoxLayout();
	t_addr = new QLineEdit();
	t_addr->setFont(mono);
	t_addr->setPlaceholderText("00000000");
	t_addr->setMaxLength(8);
	t_addr->setMaximumWidth(65);
	hbox_controls_addr->addWidget(t_addr);
	gb_controls_addr->setLayout(hbox_controls_addr);

	// Controls: Go to
	QGroupBox* gb_controls_goto = new QGroupBox(tr("Go to:"), this);
	QHBoxLayout* hbox_controls_goto = new QHBoxLayout();
	QPushButton* b_goto_get = new QPushButton(tr("Get"), this);
	QPushButton* b_goto_put = new QPushButton(tr("Put"), this);
	b_goto_get->setAutoDefault(false);
	b_goto_put->setAutoDefault(false);
	hbox_controls_goto->addWidget(b_goto_get);
	hbox_controls_goto->addWidget(b_goto_put);
	gb_controls_goto->setLayout(hbox_controls_goto);

	// Controls: Breaks
	QGroupBox* gb_controls_breaks = new QGroupBox(tr("Break on:"), this);
	QHBoxLayout* hbox_controls_breaks = new QHBoxLayout();
	QPushButton* b_break_frame = new QPushButton(tr("Frame"), this);
	QPushButton* b_break_text  = new QPushButton(tr("Texture"), this);
	QPushButton* b_break_draw  = new QPushButton(tr("Draw"), this);
	QPushButton* b_break_prim  = new QPushButton(tr("Primitive"), this);
	QPushButton* b_break_inst  = new QPushButton(tr("Command"), this);
	b_break_frame->setAutoDefault(false);
	b_break_text->setAutoDefault(false);
	b_break_draw->setAutoDefault(false);
	b_break_prim->setAutoDefault(false);
	b_break_inst->setAutoDefault(false);
	hbox_controls_breaks->addWidget(b_break_frame);
	hbox_controls_breaks->addWidget(b_break_text);
	hbox_controls_breaks->addWidget(b_break_draw);
	hbox_controls_breaks->addWidget(b_break_prim);
	hbox_controls_breaks->addWidget(b_break_inst);
	gb_controls_breaks->setLayout(hbox_controls_breaks);

	// TODO: This feature is not yet implemented
	b_break_frame->setEnabled(false);
	b_break_text->setEnabled(false);
	b_break_draw->setEnabled(false);
	b_break_prim->setEnabled(false);
	b_break_inst->setEnabled(false);
	
	hbox_controls->addWidget(gb_controls_addr);
	hbox_controls->addWidget(gb_controls_goto);
	hbox_controls->addWidget(gb_controls_breaks);
	gb_controls->setLayout(hbox_controls);


	QTabWidget* tw_rsx = new QTabWidget(this);
	tw_rsx->setFixedSize(QSize(726, 660));

	//adds a tab containing a list to the tabwidget
	auto l_addRSXTab = [=](QTableWidget* table, QString tabname, int columns)
	{
		QWidget* tab = new QWidget(tw_rsx);
		tw_rsx->addTab(tab, tabname);
		table = new QTableWidget(tab);
		table->setItemDelegate(new table_item_delegate(this));
		table->setFixedSize(QSize(720, 634));
		table->setFont(mono);
		table->setGridStyle(Qt::NoPen);
		table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		table->setSelectionBehavior(QAbstractItemView::SelectRows);
		table->verticalHeader()->setVisible(false);
		table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
		table->verticalHeader()->setDefaultSectionSize(16);
		table->setColumnCount(columns);
		return table;
	};

	m_list_commands = l_addRSXTab(m_list_commands, tr("RSX Commands"), 4);
	m_list_captured_frame = l_addRSXTab(m_list_captured_frame, tr("Captured Frame"), 1);
	m_list_captured_draw_calls = l_addRSXTab(m_list_captured_draw_calls, tr("Captured Draw Calls"), 1);
	m_list_flags = l_addRSXTab(m_list_flags, tr("Flags"), 2);
	m_list_lightning = l_addRSXTab(m_list_lightning, tr("Lightning"), 2);
	m_list_texture = l_addRSXTab(m_list_texture, tr("Texture"), 9);
	m_list_settings = l_addRSXTab(m_list_settings, tr("Settings"), 2);

	//Tabs: List Columns
	m_list_commands->setHorizontalHeaderLabels(QStringList() << tr("Column") << tr("Value") << tr("Command") << tr("Count"));
	m_list_commands->setColumnWidth(0, 70);
	m_list_commands->setColumnWidth(1, 70);
	m_list_commands->setColumnWidth(2, 520);
	m_list_commands->setColumnWidth(3, 60);

	m_list_captured_frame->setHorizontalHeaderLabels(QStringList() << tr("Column"));
	m_list_captured_frame->setColumnWidth(0, 720);

	m_list_captured_draw_calls->setHorizontalHeaderLabels(QStringList() << tr("Draw calls"));
	m_list_captured_draw_calls->setColumnWidth(0, 720);

	m_list_flags->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Value"));
	m_list_flags->setColumnWidth(0, 170);
	m_list_flags->setColumnWidth(1, 270);

	m_list_lightning->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Value"));
	m_list_lightning->setColumnWidth(0, 170);
	m_list_lightning->setColumnWidth(1, 270);

	m_list_texture->setHorizontalHeaderLabels(QStringList() << tr("Index") << tr("Address") << tr("Cubemap")
		<< tr("Dimension") << tr("Enabled") << tr("Format") << tr("Mipmap") << tr("Pitch") << tr("Size"));
	for (int i = 0; i<m_list_texture->columnCount(); i++) m_list_lightning->setColumnWidth(i, 80);

	m_list_settings->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Value"));
	m_list_settings->setColumnWidth(0, 170);
	m_list_settings->setColumnWidth(1, 270);

	// Fill list
	for(u32 i=0; i<m_item_count; i++)
	{
		m_list_commands->insertRow(m_list_commands->rowCount());
	}
	for (u32 i = 0; i<frame_debug.command_queue.size(); i++)
		m_list_captured_frame->insertRow(1);

	//Tools: Tools = Controls + Notebook Tabs
	vbox_tools->addWidget(gb_controls);
	vbox_tools->addSpacing(10);
	vbox_tools->addWidget(tw_rsx);

	// State explorer
	QHBoxLayout* hbox_state_explorer = new QHBoxLayout();
	QTabWidget* state_rsx = new QTabWidget(this);
	state_rsx->setFixedSize(QSize(726,746)); // content fits nicely

	QWidget* p_buffers = new QWidget(state_rsx);
	QWidget* p_transform_program = new QWidget(state_rsx);
	QWidget* p_shader_program = new QWidget(state_rsx);
	QWidget* p_index_buffer = new QWidget(state_rsx);

	state_rsx->addTab(p_buffers, tr("RTTs and DS"));
	state_rsx->addTab(p_transform_program, tr("Transform program"));
	state_rsx->addTab(p_shader_program, tr("Shader program"));
	state_rsx->addTab(p_index_buffer, tr("Index buffer"));

	m_text_transform_program = new QLabel(p_transform_program);
	m_text_transform_program->setFixedSize(QSize(720, 720));
	m_text_transform_program->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_text_transform_program->setFont(mono);
	m_text_transform_program->setText("");

	m_text_shader_program = new QLabel(p_shader_program);
	m_text_shader_program->setFixedSize(QSize(720, 720));
	m_text_shader_program->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_text_shader_program->setFont(mono);
	m_text_shader_program->setText("");

	m_list_index_buffer = new QListWidget(p_index_buffer);
	m_list_index_buffer->setFixedSize(QSize(720, 720));
	m_list_index_buffer->setFont(mono);

	//Buffers
	QVBoxLayout* vbox_buffers1 = new QVBoxLayout();
	QVBoxLayout* vbox_buffers2 = new QVBoxLayout();
	QGroupBox* gb_buffers_colorA  = new QGroupBox(tr("Color Buffer A"), p_buffers);
	QGroupBox* gb_buffers_colorB  = new QGroupBox(tr("Color Buffer B"), p_buffers);
	QGroupBox* gb_buffers_colorC  = new QGroupBox(tr("Color Buffer C"), p_buffers);
	QGroupBox* gb_buffers_colorD  = new QGroupBox(tr("Color Buffer D"), p_buffers);
	QGroupBox* gb_buffers_depth   = new QGroupBox(tr("Depth Buffer"), p_buffers);
	QGroupBox* gb_buffers_stencil = new QGroupBox(tr("Stencil Buffer"), p_buffers);
	QGroupBox* gb_buffers_text    = new QGroupBox(tr("Texture"), p_buffers);
	QHBoxLayout* hbox_buffers_colorA	= new QHBoxLayout();
	QHBoxLayout* hbox_buffers_colorB	= new QHBoxLayout();
	QHBoxLayout* hbox_buffers_colorC	= new QHBoxLayout();
	QHBoxLayout* hbox_buffers_colorD	= new QHBoxLayout();
	QHBoxLayout* hbox_buffers_depth		= new QHBoxLayout();
	QHBoxLayout* hbox_buffers_stencil	= new QHBoxLayout();
	QHBoxLayout* hbox_buffers_text		= new QHBoxLayout();
	gb_buffers_colorA	->setLayout(hbox_buffers_colorA);
	gb_buffers_colorB	->setLayout(hbox_buffers_colorB);
	gb_buffers_colorC	->setLayout(hbox_buffers_colorC);
	gb_buffers_colorD	->setLayout(hbox_buffers_colorD);
	gb_buffers_depth	->setLayout(hbox_buffers_depth);
	gb_buffers_stencil->setLayout(hbox_buffers_stencil);
	gb_buffers_text		->setLayout(hbox_buffers_text);
	
	//Buffers and textures
	m_panel_width = 108;
	m_panel_height = 108;
	m_text_width = 108;
	m_text_height = 108;

	//Panels for displaying the buffers
	m_buffer_colorA  = new Buffer(p_buffers, false, 0);
	m_buffer_colorB  = new Buffer(p_buffers, false, 1);
	m_buffer_colorC  = new Buffer(p_buffers, false, 2);
	m_buffer_colorD  = new Buffer(p_buffers, false, 3);
	m_buffer_depth   = new Buffer(p_buffers, false);
	m_buffer_stencil = new Buffer(p_buffers, false);
	m_buffer_tex     = new Buffer(p_buffers, true);
	m_buffer_colorA ->setFixedSize(QSize(m_panel_width, m_panel_height));
	m_buffer_colorB ->setFixedSize(QSize(m_panel_width, m_panel_height));
	m_buffer_colorC ->setFixedSize(QSize(m_panel_width, m_panel_height));
	m_buffer_colorD ->setFixedSize(QSize(m_panel_width, m_panel_height));
	m_buffer_depth  ->setFixedSize(QSize(m_panel_width, m_panel_height));
	m_buffer_stencil->setFixedSize(QSize(m_panel_width, m_panel_height));
	m_buffer_tex    ->setFixedSize(QSize(m_text_width, m_text_height));
	hbox_buffers_colorA ->addWidget(m_buffer_colorA);
	hbox_buffers_colorB ->addWidget(m_buffer_colorB);
	hbox_buffers_colorC ->addWidget(m_buffer_colorC);
	hbox_buffers_colorD ->addWidget(m_buffer_colorD);
	hbox_buffers_depth  ->addWidget(m_buffer_depth);
	hbox_buffers_stencil->addWidget(m_buffer_stencil);
	hbox_buffers_text   ->addWidget(m_buffer_tex);
	
	//Merge and display everything
	vbox_buffers1->addSpacing(10);
	vbox_buffers1->addWidget(gb_buffers_colorA);
	vbox_buffers1->addSpacing(10);
	vbox_buffers1->addWidget(gb_buffers_colorC);
	vbox_buffers1->addSpacing(10);
	vbox_buffers1->addWidget(gb_buffers_depth);
	vbox_buffers1->addSpacing(10);
	vbox_buffers1->addWidget(gb_buffers_text);
	vbox_buffers1->addSpacing(10);
	vbox_buffers1->addStretch();

	vbox_buffers2->addSpacing(10);
	vbox_buffers2->addWidget(gb_buffers_colorB);
	vbox_buffers2->addSpacing(10);
	vbox_buffers2->addWidget(gb_buffers_colorD);
	vbox_buffers2->addSpacing(10);
	vbox_buffers2->addWidget(gb_buffers_stencil);
	vbox_buffers2->addSpacing(10);
	vbox_buffers2->addStretch();

	hbox_state_explorer->addLayout(vbox_buffers1);
	hbox_state_explorer->addSpacing(10);
	hbox_state_explorer->addLayout(vbox_buffers2);
	hbox_state_explorer->addStretch();

	p_buffers->setLayout(hbox_state_explorer);

	hbox_panel->addLayout(vbox_tools);
	hbox_panel->addSpacing(10);
	hbox_panel->addWidget(state_rsx);
	setLayout(hbox_panel);

	//Events
	connect(b_goto_get, &QAbstractButton::clicked, [=](){
		if (const auto render = fxm::get<GSRender>()){
			u32 realAddr;
			if (RSXIOMem.getRealAddr(render->ctrl->get.load(), realAddr)){
				m_addr = realAddr;
				UpdateInformation();
			}
		}
	});
	connect(b_goto_put, &QAbstractButton::clicked, [=](){
		if (const auto render = fxm::get<GSRender>()){
			u32 realAddr;
			if (RSXIOMem.getRealAddr(render->ctrl->put.load(), realAddr)){
				m_addr = realAddr;
				UpdateInformation();
			}
		}
	});
	connect(t_addr, &QLineEdit::returnPressed, [=](){
		bool ok;
		m_addr = t_addr->text().toULong(&ok, 16);
		UpdateInformation();
	});
	connect(m_list_flags, &QTableWidget::itemClicked, this, &rsx_debugger::SetFlags);
	connect(m_list_texture, &QTableWidget::itemClicked, [=](){
		int index = m_list_texture->currentRow();
		if (index >= 0) m_cur_texture = index;
		UpdateInformation();
	});
	connect(m_list_captured_draw_calls, &QTableWidget::itemClicked, this, &rsx_debugger::OnClickDrawCalls);

	//Fill the frame
	UpdateInformation();
	setFixedSize(sizeHint());
	setFocusProxy(t_addr);
};

void rsx_debugger::keyPressEvent(QKeyEvent* event)
{
	if(isActiveWindow())
	{
		switch(event->key())
		{
		case Qt::Key_F5: UpdateInformation(); return;
		}
	}
}

void rsx_debugger::wheelEvent(QWheelEvent* event)
{
	if (!m_list_commands->underMouse())
	{
		return;
	}
	QPoint numSteps = event->angleDelta() / 8 / 15; // http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
	if(vm::check_addr(m_addr, 4))
	{
		int items = event->modifiers() & Qt::ControlModifier ? m_item_count : 1;

		for(int i=0; i<items; ++i)
		{
			u32 offset;
			if(vm::check_addr(m_addr, 4))
			{
				u32 cmd = vm::ps3::read32(m_addr);
				u32 count = ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD)
					|| ((cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD)
					|| ((cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD)
					|| cmd == RSX_METHOD_RETURN_CMD ? 0 : (cmd >> 18) & 0x7ff;

				offset = 1 + count;
			}
			else
			{
				offset = 1;
			}

			m_addr -= 4 * offset * numSteps.y();
		}
	}
	else
	{
		m_addr -= (event->modifiers() & Qt::ControlModifier ? m_item_count : 1) * 4 * numSteps.y();
	}

	UpdateInformation();
}

namespace
{
	// Opens an image in a new window with original size
	void display_buffer(QWidget *parent, const QImage img)
	{
		if (img.isNull()) return;
		//QString title = qstr(fmt::format("Raw Image @ 0x%x", addr));
		QLabel* canvas = new QLabel();
		QPalette* pal_bg = new QPalette();
		pal_bg->setColor(canvas->backgroundRole(), QColor(240, 240, 240));
		canvas->setPalette(*pal_bg); //This fix the ugly background color under Windows
		canvas->setPixmap(QPixmap::fromImage(img));
		canvas->setFixedSize(img.size());
		canvas->show();
	}
}

// Draws a formatted and buffered <image> inside the Buffer Widget
void Buffer::showImage(QImage image)
{
	if (image.isNull()) return;
	m_image = image;
	m_scaled = m_image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_canvas = new QLabel();
	m_canvas->setFixedSize(size());
	m_canvas->setPixmap(QPixmap::fromImage(m_scaled));
	m_layout = new QHBoxLayout();
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->addWidget(m_canvas);
	setLayout(m_layout);
}

void Buffer::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		const auto render = fxm::get<GSRender>();
		if (!render)
		{
			return;
		}

		const auto buffers = render->gcm_buffers;
		if (!buffers)
			return;

		// TODO: Is there any better way to choose the color buffers
#define SHOW_BUFFER(id) \
	{ \
		u32 addr = render->local_mem_addr + buffers[id].offset; \
		if (vm::check_addr(addr) && buffers[id].width && buffers[id].height) \
			memory_viewer_panel::ShowImage(this, addr, 3, buffers[id].width, buffers[id].height, true); \
		return; \
	} \

	//if (0 <= m_id && m_id < 4) SHOW_BUFFER(m_id);

		display_buffer(this, m_image);

		if (m_isTex)
		{
			/*		u8 location = render->textures[m_cur_texture].location();
					if(location <= 1 && vm::check_addr(rsx::get_address(render->textures[m_cur_texture].offset(), location))
						&& render->textures[m_cur_texture].width() && render->textures[m_cur_texture].height())
						memory_viewer_panel::ShowImage(this,
							rsx::get_address(render->textures[m_cur_texture].offset(), location), 1,
							render->textures[m_cur_texture].width(),
							render->textures[m_cur_texture].height(), false);*/
		}

#undef SHOW_BUFFER
	}
}

namespace
{
	std::array<u8, 3> get_value(gsl::span<const gsl::byte> orig_buffer, rsx::surface_color_format format, size_t idx)
	{
		switch (format)
		{
		case rsx::surface_color_format::b8:
		{
			u8 value = gsl::as_span<const u8>(orig_buffer)[idx];
			return{ value, value, value };
		}
		case rsx::surface_color_format::x32:
		{
			be_t<u32> stored_val = gsl::as_span<const be_t<u32>>(orig_buffer)[idx];
			u32 swapped_val = stored_val;
			f32 float_val = (f32&)swapped_val;
			u8 val = float_val * 255.f;
			return{ val, val, val };
		}
		case rsx::surface_color_format::a8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		{
			auto ptr = gsl::as_span<const u8>(orig_buffer);
			return{ ptr[1 + idx * 4], ptr[2 + idx * 4], ptr[3 + idx * 4] };
		}
		case rsx::surface_color_format::a8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		{
			auto ptr = gsl::as_span<const u8>(orig_buffer);
			return{ ptr[3 + idx * 4], ptr[2 + idx * 4], ptr[1 + idx * 4] };
		}
		case rsx::surface_color_format::w16z16y16x16:
		{
			auto ptr = gsl::as_span<const u16>(orig_buffer);
			f16 h0 = f16(ptr[4 * idx]);
			f16 h1 = f16(ptr[4 * idx + 1]);
			f16 h2 = f16(ptr[4 * idx + 2]);
			f32 f0 = float(h0);
			f32 f1 = float(h1);
			f32 f2 = float(h2);

			u8 val0 = f0 * 255.;
			u8 val1 = f1 * 255.;
			u8 val2 = f2 * 255.;
			return{ val0, val1, val2 };
		}
		case rsx::surface_color_format::g8b8:
		case rsx::surface_color_format::r5g6b5:
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		case rsx::surface_color_format::w32z32y32x32:
			fmt::throw_exception("Unsupported format for display" HERE);
		}
	}

	/**
	 * Return a new buffer that can be passed to QImage.
	 */
	u8* convert_to_QImage_buffer(rsx::surface_color_format format, gsl::span<const gsl::byte> orig_buffer, size_t width, size_t height) noexcept
	{
		unsigned char* buffer = (unsigned char*)malloc(width * height * 4);
		for (u32 i = 0; i < width * height; i++)
		{
			// depending on original buffer, the colors may need to be reversed
			const auto &colors = get_value(orig_buffer, format, i);
			buffer[0 + i * 4] = colors[0];
			buffer[1 + i * 4] = colors[1];
			buffer[2 + i * 4] = colors[2];
			buffer[3 + i * 4] = 255;
		}
		return buffer;
	}
};

void rsx_debugger::OnClickDrawCalls()
{
	size_t draw_id = m_list_captured_draw_calls->currentRow();

	const auto& draw_call = frame_debug.draw_calls[draw_id];

	Buffer* buffers[] =
	{
		m_buffer_colorA,
		m_buffer_colorB,
		m_buffer_colorC,
		m_buffer_colorD,
	};

	size_t width = draw_call.state.surface_clip_width();
	size_t height = draw_call.state.surface_clip_height();

	for (size_t i = 0; i < 4; i++)
	{
		if (width && height && !draw_call.color_buffer[i].empty())
		{
			unsigned char* buffer = convert_to_QImage_buffer(draw_call.state.surface_color(), draw_call.color_buffer[i], width, height);
			buffers[i]->showImage(QImage(buffer, width, height, QImage::Format_RGB32));
		}
	}

	// Buffer Z
	{
		if (width && height && !draw_call.depth_stencil[0].empty())
		{
			gsl::span<const gsl::byte> orig_buffer = draw_call.depth_stencil[0];
			unsigned char *buffer = (unsigned char *)malloc(width * height * 4);

			if (draw_call.state.surface_depth_fmt() == rsx::surface_depth_format::z24s8)
			{
				for (u32 row = 0; row < height; row++)
				{
					for (u32 col = 0; col < width; col++)
					{
						u32 depth_val = gsl::as_span<const u32>(orig_buffer)[row * width + col];
						u8 displayed_depth_val = 255 * depth_val / 0xFFFFFF;
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
						u16 depth_val = gsl::as_span<const u16>(orig_buffer)[row * width + col];
						u8 displayed_depth_val = 255 * depth_val / 0xFFFF;
						buffer[4 * col + 0 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 1 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 2 + width * row * 4] = displayed_depth_val;
						buffer[4 * col + 3 + width * row * 4] = 255;
					}
				}
			}
			m_buffer_depth->showImage(QImage(buffer, width, height, QImage::Format_RGB32));
		}
	}

	// Buffer S
	{
		if (width && height && !draw_call.depth_stencil[1].empty())
		{
			gsl::span<const gsl::byte> orig_buffer = draw_call.depth_stencil[1];
			unsigned char *buffer = (unsigned char *)malloc(width * height * 4);

			for (u32 row = 0; row < height; row++)
			{
				for (u32 col = 0; col < width; col++)
				{
					u8 stencil_val = gsl::as_span<const u8>(orig_buffer)[row * width + col];
					buffer[4 * col + 0 + width * row * 4] = stencil_val;
					buffer[4 * col + 1 + width * row * 4] = stencil_val;
					buffer[4 * col + 2 + width * row * 4] = stencil_val;
					buffer[4 * col + 3 + width * row * 4] = 255;
				}
			}
			m_buffer_stencil->showImage(QImage(buffer, width, height, QImage::Format_RGB32));
		}
	}

	// Programs
	m_text_transform_program->clear();
	m_text_transform_program->setText(qstr(frame_debug.draw_calls[draw_id].programs.first));
	m_text_shader_program->clear();
	m_text_shader_program->setText(qstr(frame_debug.draw_calls[draw_id].programs.second));

	m_list_index_buffer->clear();
	//m_list_index_buffer->insertColumn(0, "Index", 0, 700);
	if (frame_debug.draw_calls[draw_id].state.index_type() == rsx::index_array_type::u16)
	{
		u16 *index_buffer = (u16*)frame_debug.draw_calls[draw_id].index.data();
		for (u32 i = 0; i < frame_debug.draw_calls[draw_id].vertex_count; ++i)
		{
			m_list_index_buffer->insertItem(i, qstr(std::to_string(index_buffer[i])));
		}
	}
	if (frame_debug.draw_calls[draw_id].state.index_type() == rsx::index_array_type::u32)
	{
		u32 *index_buffer = (u32*)frame_debug.draw_calls[draw_id].index.data();
		for (u32 i = 0; i < frame_debug.draw_calls[draw_id].vertex_count; ++i)
		{
			m_list_index_buffer->insertItem(i, qstr(std::to_string(index_buffer[i])));
		}
	}
}

void rsx_debugger::UpdateInformation()
{
	t_addr->setText(QString("%1").arg(m_addr, 8, 16, QChar('0'))); // get 8 digits in input line
	GetMemory();
	GetBuffers();
	GetFlags();
	GetLightning();
	GetTexture();
	GetSettings();
}

void rsx_debugger::GetMemory()
{
	// Clean commands column
	for(u32 i=0; i<m_item_count; i++)
		m_list_commands->setItem(i, 2, new QTableWidgetItem(""));

	// Write information
	for(u32 i=0, addr = m_addr; i<m_item_count; i++, addr += 4)
	{
		m_list_commands->setItem(i, 0, new QTableWidgetItem(qstr(fmt::format("%08x", addr))));
	
		if (vm::check_addr(addr))
		{
			u32 cmd = vm::ps3::read32(addr);
			u32 count = (cmd >> 18) & 0x7ff;
			m_list_commands->setItem(i, 1, new QTableWidgetItem(qstr(fmt::format("%08x", cmd))));
			m_list_commands->setItem(i, 3, new QTableWidgetItem(qstr(fmt::format("%d", count))));
			m_list_commands->setItem(i, 2, new QTableWidgetItem(DisAsmCommand(cmd, count, addr, 0)));

			if((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) != RSX_METHOD_OLD_JUMP_CMD
				&& (cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) != RSX_METHOD_NEW_JUMP_CMD
				&& (cmd & RSX_METHOD_CALL_CMD_MASK) != RSX_METHOD_CALL_CMD
				&& cmd != RSX_METHOD_RETURN_CMD)
			{
				addr += 4 * count;
			}
		}
		else
		{
			m_list_commands->setItem(i, 1, new QTableWidgetItem("????????"));
		}
	}

	std::string dump;

	for (u32 i = 0; i < frame_debug.command_queue.size(); i++)
	{
		const std::string& str = rsx::get_pretty_printing_function(frame_debug.command_queue[i].first)(frame_debug.command_queue[i].second);
		m_list_captured_frame->setItem(i, 0, new QTableWidgetItem(qstr(str)));

		dump += str;
		dump += '\n';
	}

	fs::file(fs::get_config_dir() + "command_dump.log", fs::rewrite).write(dump);

	for (u32 i = 0;i < frame_debug.draw_calls.size(); i++)
		m_list_captured_draw_calls->setItem(i, 0, new QTableWidgetItem(qstr(frame_debug.draw_calls[i].name)));
}

void rsx_debugger::GetBuffers()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	// Draw Buffers
	// TODO: Currently it only supports color buffers
	for (u32 bufferId=0; bufferId < render->gcm_buffers_count; bufferId++)
	{
		if(!vm::check_addr(render->gcm_buffers.addr()))
			continue;

		auto buffers = render->gcm_buffers;
		u32 RSXbuffer_addr = render->local_mem_addr + buffers[bufferId].offset;

		if(!vm::check_addr(RSXbuffer_addr))
			continue;

		auto RSXbuffer = vm::ps3::_ptr<u8>(RSXbuffer_addr);
		
		u32 width  = buffers[bufferId].width;
		u32 height = buffers[bufferId].height;
		unsigned char* buffer = (unsigned char*)malloc(width * height * 4);

		// ABGR to ARGB and flip vertically
		for (u32 y=0; y<height; y++){
			for (u32 i=0, j=0; j<width*4; i+=4, j+=4)
			{
				buffer[i+0 + y*width*4] = RSXbuffer[j+1 + (height-y-1)*width*4];	//B
				buffer[i+1 + y*width*4] = RSXbuffer[j+2 + (height-y-1)*width*4];	//G
				buffer[i+2 + y*width*4] = RSXbuffer[j+3 + (height-y-1)*width*4];	//R
				buffer[i+3 + y*width*4] = RSXbuffer[j+0 + (height-y-1)*width*4];	//A
			}
		}

		// TODO: Is there any better way to clasify the color buffers? How can we include the depth and stencil buffers?
		Buffer* pnl;
		switch(bufferId)
		{
		case 0:  pnl = m_buffer_colorA;  break;
		case 1:  pnl = m_buffer_colorB;  break;
		case 2:  pnl = m_buffer_colorC;  break;
		default: pnl = m_buffer_colorD;  break;
		}
		pnl->showImage(QImage(buffer, width, height, QImage::Format_RGB32));
	}

	// Draw Texture
/*	if(!render->textures[m_cur_texture].enabled())
		return;

	u32 offset = render->textures[m_cur_texture].offset();

	if(!offset)
		return;

	u8 location = render->textures[m_cur_texture].location();

	if(location > 1)
		return;

	u32 TexBuffer_addr = rsx::get_address(offset, location);

	if(!vm::check_addr(TexBuffer_addr))
		return;

	unsigned char* TexBuffer = vm::ps3::_ptr<u8>(TexBuffer_addr);

	u32 width  = render->textures[m_cur_texture].width();
	u32 height = render->textures[m_cur_texture].height();
	unsigned char* buffer = (unsigned char*)malloc(width * height * 3);
	std::memcpy(buffer, vm::base(TexBuffer_addr), width * height * 3);

	m_buffer_tex->showImage(QImage(buffer, m_text_width, m_text_height, QImage::Format_RGB32));*/
}

void rsx_debugger::GetFlags()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	m_list_flags->clearContents();
	int i=0;

#define LIST_FLAGS_ADD(name, value) \
	m_list_flags->setItem(i, 0, new QTableWidgetItem(qstr(name))); m_list_flags->setItem(i, 1, new QTableWidgetItem(qstr(value ? "Enabled" : "Disabled"))); i++;
	/*
	LIST_FLAGS_ADD("Alpha test",         render->m_set_alpha_test);
	LIST_FLAGS_ADD("Blend",              render->m_set_blend);
	LIST_FLAGS_ADD("Scissor",            render->m_set_scissor_horizontal && render->m_set_scissor_vertical);
	LIST_FLAGS_ADD("Cull face",          render->m_set_cull_face);
	LIST_FLAGS_ADD("Depth bounds test",  render->m_set_depth_bounds_test);
	LIST_FLAGS_ADD("Depth test",         render->m_set_depth_test);
	LIST_FLAGS_ADD("Dither",             render->m_set_dither);
	LIST_FLAGS_ADD("Line smooth",        render->m_set_line_smooth);
	LIST_FLAGS_ADD("Logic op",           render->m_set_logic_op);
	LIST_FLAGS_ADD("Poly smooth",        render->m_set_poly_smooth);
	LIST_FLAGS_ADD("Poly offset fill",   render->m_set_poly_offset_fill);
	LIST_FLAGS_ADD("Poly offset line",   render->m_set_poly_offset_line);
	LIST_FLAGS_ADD("Poly offset point",  render->m_set_poly_offset_point);
	LIST_FLAGS_ADD("Stencil test",       render->m_set_stencil_test);
	LIST_FLAGS_ADD("Primitive restart",  render->m_set_restart_index);
	LIST_FLAGS_ADD("Two sided lighting", render->m_set_two_side_light_enable);
	LIST_FLAGS_ADD("Point Sprite",	     render->m_set_point_sprite_control);
	LIST_FLAGS_ADD("Lighting ",	         render->m_set_specular);
	*/

#undef LIST_FLAGS_ADD
}

void rsx_debugger::GetLightning()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	m_list_lightning->clearContents();
	int i=0;

#define LIST_LIGHTNING_ADD(name, value) \
	m_list_lightning->setItem(i, 0, new QTableWidgetItem(qstr(name))); m_list_lightning->setItem(i, 1, new QTableWidgetItem(qstr(value))); i++;

	//LIST_LIGHTNING_ADD("Shade model", (render->m_shade_mode == 0x1D00) ? "Flat" : "Smooth");

#undef LIST_LIGHTNING_ADD
}

void rsx_debugger::GetTexture()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	m_list_texture->clearContents();
	m_list_texture->setRowCount(rsx::limits::fragment_textures_count);

	for(uint i=0; i<rsx::limits::fragment_textures_count; ++i)
	{
/*		if(render->textures[i].enabled())
		{
			m_list_texture->setItem(i, 0, new QTableWidgetItem(qstr(fmt::format("%d", i)));
			u8 location = render->textures[i].location();
			if(location > 1)
			{
				m_list_texture->setItem(i, 1,
					new QTableWidgetItem(qstr(fmt::format("Bad address (offset=0x%x, location=%d)", render->textures[i].offset(), location))));
			}
			else
			{
				m_list_texture->setItem(i, 1, 
					new QTableWidgetItem(qstr(fmt::format("0x%x", rsx::get_address(render->textures[i].offset(), location)))));
			}

			m_list_texture->setItem(i, 2, new QTableWidgetItem(render->textures[i].cubemap() ? "True" : "False"));
			m_list_texture->setItem(i, 3, new QTableWidgetItem(qstr(fmt::format("%dD", render->textures[i].dimension()))));
			m_list_texture->setItem(i, 4, new QTableWidgetItem(render->textures[i].enabled() ? "True" : "False"));
			m_list_texture->setItem(i, 5, new QTableWidgetItem(qstr(fmt::format("0x%x", render->textures[i].format()))));
			m_list_texture->setItem(i, 6, new QTableWidgetItem(qstr(fmt::format("0x%x", render->textures[i].mipmap()))));
			m_list_texture->setItem(i, 7, new QTableWidgetItem(qstr(fmt::format("0x%x", render->textures[i].pitch()))));
			m_list_texture->setItem(i, 8, new QTableWidgetItem(qstr(fmt::format("%dx%d",
				render->textures[i].width(),
				render->textures[i].height()))));

			m_list_texture->SetItemBackgroundColour(i, QColor(m_cur_texture == i ? QColor::yellow : QColor::white));
		}*/
	}
}

void rsx_debugger::GetSettings()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	m_list_settings->clearContents();
	int i=0;

#define LIST_SETTINGS_ADD(name, value) \
	m_list_settings->setItem(i, 0, new QTableWidgetItem(qstr(name))); m_list_settings->setItem(i, 1, new QTableWidgetItem(qstr(value))); i++;
	/*
	LIST_SETTINGS_ADD("Alpha func", !(render->m_set_alpha_func) ? "(none)" : fmt::format("0x%x (%s)",
		render->m_alpha_func,
		ParseGCMEnum(render->m_alpha_func, CELL_GCM_ENUM)));
	LIST_SETTINGS_ADD("Blend color", !(render->m_set_blend_color) ? "(none)" : fmt::format("R:%d, G:%d, B:%d, A:%d",
		render->m_blend_color_r,
		render->m_blend_color_g,
		render->m_blend_color_b,
		render->m_blend_color_a));
	LIST_SETTINGS_ADD("Clipping", fmt::format("Min:%f, Max:%f", render->m_clip_min, render->m_clip_max));
	LIST_SETTINGS_ADD("Color mask", !(render->m_set_color_mask) ? "(none)" : fmt::format("R:%d, G:%d, B:%d, A:%d",
		render->m_color_mask_r,
		render->m_color_mask_g,
		render->m_color_mask_b,
		render->m_color_mask_a));
	LIST_SETTINGS_ADD("Context DMA Color A", fmt::format("0x%x", render->m_context_dma_color_a));
	LIST_SETTINGS_ADD("Context DMA Color B", fmt::format("0x%x", render->m_context_dma_color_b));
	LIST_SETTINGS_ADD("Context DMA Color C", fmt::format("0x%x", render->m_context_dma_color_c));
	LIST_SETTINGS_ADD("Context DMA Color D", fmt::format("0x%x", render->m_context_dma_color_d));
	LIST_SETTINGS_ADD("Context DMA Zeta", fmt::format("0x%x", render->m_context_dma_z));
	LIST_SETTINGS_ADD("Depth bounds", fmt::format("Min:%f, Max:%f", render->m_depth_bounds_min, render->m_depth_bounds_max));
	LIST_SETTINGS_ADD("Depth func", !(render->m_set_depth_func) ? "(none)" : fmt::format("0x%x (%s)",
		render->m_depth_func,
		ParseGCMEnum(render->m_depth_func, CELL_GCM_ENUM)));
	LIST_SETTINGS_ADD("Draw mode", fmt::format("%d (%s)",
		render->m_draw_mode,
		ParseGCMEnum(render->m_draw_mode, CELL_GCM_PRIMITIVE_ENUM)));
	LIST_SETTINGS_ADD("Scissor", fmt::format("X:%d, Y:%d, W:%d, H:%d",
		render->m_scissor_x,
		render->m_scissor_y,
		render->m_scissor_w,
		render->m_scissor_h));
	LIST_SETTINGS_ADD("Stencil func", !(render->m_set_stencil_func) ? "(none)" : fmt::format("0x%x (%s)",
		render->m_stencil_func,
		ParseGCMEnum(render->m_stencil_func, CELL_GCM_ENUM)));
	LIST_SETTINGS_ADD("Surface Pitch A", fmt::format("0x%x", render->m_surface_pitch_a));
	LIST_SETTINGS_ADD("Surface Pitch B", fmt::format("0x%x", render->m_surface_pitch_b));
	LIST_SETTINGS_ADD("Surface Pitch C", fmt::format("0x%x", render->m_surface_pitch_c));
	LIST_SETTINGS_ADD("Surface Pitch D", fmt::format("0x%x", render->m_surface_pitch_d));
	LIST_SETTINGS_ADD("Surface Pitch Z", fmt::format("0x%x", render->m_surface_pitch_z));
	LIST_SETTINGS_ADD("Surface Offset A", fmt::format("0x%x", render->m_surface_offset_a));
	LIST_SETTINGS_ADD("Surface Offset B", fmt::format("0x%x", render->m_surface_offset_b));
	LIST_SETTINGS_ADD("Surface Offset C", fmt::format("0x%x", render->m_surface_offset_c));
	LIST_SETTINGS_ADD("Surface Offset D", fmt::format("0x%x", render->m_surface_offset_d));
	LIST_SETTINGS_ADD("Surface Offset Z", fmt::format("0x%x", render->m_surface_offset_z));
	LIST_SETTINGS_ADD("Viewport", fmt::format("X:%d, Y:%d, W:%d, H:%d",
		render->m_viewport_x,
		render->m_viewport_y,
		render->m_viewport_w,
		render->m_viewport_h));
		*/
#undef LIST_SETTINGS_ADD
}

void rsx_debugger::SetFlags()
{
	/*
	int index = m_list_flags->currentRow();
	if (!RSXReady()) return;
	GSRender& render = Emu.GetGSManager().GetRender();
	switch(index)
	{
	case 0:  render->m_set_alpha_test ^= true; break;
	case 1:  render->m_set_blend ^= true; break;
	case 2:  render->m_set_cull_face ^= true; break;
	case 3:  render->m_set_depth_bounds_test ^= true; break;
	case 4:  render->m_set_depth_test ^= true; break;
	case 5:  render->m_set_dither ^= true; break;
	case 6:  render->m_set_line_smooth ^= true; break;
	case 7:  render->m_set_logic_op ^= true; break;
	case 8:  render->m_set_poly_smooth ^= true; break;
	case 9:  render->m_set_poly_offset_fill ^= true; break;
	case 10: render->m_set_poly_offset_line ^= true; break;
	case 11: render->m_set_poly_offset_point ^= true; break;
	case 12: render->m_set_stencil_test ^= true; break;
	case 13: render->m_set_point_sprite_control ^= true; break;
	case 14: render->m_set_restart_index ^= true; break;
	case 15: render->m_set_specular ^= true; break;
	case 16: render->m_set_scissor_horizontal ^= true; break;
	case 17: render->m_set_scissor_vertical ^= true; break;
	}
	*/

	UpdateInformation();
}

void rsx_debugger::SetPrograms()
{
	const auto render = fxm::get<GSRender>();
	if (!render)
	{
		return;
	}

	return;
	//rsx_debuggerProgram& program = m_debug_programs[event.m_itemIndex];

	//// Program Editor
	//QString title = qstr(fmt::format("Program ID: %d (VP:%d, FP:%d)", program.id, program.vp_id, program.fp_id));
	//QDialog d_editor(this, title, QSize(800,500));

	//QHBoxLayout* hbox_panel = new QHBoxLayout();
	//QHBoxLayout* hbox_vp_box = new QHBoxLayout(&d_editor, tr("Vertex Program"));
	//QHBoxLayout* hbox_fp_box = new QHBoxLayout(&d_editor, tr("Fragment Program"));
	//QLabel or QTextEdit* t_vp_edit  = new QTextEdit(&d_editor, program.vp_shader);
	//QLabel or QTextEdit* t_fp_edit  = new QTextEdit(&d_editor, program.fp_shader);
	//t_vp_edit->setFont(mono);
	//t_fp_edit->setFont(mono);
	//hbox_vp_box->addWidget(t_vp_edit, 1, wxEXPAND);
	//hbox_fp_box->addWidget(t_fp_edit, 1, wxEXPAND);
	//hbox_panel->addLayout(hbox_vp_box, 1, wxEXPAND);
	//hbox_panel->addLayout(hbox_fp_box, 1, wxEXPAND);
	//d_editor.setLayout(hbox_panel);

	//// Show editor and open Save Dialog when closing
	//if (d_editor.ShowModal())
	//{
	//	wxMessageDialog d_save(&d_editor, "Save changes and compile shaders?", title, wxYES_NO|wxCENTRE);
	//	if(d_save.ShowModal() == wxID_YES)
	//	{
	//		program.modified = true;
	//		program.vp_shader = t_vp_edit->GetValue();
	//		program.fp_shader = t_fp_edit->GetValue();
	//	}
	//}
	//UpdateInformation();
}

const char* rsx_debugger::ParseGCMEnum(u32 value, u32 type)
{
	switch(type)
	{
	case CELL_GCM_ENUM:
	{
		switch(value)
		{
		case 0x0200: return "Never";
		case 0x0201: return "Less";
		case 0x0202: return "Equal";
		case 0x0203: return "Less or Equal";
		case 0x0204: return "Greater";
		case 0x0205: return "Not Equal";
		case 0x0206: return "Greater or Equal";
		case 0x0207: return "Always";

		case 0x0:    return "Zero";
		case 0x1:    return "One";
		case 0x0300: return "SRC_COLOR";
		case 0x0301: return "1 - SRC_COLOR";
		case 0x0302: return "SRC_ALPHA";
		case 0x0303: return "1 - SRC_ALPHA";
		case 0x0304: return "DST_ALPHA";
		case 0x0305: return "1 - DST_ALPHA";
		case 0x0306: return "DST_COLOR";
		case 0x0307: return "1 - DST_COLOR";
		case 0x0308: return "SRC_ALPHA_SATURATE";
		case 0x8001: return "CONSTANT_COLOR";
		case 0x8002: return "1 - CONSTANT_COLOR";
		case 0x8003: return "CONSTANT_ALPHA";
		case 0x8004: return "1 - CONSTANT_ALPHA";

		case 0x8006: return "Add";
		case 0x8007: return "Min";
		case 0x8008: return "Max";
		case 0x800A: return "Substract";
		case 0x800B: return "Reverse Substract";
		case 0xF005: return "Reverse Substract Signed";
		case 0xF006: return "Add Signed";
		case 0xF007: return "Reverse Add Signed";

		default: return "Wrong Value!";
		}
	}
	case CELL_GCM_PRIMITIVE_ENUM:
	{
		switch(value)
		{
		case 1:  return "POINTS";
		case 2:  return "LINES";
		case 3:  return "LINE_LOOP";
		case 4:  return "LINE_STRIP";
		case 5:  return "TRIANGLES";
		case 6:  return "TRIANGLE_STRIP";
		case 7:  return "TRIANGLE_FAN";
		case 8:  return "QUADS";
		case 9:  return "QUAD_STRIP";
		case 10: return "POLYGON";

		default: return "Wrong Value!"; 
		}
	}
	default: return "Unknown!";
	}
}

#define case_16(a, m) \
	case a + m: \
	case a + m * 2: \
	case a + m * 3: \
	case a + m * 4: \
	case a + m * 5: \
	case a + m * 6: \
	case a + m * 7: \
	case a + m * 8: \
	case a + m * 9: \
	case a + m * 10: \
	case a + m * 11: \
	case a + m * 12: \
	case a + m * 13: \
	case a + m * 14: \
	case a + m * 15: \
	index = (cmd - a) / m; \
	case a \

QString rsx_debugger::DisAsmCommand(u32 cmd, u32 count, u32 currentAddr, u32 ioAddr)
{
	std::string disasm = "";

#define DISASM(string, ...) { if(disasm.empty()) disasm = fmt::format((string), ##__VA_ARGS__); else disasm += (' ' + fmt::format((string), ##__VA_ARGS__)); }
	if((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) == RSX_METHOD_OLD_JUMP_CMD)
	{
		u32 jumpAddr = cmd & RSX_METHOD_OLD_JUMP_OFFSET_MASK;
		DISASM("JUMP: %08x -> %08x", currentAddr, ioAddr+jumpAddr);
	}
	else if((cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) == RSX_METHOD_NEW_JUMP_CMD)
	{
		u32 jumpAddr = cmd & RSX_METHOD_NEW_JUMP_OFFSET_MASK;
		DISASM("JUMP: %08x -> %08x", currentAddr, ioAddr + jumpAddr);
	}
	else if((cmd & RSX_METHOD_CALL_CMD_MASK) == RSX_METHOD_CALL_CMD)
	{
		u32 callAddr = cmd & RSX_METHOD_CALL_OFFSET_MASK;
		DISASM("CALL: %08x -> %08x", currentAddr, ioAddr+callAddr);
	}
	if(cmd == RSX_METHOD_RETURN_CMD)
	{
		DISASM("RETURN");
	}

	if(cmd == 0)
	{
		DISASM("Null cmd");
	}
	else if ((cmd & RSX_METHOD_OLD_JUMP_CMD_MASK) != RSX_METHOD_OLD_JUMP_CMD
		&& (cmd & RSX_METHOD_NEW_JUMP_CMD_MASK) != RSX_METHOD_NEW_JUMP_CMD
		&& (cmd & RSX_METHOD_CALL_CMD_MASK) != RSX_METHOD_CALL_CMD
		&& cmd != RSX_METHOD_RETURN_CMD)
	{
		auto args = vm::ps3::ptr<u32>::make(currentAddr + 4);

		u32 index = 0;
		switch((cmd & 0x3ffff) >> 2)
		{
		case 0x3fead:
			DISASM("Flip and change current buffer: %d", (u32)args[0]);
		break;

		case_16(NV4097_SET_TEXTURE_OFFSET, 0x20):
			DISASM("Texture Offset[%d]: %08x", index, (u32)args[0]);
			switch ((args[1] & 0x3) - 1)
			{
			case CELL_GCM_LOCATION_LOCAL: DISASM("(Local memory);");  break;
			case CELL_GCM_LOCATION_MAIN:  DISASM("(Main memory);");   break;
			default:                      DISASM("(Bad location!);"); break;
			}
			DISASM("    Cubemap:%s; Dimension:0x%x; Format:0x%x; Mipmap:0x%x",
				(((args[1] >> 2) & 0x1) ? "True" : "False"),
				((args[1] >> 4) & 0xf),
				((args[1] >> 8) & 0xff),
				((args[1] >> 16) & 0xffff));
		break;

		case NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE:
			DISASM(args[0] ? "Depth bounds test: Enable" : "Depth bounds test: Disable");
		break;
		default:
		{
			std::string str = rsx::get_pretty_printing_function((cmd & 0x3ffff) >> 2)((u32)args[0]);
			DISASM("%s", str.c_str());
		}
		}

		if((cmd & RSX_METHOD_NON_INCREMENT_CMD_MASK) == RSX_METHOD_NON_INCREMENT_CMD)
		{
			DISASM("Non Increment cmd");
		}

		DISASM("[0x%08x(", cmd);

		for(uint i=0; i<count; ++i)
		{
			if(i != 0) disasm += ", ";
			disasm += fmt::format("0x%x", (u32)args[i]);
		}

		disasm += ")]";
	}
#undef DISASM

	return qstr(disasm);
}
