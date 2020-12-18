#include "Utilities/mutex.h"
#include "Emu/Memory/vm_locking.h"
#include "Emu/Memory/vm.h"

#include "memory_viewer_panel.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/IdManager.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>
#include <QTextEdit>
#include <QComboBox>
#include <QWheelEvent>
#include <shared_mutex>

constexpr auto qstr = QString::fromStdString;

memory_viewer_panel::memory_viewer_panel(QWidget* parent, u32 addr)
	: QDialog(parent)
	, m_addr(addr)
{
	setWindowTitle(tr("Memory Viewer"));
	setObjectName("memory_viewer");
	setAttribute(Qt::WA_DeleteOnClose);
	exit = false;
	m_colcount = 4;
	m_rowcount = 16;
	m_addr -= m_addr % (m_colcount * 4); // Align by amount of bytes in a row
	int pSize = 10;

	// Font
	QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	mono.setPointSize(pSize);
	m_fontMetrics = new QFontMetrics(mono);

	// Layout:
	QVBoxLayout* vbox_panel = new QVBoxLayout();

	// Tools
	QHBoxLayout* hbox_tools = new QHBoxLayout();

	// Tools: Memory Viewer Options
	QGroupBox* tools_mem = new QGroupBox(tr("Memory Viewer Options"));
	QHBoxLayout* hbox_tools_mem = new QHBoxLayout();

	// Tools: Memory Viewer Options: Address
	QGroupBox* tools_mem_addr = new QGroupBox(tr("Address"));
	QHBoxLayout* hbox_tools_mem_addr = new QHBoxLayout();
	m_addr_line = new QLineEdit(this);
	m_addr_line->setPlaceholderText("00000000");
	m_addr_line->setText(qstr(fmt::format("%08x", m_addr)));
	m_addr_line->setFont(mono);
	m_addr_line->setMaxLength(10);
	m_addr_line->setFixedWidth(75);
	m_addr_line->setFocus();
	m_addr_line->setValidator(new QRegExpValidator(QRegExp("^([0][xX])?[a-fA-F0-9]{0,8}$")));
	hbox_tools_mem_addr->addWidget(m_addr_line);
	tools_mem_addr->setLayout(hbox_tools_mem_addr);

	// Tools: Memory Viewer Options: Words
	QGroupBox* tools_mem_words = new QGroupBox(tr("Words"));
	QHBoxLayout* hbox_tools_mem_words = new QHBoxLayout();
	QSpinBox* sb_words = new QSpinBox(this);
	sb_words->setRange(1, 4);
	sb_words->setValue(4);
	hbox_tools_mem_words->addWidget(sb_words);
	tools_mem_words->setLayout(hbox_tools_mem_words);

	// Tools: Memory Viewer Options: Control
	QGroupBox* tools_mem_buttons = new QGroupBox(tr("Control"));
	QHBoxLayout* hbox_tools_mem_buttons = new QHBoxLayout();
	QPushButton* b_fprev = new QPushButton("<<", this);
	QPushButton* b_prev = new QPushButton("<", this);
	QPushButton* b_next = new QPushButton(">", this);
	QPushButton* b_fnext = new QPushButton(">>", this);
	b_fprev->setFixedWidth(20);
	b_prev->setFixedWidth(20);
	b_next->setFixedWidth(20);
	b_fnext->setFixedWidth(20);
	b_fprev->setAutoDefault(false);
	b_prev->setAutoDefault(false);
	b_next->setAutoDefault(false);
	b_fnext->setAutoDefault(false);
	hbox_tools_mem_buttons->addWidget(b_fprev);
	hbox_tools_mem_buttons->addWidget(b_prev);
	hbox_tools_mem_buttons->addWidget(b_next);
	hbox_tools_mem_buttons->addWidget(b_fnext);
	tools_mem_buttons->setLayout(hbox_tools_mem_buttons);

	// Merge Tools: Memory Viewer
	hbox_tools_mem->addWidget(tools_mem_addr);
	hbox_tools_mem->addWidget(tools_mem_words);
	hbox_tools_mem->addWidget(tools_mem_buttons);
	tools_mem->setLayout(hbox_tools_mem);

	// Tools: Raw Image Preview Options
	QGroupBox* tools_img = new QGroupBox(tr("Raw Image Preview Options"));
	QHBoxLayout* hbox_tools_img = new QHBoxLayout();;

	// Tools: Raw Image Preview Options : Size
	QGroupBox* tools_img_size = new QGroupBox(tr("Size"));
	QHBoxLayout* hbox_tools_img_size = new QHBoxLayout();
	QLabel* l_x = new QLabel(" x ");
	QSpinBox* sb_img_size_x = new QSpinBox(this);
	QSpinBox* sb_img_size_y = new QSpinBox(this);
	sb_img_size_x->setRange(1, 8192);
	sb_img_size_y->setRange(1, 8192);
	sb_img_size_x->setValue(256);
	sb_img_size_y->setValue(256);
	hbox_tools_img_size->addWidget(sb_img_size_x);
	hbox_tools_img_size->addWidget(l_x);
	hbox_tools_img_size->addWidget(sb_img_size_y);
	tools_img_size->setLayout(hbox_tools_img_size);

	// Tools: Raw Image Preview Options: Mode
	QGroupBox* tools_img_mode = new QGroupBox(tr("Mode"));
	QHBoxLayout* hbox_tools_img_mode = new QHBoxLayout();
	QComboBox* cbox_img_mode = new QComboBox(this);
	cbox_img_mode->addItem("RGB", QVariant::fromValue(color_format::RGB));
	cbox_img_mode->addItem("ARGB", QVariant::fromValue(color_format::ARGB));
	cbox_img_mode->addItem("RGBA", QVariant::fromValue(color_format::RGBA));
	cbox_img_mode->addItem("ABGR", QVariant::fromValue(color_format::ABGR));
	cbox_img_mode->setCurrentIndex(1); //ARGB
	hbox_tools_img_mode->addWidget(cbox_img_mode);
	tools_img_mode->setLayout(hbox_tools_img_mode);

	// Merge Tools: Raw Image Preview Options
	hbox_tools_img->addWidget(tools_img_size);
	hbox_tools_img->addWidget(tools_img_mode);
	tools_img->setLayout(hbox_tools_img);

	// Tools: Tool Buttons
	QGroupBox* tools_buttons = new QGroupBox(tr("Tools"));
	QVBoxLayout* hbox_tools_buttons = new QVBoxLayout(this);
	QPushButton* b_img = new QPushButton(tr("View\nimage"), this);
	b_img->setAutoDefault(false);
	hbox_tools_buttons->addWidget(b_img);
	tools_buttons->setLayout(hbox_tools_buttons);

	// Merge Tools = Memory Viewer Options + Raw Image Preview Options + Tool Buttons
	hbox_tools->addSpacing(10);
	hbox_tools->addWidget(tools_mem);
	hbox_tools->addWidget(tools_img);
	hbox_tools->addWidget(tools_buttons);
	hbox_tools->addSpacing(10);

	// Memory Panel:
	QHBoxLayout* hbox_mem_panel = new QHBoxLayout();

	// Memory Panel: Address Panel
	m_mem_addr = new QLabel("");
	m_mem_addr->setObjectName("memory_viewer_address_panel");
	m_mem_addr->setFont(mono);
	m_mem_addr->setAutoFillBackground(true);
	m_mem_addr->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_mem_addr->ensurePolished();

	// Memory Panel: Hex Panel
	m_mem_hex = new QLabel("");
	m_mem_hex->setObjectName("memory_viewer_hex_panel");
	m_mem_hex->setFont(mono);
	m_mem_hex->setAutoFillBackground(true);
	m_mem_hex->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_mem_hex->ensurePolished();

	// Memory Panel: ASCII Panel
	m_mem_ascii = new QLabel("");
	m_mem_ascii->setObjectName("memory_viewer_ascii_panel");
	m_mem_ascii->setFont(mono);
	m_mem_ascii->setAutoFillBackground(true);
	m_mem_ascii->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_mem_ascii->ensurePolished();

	// Merge Memory Panel:
	hbox_mem_panel->setAlignment(Qt::AlignLeft);
	hbox_mem_panel->addSpacing(20);
	hbox_mem_panel->addWidget(m_mem_addr);
	hbox_mem_panel->addSpacing(10);
	hbox_mem_panel->addWidget(m_mem_hex);
	hbox_mem_panel->addSpacing(10);
	hbox_mem_panel->addWidget(m_mem_ascii);
	hbox_mem_panel->addSpacing(10);

	// Set Margins to adjust WindowSize
	vbox_panel->setContentsMargins(0, 0, 0, 0);
	hbox_tools->setContentsMargins(0, 0, 0, 0);
	tools_mem_addr->setContentsMargins(0, 10, 0, 0);
	tools_mem_words->setContentsMargins(0, 10, 0, 0);
	tools_mem_buttons->setContentsMargins(0, 10, 0, 0);
	tools_img_mode->setContentsMargins(0, 10, 0, 0);
	tools_img_size->setContentsMargins(0, 10, 0, 0);
	tools_mem->setContentsMargins(0, 10, 0, 0);
	tools_img->setContentsMargins(0, 10, 0, 0);
	tools_buttons->setContentsMargins(0, 10, 0, 0);
	hbox_mem_panel->setContentsMargins(0, 0, 0, 0);

	// Merge and display everything
	vbox_panel->addSpacing(10);
	vbox_panel->addLayout(hbox_tools);
	vbox_panel->addSpacing(10);
	vbox_panel->addLayout(hbox_mem_panel);
	vbox_panel->addSpacing(10);
	setLayout(vbox_panel);

	// Events
	connect(m_addr_line, &QLineEdit::returnPressed, [this]()
	{
		bool ok;
		const QString text = m_addr_line->text();
		m_addr = (text.startsWith("0x", Qt::CaseInsensitive) ? text.right(text.size() - 2) : text).toULong(&ok, 16); 
		m_addr -= m_addr % (m_colcount * 4); // Align by amount of bytes in a row
		m_addr_line->setText(QString("%1").arg(m_addr, 8, 16, QChar('0')));	// get 8 digits in input line
		ShowMemory();
	});
	connect(sb_words, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=, this]()
	{
		m_colcount = sb_words->value();
		ShowMemory();
	});

	connect(b_prev, &QAbstractButton::clicked, [this]() { scroll(-1); });
	connect(b_next, &QAbstractButton::clicked, [this]() { scroll(1); });
	connect(b_fprev, &QAbstractButton::clicked, [this]() { scroll(m_rowcount * -1); });
	connect(b_fnext, &QAbstractButton::clicked, [this]() { scroll(m_rowcount); });
	connect(b_img, &QAbstractButton::clicked, [=, this]()
	{
		const color_format format = cbox_img_mode->currentData().value<color_format>();
		const int sizex = sb_img_size_x->value();
		const int sizey = sb_img_size_y->value();
		ShowImage(this, m_addr, format, sizex, sizey, false);
	});

	// Fill the QTextEdits
	ShowMemory();

	setFixedWidth(sizeHint().width());
	setMinimumHeight(hbox_tools->sizeHint().height());
}

