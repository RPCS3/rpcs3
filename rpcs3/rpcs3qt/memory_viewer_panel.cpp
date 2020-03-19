#include "stdafx.h"
#include "Utilities/mutex.h"
#include "Emu/Memory/vm_locking.h"

#include "memory_viewer_panel.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>
#include <QTextEdit>
#include <QComboBox>
#include <QWheelEvent>
#include <shared_mutex>

constexpr auto qstr = QString::fromStdString;

memory_viewer_panel::memory_viewer_panel(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Memory Viewer"));
	setObjectName("memory_viewer");
	setAttribute(Qt::WA_DeleteOnClose);
	exit = false;
	m_addr = 0;
	m_colcount = 16;
	m_rowcount = 16;
	int pSize = 10;

	//Font
	QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	mono.setPointSize(pSize);
	m_fontMetrics = new QFontMetrics(mono);

	//Layout:
	QVBoxLayout* vbox_panel = new QVBoxLayout();

	//Tools
	QHBoxLayout* hbox_tools = new QHBoxLayout();

	//Tools: Memory Viewer Options
	QGroupBox* tools_mem = new QGroupBox(tr("Memory Viewer Options"));
	QHBoxLayout* hbox_tools_mem = new QHBoxLayout();

	//Tools: Memory Viewer Options: Address
	QGroupBox* tools_mem_addr = new QGroupBox(tr("Address"));
	QHBoxLayout* hbox_tools_mem_addr = new QHBoxLayout();
	m_addr_line = new QLineEdit(this);
	m_addr_line->setPlaceholderText("00000000");
	m_addr_line->setFont(mono);
	m_addr_line->setMaxLength(8);
	m_addr_line->setFixedWidth(75);
	m_addr_line->setFocus();
	hbox_tools_mem_addr->addWidget(m_addr_line);
	tools_mem_addr->setLayout(hbox_tools_mem_addr);

	//Tools: Memory Viewer Options: Bytes
	QGroupBox* tools_mem_bytes = new QGroupBox(tr("Bytes"));
	QHBoxLayout* hbox_tools_mem_bytes = new QHBoxLayout();
	QSpinBox* sb_bytes = new QSpinBox(this);
	sb_bytes->setRange(1, 16);
	sb_bytes->setValue(16);
	hbox_tools_mem_bytes->addWidget(sb_bytes);
	tools_mem_bytes->setLayout(hbox_tools_mem_bytes);

	//Tools: Memory Viewer Options: Control
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

	//Merge Tools: Memory Viewer
	hbox_tools_mem->addWidget(tools_mem_addr);
	hbox_tools_mem->addWidget(tools_mem_bytes);
	hbox_tools_mem->addWidget(tools_mem_buttons);
	tools_mem->setLayout(hbox_tools_mem);

	//Tools: Raw Image Preview Options
	QGroupBox* tools_img = new QGroupBox(tr("Raw Image Preview Options"));
	QHBoxLayout* hbox_tools_img = new QHBoxLayout();;

	//Tools: Raw Image Preview Options : Size
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

	//Tools: Raw Image Preview Options: Mode
	QGroupBox* tools_img_mode = new QGroupBox(tr("Mode"));
	QHBoxLayout* hbox_tools_img_mode = new QHBoxLayout();
	QComboBox* cbox_img_mode = new QComboBox(this);
	cbox_img_mode->addItem("RGB");
	cbox_img_mode->addItem("ARGB");
	cbox_img_mode->addItem("RGBA");
	cbox_img_mode->addItem("ABGR");
	cbox_img_mode->setCurrentIndex(1); //ARGB
	hbox_tools_img_mode->addWidget(cbox_img_mode);
	tools_img_mode->setLayout(hbox_tools_img_mode);

	//Merge Tools: Raw Image Preview Options
	hbox_tools_img->addWidget(tools_img_size);
	hbox_tools_img->addWidget(tools_img_mode);
	tools_img->setLayout(hbox_tools_img);

	//Tools: Tool Buttons
	QGroupBox* tools_buttons = new QGroupBox(tr("Tools"));
	QVBoxLayout* hbox_tools_buttons = new QVBoxLayout(this);
	QPushButton* b_img = new QPushButton(tr("View\nimage"), this);
	b_img->setAutoDefault(false);
	hbox_tools_buttons->addWidget(b_img);
	tools_buttons->setLayout(hbox_tools_buttons);

	//Merge Tools = Memory Viewer Options + Raw Image Preview Options + Tool Buttons
	hbox_tools->addSpacing(10);
	hbox_tools->addWidget(tools_mem);
	hbox_tools->addWidget(tools_img);
	hbox_tools->addWidget(tools_buttons);
	hbox_tools->addSpacing(10);

	//Memory Panel:
	QHBoxLayout* hbox_mem_panel = new QHBoxLayout();

	//Memory Panel: Address Panel
	m_mem_addr = new QLabel("");
	m_mem_addr->setObjectName("memory_viewer_address_panel");
	m_mem_addr->setFont(mono);
	m_mem_addr->setAutoFillBackground(true);
	m_mem_addr->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_mem_addr->ensurePolished();

	//Memory Panel: Hex Panel
	m_mem_hex = new QLabel("");
	m_mem_hex->setObjectName("memory_viewer_hex_panel");
	m_mem_hex->setFont(mono);
	m_mem_hex->setAutoFillBackground(true);
	m_mem_hex->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_mem_hex->ensurePolished();

	//Memory Panel: ASCII Panel
	m_mem_ascii = new QLabel("");
	m_mem_ascii->setObjectName("memory_viewer_ascii_panel");
	m_mem_ascii->setFont(mono);
	m_mem_ascii->setAutoFillBackground(true);
	m_mem_ascii->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_mem_ascii->ensurePolished();

	//Merge Memory Panel:
	hbox_mem_panel->setAlignment(Qt::AlignLeft);
	hbox_mem_panel->addSpacing(20);
	hbox_mem_panel->addWidget(m_mem_addr);
	hbox_mem_panel->addSpacing(10);
	hbox_mem_panel->addWidget(m_mem_hex);
	hbox_mem_panel->addSpacing(10);
	hbox_mem_panel->addWidget(m_mem_ascii);
	hbox_mem_panel->addSpacing(10);

	//Memory Panel: Set size of the QTextEdits
	m_mem_hex->setFixedSize(QSize(pSize * 3 * m_colcount + 6, 228));
	m_mem_ascii->setFixedSize(QSize(pSize * m_colcount + 6, 228));

	//Set Margins to adjust WindowSize
	vbox_panel->setContentsMargins(0, 0, 0, 0);
	hbox_tools->setContentsMargins(0, 0, 0, 0);
	tools_mem_addr->setContentsMargins(0, 10, 0, 0);
	tools_mem_bytes->setContentsMargins(0, 10, 0, 0);
	tools_mem_buttons->setContentsMargins(0, 10, 0, 0);
	tools_img_mode->setContentsMargins(0, 10, 0, 0);
	tools_img_size->setContentsMargins(0, 10, 0, 0);
	tools_mem->setContentsMargins(0, 10, 0, 0);
	tools_img->setContentsMargins(0, 10, 0, 0);
	tools_buttons->setContentsMargins(0, 10, 0, 0);
	hbox_mem_panel->setContentsMargins(0, 0, 0, 0);

	//Merge and display everything
	vbox_panel->addSpacing(10);
	vbox_panel->addLayout(hbox_tools);
	vbox_panel->addSpacing(10);
	vbox_panel->addLayout(hbox_mem_panel);
	vbox_panel->addSpacing(10);
	setLayout(vbox_panel);

	//Events
	connect(m_addr_line, &QLineEdit::returnPressed, [=, this]()
	{
		bool ok;
		m_addr = m_addr_line->text().toULong(&ok, 16);
		m_addr_line->setText(QString("%1").arg(m_addr, 8, 16, QChar('0')));	// get 8 digits in input line
		ShowMemory();
	});
	connect(sb_bytes, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=, this]()
	{
		m_colcount = sb_bytes->value();
		m_mem_hex->setFixedSize(QSize(pSize * 3 * m_colcount + 6, 228));
		m_mem_ascii->setFixedSize(QSize(pSize * m_colcount + 6, 228));
		ShowMemory();
	});

	connect(b_prev, &QAbstractButton::clicked, [=, this]() { m_addr -= m_colcount; ShowMemory(); });
	connect(b_next, &QAbstractButton::clicked, [=, this]() { m_addr += m_colcount; ShowMemory(); });
	connect(b_fprev, &QAbstractButton::clicked, [=, this]() { m_addr -= m_rowcount * m_colcount; ShowMemory(); });
	connect(b_fnext, &QAbstractButton::clicked, [=, this]() { m_addr += m_rowcount * m_colcount; ShowMemory(); });
	connect(b_img, &QAbstractButton::clicked, [=, this]()
	{
		int mode = cbox_img_mode->currentIndex();
		int sizex = sb_img_size_x->value();
		int sizey = sb_img_size_y->value();
		ShowImage(this, m_addr, mode, sizex, sizey, false);
	});

	//Fill the QTextEdits
	ShowMemory();
	setFixedSize(sizeHint());
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
	m_addr -= stepSize * m_colcount * numSteps.y();

	m_addr_line->setText(qstr(fmt::format("%08x", m_addr)));
	ShowMemory();
}