memory_viewer_panel::~memory_viewer_panel()
{
	exit = true;
}

void memory_viewer_panel::wheelEvent(QWheelEvent *event)
{
	// Set some scrollspeed modifiers:
	u32 stepSize = 1;
	if (event->modifiers().testFlag(Qt::ControlModifier))
		stepSize *= m_rowcount;

	QPoint numSteps = event->angleDelta() / 8 / 15; // http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta

	scroll(stepSize * (0 - numSteps.y()));
}

void memory_viewer_panel::scroll(s32 steps)
{
	if (steps == 0) return;

	m_addr += m_colcount * 4 * steps;
	m_addr_line->setText(qstr(fmt::format("%08x", m_addr)));
	ShowMemory();
}

void memory_viewer_panel::resizeEvent(QResizeEvent *event)
{
	QDialog::resizeEvent(event);

	if (event->oldSize().height() != -1)
		m_height_leftover += event->size().height() - event->oldSize().height();

	const auto font_height = m_fontMetrics->height();

	if (m_height_leftover >= font_height)
	{
		m_height_leftover -= font_height;
		++m_rowcount;
		ShowMemory();
	}
	else if (m_height_leftover < -font_height)
	{
		m_height_leftover += font_height;
		if (m_rowcount > 0)
			--m_rowcount;
		ShowMemory();
	}
}

std::string memory_viewer_panel::getHeaderAtAddr(u32 addr)
{
	// Check if its an SPU Local Storage beginning
	const u32 spu_boundary = ::align<u32>(addr, SPU_LS_SIZE);

	if (spu_boundary <= addr + m_colcount * 4 - 1)
	{
		std::shared_ptr<named_thread<spu_thread>> spu;

		if (u32 raw_spu_index = (spu_boundary - RAW_SPU_BASE_ADDR) / SPU_LS_SIZE; raw_spu_index < 5)
		{
			spu = idm::get<named_thread<spu_thread>>(spu_thread::find_raw_spu(raw_spu_index));
		}
		else if (u32 spu_index = (spu_boundary - SPU_FAKE_BASE_ADDR) / SPU_LS_SIZE; spu_index < spu_thread::id_count)
		{
			spu = idm::get<named_thread<spu_thread>>(spu_thread::id_base | spu_index);
		}

		if (spu)
		{
			return spu->get_name();
		}
	}

	return {};
}

void memory_viewer_panel::ShowMemory()
{
	QString t_mem_addr_str;
	QString t_mem_hex_str;
	QString t_mem_ascii_str;

	for (u32 row = 0, spu_passed = 0; row < m_rowcount; row++)
	{
		if (row)
		{
			t_mem_addr_str += "\r\n";
			t_mem_hex_str += "\r\n";
			t_mem_ascii_str += "\r\n";
		}

		{
			// Check if this address contains potential header
			const u32 addr = m_addr + (row - spu_passed) * m_colcount * 4;
			const std::string header = getHeaderAtAddr(addr);
			if (!header.empty())
			{
				// Create an SPU header at the beginning of local storage
				// Like so:
				// =======================================
				// SPU[0x0000100] CellSpursKernel0
				// =======================================

				bool _break = false;

				for (u32 i = 0; i < 3; i++)
				{
					t_mem_addr_str += qstr(fmt::format("%08x", addr));

					std::string str(i == 1 ? header : "");

					const u32 expected_str_size = m_colcount * 13 - 2;

					// Truncate or enlarge string to a fixed size
					str.resize(expected_str_size);
					std::replace(str.begin(), str.end(), '\0', i == 1 ? ' ' : '=');

					t_mem_hex_str += qstr(str);

					spu_passed++;
					row++;

					if (row >= m_rowcount)
					{
						_break = true;
						break;
					}

					t_mem_addr_str += "\r\n";
					t_mem_hex_str += "\r\n";
					t_mem_ascii_str += "\r\n";
				}

				if (_break)
				{
					break;
				}
			}

			t_mem_addr_str += qstr(fmt::format("%08x", m_addr + (row - spu_passed) * m_colcount * 4));
		}

		for (u32 col = 0; col < m_colcount; col++)
		{
			if (col)
			{
				t_mem_hex_str += "  ";
			}

			u32 addr = m_addr + (row - spu_passed) * m_colcount * 4 + col * 4;

			if (vm::check_addr(addr, 0, 4))
			{
				const be_t<u32> rmem = *vm::get_super_ptr<u32>(addr);
				t_mem_hex_str += qstr(fmt::format("%02x %02x %02x %02x",
					static_cast<u8>(rmem >> 24),
					static_cast<u8>(rmem >> 16),
					static_cast<u8>(rmem >> 8),
					static_cast<u8>(rmem >> 0)));

				std::string str{reinterpret_cast<const char*>(&rmem), 4};

				for (auto& ch : str)
				{
					if (!std::isprint(static_cast<u8>(ch))) ch = '.';
				}

				t_mem_ascii_str += qstr(std::move(str));
			}
			else
			{
				t_mem_hex_str += "?? ?? ?? ??";
				t_mem_ascii_str += "????";
			}
		}
	}

	m_mem_addr->setText(t_mem_addr_str);
	m_mem_hex->setText(t_mem_hex_str);
	m_mem_ascii->setText(t_mem_ascii_str);

	// Adjust Text Boxes
	QSize textSize = m_fontMetrics->size(0, m_mem_addr->text());
	m_mem_addr->setFixedSize(textSize.width() + 10, textSize.height() + 10);

	textSize = m_fontMetrics->size(0, m_mem_hex->text());
	m_mem_hex->setFixedSize(textSize.width() + 10, textSize.height() + 10);

	textSize = m_fontMetrics->size(0, m_mem_ascii->text());
	m_mem_ascii->setFixedSize(textSize.width() + 10, textSize.height() + 10);
}