void memory_viewer_panel::ShowMemory()
{
	QString t_mem_addr_str;
	QString t_mem_hex_str;
	QString t_mem_ascii_str;

	for(u32 addr = m_addr; addr != m_addr + m_rowcount * m_colcount; addr += m_colcount)
	{
		t_mem_addr_str += qstr(fmt::format("%08x", addr));
		if (addr != m_addr + m_rowcount * m_colcount - m_colcount) t_mem_addr_str += "\r\n";
	}

	for (u32 row = 0; row < m_rowcount; row++)
	{
		for (u32 col = 0; col < m_colcount; col++)
		{
			u32 addr = m_addr + row * m_colcount + col;

			if (vm::check_addr(addr))
			{
				const u8 rmem = *vm::get_super_ptr<u8>(addr);
				t_mem_hex_str += qstr(fmt::format("%02x ", rmem));
				t_mem_ascii_str += qstr(std::string(1, std::isprint(rmem) ? static_cast<char>(rmem) : '.'));
			}
			else
			{
				t_mem_hex_str += "??";
				t_mem_ascii_str += "?";
				if (col != m_colcount - 1) t_mem_hex_str += " ";
			}
		}
		if (row != m_rowcount - 1)
		{
			t_mem_hex_str += "\r\n";
			t_mem_ascii_str += "\r\n";
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

void memory_viewer_panel::ShowImage(QWidget* parent, u32 addr, int mode, u32 width, u32 height, bool flipv)
{
	std::shared_lock rlock(vm::g_mutex);

	if (!vm::check_addr(addr, width * height * 4))
	{
		return;
	}

	const auto originalBuffer  = vm::get_super_ptr<const uchar>(addr);
	const auto convertedBuffer = static_cast<uchar*>(std::malloc(width * height * 4));

	switch(mode)
	{
	case(0): // RGB
		for (u32 y = 0; y < height; y++)
		{
			for (u32 i = 0, j = 0; j < width * 4; i += 4, j += 3)
			{
				convertedBuffer[i + 0 + y * width * 4] = originalBuffer[j + 2 + y * width * 3];
				convertedBuffer[i + 1 + y * width * 4] = originalBuffer[j + 1 + y * width * 3];
				convertedBuffer[i + 2 + y * width * 4] = originalBuffer[j + 0 + y * width * 3];
				convertedBuffer[i + 3 + y * width * 4] = 255;
			}
		}
	break;

	case(1): // ARGB
		for (u32 y = 0; y < height; y++)
		{
			for (u32 i = 0, j = 0; j < width * 4; i += 4, j += 4)
			{
				convertedBuffer[i + 0 + y * width * 4] = originalBuffer[j + 3 + y * width * 4];
				convertedBuffer[i + 1 + y * width * 4] = originalBuffer[j + 2 + y * width * 4];
				convertedBuffer[i + 2 + y * width * 4] = originalBuffer[j + 1 + y * width * 4];
				convertedBuffer[i + 3 + y * width * 4] = originalBuffer[j + 0 + y * width * 4];
			}
		}
	break;

	case(2): // RGBA
		for (u32 y = 0; y < height; y++)
		{
			for (u32 i = 0, j = 0; j < width * 4; i += 4, j += 4)
			{
				convertedBuffer[i + 0 + y * width * 4] = originalBuffer[j + 2 + y * width * 4];
				convertedBuffer[i + 1 + y * width * 4] = originalBuffer[j + 1 + y * width * 4];
				convertedBuffer[i + 2 + y * width * 4] = originalBuffer[j + 0 + y * width * 4];
				convertedBuffer[i + 3 + y * width * 4] = originalBuffer[j + 3 + y * width * 4];
			}
		}
	break;

	case(3): // ABGR
		for (u32 y = 0; y < height; y++)
		{
			for (u32 i = 0, j = 0; j < width * 4; i += 4, j += 4)
			{
				convertedBuffer[i + 0 + y * width * 4] = originalBuffer[j + 1 + y * width * 4];
				convertedBuffer[i + 1 + y * width * 4] = originalBuffer[j + 2 + y * width * 4];
				convertedBuffer[i + 2 + y * width * 4] = originalBuffer[j + 3 + y * width * 4];
				convertedBuffer[i + 3 + y * width * 4] = originalBuffer[j + 0 + y * width * 4];
			}
		}
	break;
	}

	rlock.unlock();

	// Flip vertically
	if (flipv)
	{
		for (u32 y = 0; y < height / 2; y++)
		{
			for (u32 x = 0; x < width * 4; x++)
			{
				const u8 t = convertedBuffer[x + y * width * 4];
				convertedBuffer[x + y * width * 4] = convertedBuffer[x + (height - y - 1) * width * 4];
				convertedBuffer[x + (height - y - 1) * width * 4] = t;
			}
		}
	}

	QImage image = QImage(convertedBuffer, width, height, QImage::Format_ARGB32, [](void* buffer){ std::free(buffer); }, convertedBuffer);
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