void memory_viewer_panel::SetPC(const uint pc)
{
	m_addr = pc;
}

void memory_viewer_panel::ShowImage(QWidget* parent, u32 addr, color_format format, u32 width, u32 height, bool flipv)
{
	if (width == 0 || height == 0)
	{
		return;
	}

	std::shared_lock rlock(vm::g_mutex);

	if (!vm::check_addr(addr, 0, width * height * 4))
	{
		return;
	}

	const auto originalBuffer  = vm::get_super_ptr<const uchar>(addr);
	const auto convertedBuffer = static_cast<uchar*>(std::malloc(4ULL * width * height));

	switch (format)
	{
	case color_format::RGB:
	{
		const u32 pitch = width * 3;
		const u32 pitch_new = width * 4;
		for (u32 y = 0; y < height; y++)
		{
			const u32 offset = y * pitch;
			const u32 offset_new = y * pitch_new;
			for (u32 x = 0, x_new = 0; x < pitch; x += 3, x_new += 4)
			{
				convertedBuffer[offset_new + x_new + 0] = originalBuffer[offset + x + 2];
				convertedBuffer[offset_new + x_new + 1] = originalBuffer[offset + x + 1];
				convertedBuffer[offset_new + x_new + 2] = originalBuffer[offset + x + 0];
				convertedBuffer[offset_new + x_new + 3] = 255;
			}
		}
		break;
	}
	case color_format::ARGB:
	{
		const u32 pitch = width * 4;
		for (u32 y = 0; y < height; y++)
		{
			const u32 offset = y * pitch;
			for (u32 x = 0; x < pitch; x += 4)
			{
				convertedBuffer[offset + x + 0] = originalBuffer[offset + x + 3];
				convertedBuffer[offset + x + 1] = originalBuffer[offset + x + 2];
				convertedBuffer[offset + x + 2] = originalBuffer[offset + x + 1];
				convertedBuffer[offset + x + 3] = originalBuffer[offset + x + 0];
			}
		}
		break;
	}
	case color_format::RGBA:
	{
		const u32 pitch = width * 4;
		for (u32 y = 0; y < height; y++)
		{
			const u32 offset = y * pitch;
			for (u32 x = 0; x < pitch; x += 4)
			{
				convertedBuffer[offset + x + 0] = originalBuffer[offset + x + 2];
				convertedBuffer[offset + x + 1] = originalBuffer[offset + x + 1];
				convertedBuffer[offset + x + 2] = originalBuffer[offset + x + 0];
				convertedBuffer[offset + x + 3] = originalBuffer[offset + x + 3];
			}
		}
		break;
	}
	case color_format::ABGR:
	{
		const u32 pitch = width * 4;
		for (u32 y = 0; y < height; y++)
		{
			const u32 offset = y * pitch;
			for (u32 x = 0; x < pitch; x += 4)
			{
				convertedBuffer[offset + x + 0] = originalBuffer[offset + x + 1];
				convertedBuffer[offset + x + 1] = originalBuffer[offset + x + 2];
				convertedBuffer[offset + x + 2] = originalBuffer[offset + x + 3];
				convertedBuffer[offset + x + 3] = originalBuffer[offset + x + 0];
			}
		}
		break;
	}
	}

	rlock.unlock();

	// Flip vertically
	if (flipv && height > 1)
	{
		const u32 pitch = width * 4;
		for (u32 y = 0; y < height / 2; y++)
		{
			const u32 offset = y * pitch;
			const u32 flip_offset = (height - y - 1) * pitch;
			for (u32 x = 0; x < pitch; x++)
			{
				const u8 tmp = convertedBuffer[offset + x];
				convertedBuffer[offset + x] = convertedBuffer[flip_offset + x];
				convertedBuffer[flip_offset + x] = tmp;
			}
		}
	}

	QImage image(convertedBuffer, width, height, QImage::Format_ARGB32, [](void* buffer){ std::free(buffer); }, convertedBuffer);
	if (image.isNull()) return;

	QLabel* canvas = new QLabel();
	canvas->setFixedSize(width, height);
	canvas->setPixmap(QPixmap::fromImage(image.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation)));

	QHBoxLayout* layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(canvas);

	QDialog* f_image_viewer = new QDialog(parent);
	f_image_viewer->setWindowTitle(qstr(fmt::format("Raw Image @ 0x%x", addr)));
	f_image_viewer->setFixedSize(QSize(width, height));
	f_image_viewer->setLayout(layout);
	f_image_viewer->show();
}
