#include "Emu/Memory/vm_locking.h"
#include "Emu/Memory/vm.h"

#include "memory_viewer_panel.h"
#include "hex_validator.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/rsx_utils.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QWheelEvent>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QThread>
#include <QKeyEvent>

#include "util/logs.hpp"
#include "util/asm.hpp"
#include "debugger_frame.h"

LOG_CHANNEL(gui_log, "GUI");

memory_viewer_panel::memory_viewer_panel(QWidget* parent, std::shared_ptr<CPUDisAsm> disasm, u32 addr, std::function<cpu_thread*()> func)
	: QDialog(parent)
	, m_addr(addr)
	, m_get_cpu(func ? std::move(func) : std::function<cpu_thread*()>(FN(nullptr)))
	, m_type([&]()
	{
		const auto cpu = m_get_cpu();
		if (!cpu) return thread_class::general;

		thread_class type = cpu->get_class();

		switch (type)
		{
		case thread_class::ppu:
		case thread_class::spu:
		case thread_class::rsx:
			break;
		default:
			fmt::throw_exception("Unknown CPU type (0x%x)", cpu->id_type());
		}

		return type;
	}())
	, m_rsx(m_type == thread_class::rsx ? static_cast<rsx::thread*>(m_get_cpu()) : nullptr)
	, m_spu_shm([&]()
	{
		const auto cpu = m_get_cpu();
		return cpu && m_type == thread_class::spu ? static_cast<spu_thread*>(cpu)->shm : nullptr;
	}())
	, m_addr_mask(m_type == thread_class::spu ? SPU_LS_SIZE - 1 : ~0)
	, m_disasm(std::move(disasm))
{
	const auto cpu = m_get_cpu();

	setWindowTitle(
		cpu && m_type == thread_class::spu ? tr("Memory Viewer Of %0").arg(QString::fromStdString(cpu->get_name())) :
		cpu && m_type == thread_class::rsx ? tr("Memory Viewer Of RSX[0x55555555]") :
		tr("Memory Viewer"));

	setObjectName("memory_viewer");
	m_colcount = 4;
	m_rowcount = 1;
	const int pSize = 10;

	// Font
	QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	mono.setPointSize(pSize);
	m_fontMetrics = new QFontMetrics(mono);

	// Layout:
	QVBoxLayout* vbox_panel = new QVBoxLayout(this);

	// Tools
	QHBoxLayout* hbox_tools = new QHBoxLayout(this);

	// Tools: Memory Viewer Options
	QGroupBox* tools_mem = new QGroupBox(tr("Memory Viewer Options"), this);
	QHBoxLayout* hbox_tools_mem = new QHBoxLayout(this);

	// Tools: Memory Viewer Options: Address
	QGroupBox* tools_mem_addr = new QGroupBox(tr("Address"), this);
	QHBoxLayout* hbox_tools_mem_addr = new QHBoxLayout(this);
	m_addr_line = new QLineEdit(this);
	m_addr_line->setPlaceholderText("00000000");
	m_addr_line->setFont(mono);
	m_addr_line->setValidator(new HexValidator(m_addr_line));
	m_addr_line->setFixedWidth(100);
	m_addr_line->setFocus();
	m_addr_line->setAlignment(Qt::AlignCenter);
	hbox_tools_mem_addr->addWidget(m_addr_line);
	tools_mem_addr->setLayout(hbox_tools_mem_addr);

	// Tools: Memory Viewer Options: Words
	QGroupBox* tools_mem_words = new QGroupBox(tr("Words"), this);
	QHBoxLayout* hbox_tools_mem_words = new QHBoxLayout();

	class words_spin_box : public QSpinBox
	{
	public:
		words_spin_box(QWidget* parent = nullptr) : QSpinBox(parent) {}
		~words_spin_box() override {}

	private:
		int valueFromText(const QString &text) const override
		{
			return std::countr_zero(text.toULong());
		}

		QString textFromValue(int value) const override
		{
			return QString("%0").arg(1 << value);
		}
	};

	words_spin_box* sb_words = new words_spin_box(this);
	sb_words->setRange(0, 2);
	sb_words->setValue(2);
	hbox_tools_mem_words->addWidget(sb_words);
	tools_mem_words->setLayout(hbox_tools_mem_words);

	// Tools: Memory Viewer Options: Control
	QGroupBox* tools_mem_buttons = new QGroupBox(tr("Control"));
	QHBoxLayout* hbox_tools_mem_buttons = new QHBoxLayout(this);
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

	QGroupBox* tools_mem_refresh = new QGroupBox(tr("Refresh"));
	QHBoxLayout* hbox_tools_mem_refresh = new QHBoxLayout(this);
	QPushButton* button_auto_refresh = new QPushButton(QStringLiteral(" "), this);
	button_auto_refresh->setFixedWidth(20);
	button_auto_refresh->setAutoDefault(false);
	hbox_tools_mem_refresh->addWidget(button_auto_refresh);
	tools_mem_refresh->setLayout(hbox_tools_mem_refresh);

	// Merge Tools: Memory Viewer
	hbox_tools_mem->addWidget(tools_mem_addr);
	hbox_tools_mem->addWidget(tools_mem_words);
	hbox_tools_mem->addWidget(tools_mem_buttons);
	hbox_tools_mem->addWidget(tools_mem_refresh);
	tools_mem->setLayout(hbox_tools_mem);

	// Tools: Raw Image Preview Options
	QGroupBox* tools_img = new QGroupBox(tr("Raw Image Preview Options"), this);
	QHBoxLayout* hbox_tools_img = new QHBoxLayout(this);

	// Tools: Raw Image Preview Options : Size
	QGroupBox* tools_img_size = new QGroupBox(tr("Size"), this);
	QHBoxLayout* hbox_tools_img_size = new QHBoxLayout(this);
	QLabel* l_x = new QLabel(" x ");
	QSpinBox* sb_img_size_x = new QSpinBox(this);
	QSpinBox* sb_img_size_y = new QSpinBox(this);
	sb_img_size_x->setRange(1, m_type == thread_class::spu ? 256 : 4096);
	sb_img_size_y->setRange(1, m_type == thread_class::spu ? 256 : 4096);
	sb_img_size_x->setValue(256);
	sb_img_size_y->setValue(256);
	hbox_tools_img_size->addWidget(sb_img_size_x);
	hbox_tools_img_size->addWidget(l_x);
	hbox_tools_img_size->addWidget(sb_img_size_y);
	tools_img_size->setLayout(hbox_tools_img_size);

	// Tools: Raw Image Preview Options: Mode
	QGroupBox* tools_img_mode = new QGroupBox(tr("Mode"), this);
	QHBoxLayout* hbox_tools_img_mode = new QHBoxLayout(this);
	QComboBox* cbox_img_mode = new QComboBox(this);
	cbox_img_mode->addItem("RGB", QVariant::fromValue(color_format::RGB));
	cbox_img_mode->addItem("ARGB", QVariant::fromValue(color_format::ARGB));
	cbox_img_mode->addItem("RGBA", QVariant::fromValue(color_format::RGBA));
	cbox_img_mode->addItem("ABGR", QVariant::fromValue(color_format::ABGR));
	cbox_img_mode->addItem("G8", QVariant::fromValue(color_format::G8));
	cbox_img_mode->addItem("G32MAX", QVariant::fromValue(color_format::G32MAX));
	cbox_img_mode->setCurrentIndex(1); //ARGB
	hbox_tools_img_mode->addWidget(cbox_img_mode);
	tools_img_mode->setLayout(hbox_tools_img_mode);

	// Merge Tools: Raw Image Preview Options
	hbox_tools_img->addWidget(tools_img_size);
	hbox_tools_img->addWidget(tools_img_mode);
	tools_img->setLayout(hbox_tools_img);

	// Tools: Tool Buttons
	QGroupBox* tools_buttons = new QGroupBox(tr("Tools"), this);
	QVBoxLayout* hbox_tools_buttons = new QVBoxLayout(this);
	QPushButton* b_img = new QPushButton(tr("View\nimage"), this);
	b_img->setAutoDefault(false);
	hbox_tools_buttons->addWidget(b_img);
	tools_buttons->setLayout(hbox_tools_buttons);

	// Merge Tools = Memory Viewer Options + Raw Image Preview Options + Tool Buttons
	hbox_tools->addSpacing(20);
	hbox_tools->addWidget(tools_mem);
	hbox_tools->addWidget(tools_img);
	hbox_tools->addWidget(tools_buttons);
	hbox_tools->addSpacing(20);

	// Memory Panel:
	m_hbox_mem_panel = new QHBoxLayout(this);

	// Memory Panel: Address Panel
	m_mem_addr = new QLabel("");

	QSizePolicy sp_retain = m_mem_addr->sizePolicy();
	sp_retain.setRetainSizeWhenHidden(false);

	m_mem_addr->setSizePolicy(sp_retain);
	m_mem_addr->setObjectName("memory_viewer_address_panel");
	m_mem_addr->setFont(mono);
	m_mem_addr->setAutoFillBackground(true);
	m_mem_addr->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_mem_addr->ensurePolished();

	// Memory Panel: Hex Panel
	m_mem_hex = new QLabel("");
	m_mem_hex->setSizePolicy(sp_retain);
	m_mem_hex->setObjectName("memory_viewer_hex_panel");
	m_mem_hex->setFont(mono);
	m_mem_hex->setAutoFillBackground(true);
	m_mem_hex->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_mem_hex->ensurePolished();

	// Memory Panel: ASCII Panel
	m_mem_ascii = new QLabel("");
	m_mem_ascii->setSizePolicy(sp_retain);
	m_mem_ascii->setObjectName("memory_viewer_ascii_panel");
	m_mem_ascii->setFont(mono);
	m_mem_ascii->setAutoFillBackground(true);
	m_mem_ascii->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
	m_mem_ascii->ensurePolished();

	// Merge Memory Panel:
	m_hbox_mem_panel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	m_hbox_mem_panel->addSpacing(20);
	m_hbox_mem_panel->addWidget(m_mem_addr);
	m_hbox_mem_panel->addSpacing(10);
	m_hbox_mem_panel->addWidget(m_mem_hex);
	m_hbox_mem_panel->addSpacing(10);
	m_hbox_mem_panel->addWidget(m_mem_ascii);
	m_hbox_mem_panel->addSpacing(20);

	QHBoxLayout* hbox_memory_search = new QHBoxLayout(this);

	// Set Margins to adjust WindowSize
	vbox_panel->setContentsMargins(0, 0, 0, 0);
	hbox_tools->setContentsMargins(0, 0, 0, 0);
	tools_mem_addr->setContentsMargins(0, 5, 0, 0);
	tools_mem_words->setContentsMargins(0, 5, 0, 0);
	tools_mem_buttons->setContentsMargins(0, 5, 0, 0);
	tools_img_mode->setContentsMargins(0, 5, 0, 0);
	tools_img_size->setContentsMargins(0, 5, 0, 0);
	tools_mem->setContentsMargins(0, 5, 0, 0);
	tools_img->setContentsMargins(0, 5, 0, 0);
	tools_buttons->setContentsMargins(0, 5, 0, 0);
	m_hbox_mem_panel->setContentsMargins(0, 0, 0, 0);
	hbox_memory_search->setContentsMargins(0, 0, 0, 0);

	if (m_disasm)
	{
		// Extract memory view from the disassembler
		std::tie(m_ptr, m_size) = m_disasm->get_memory_span();
	}

	QGroupBox* group_search = new QGroupBox(tr("Memory Search"), this);
	QPushButton* button_collapse_viewer = new QPushButton(reinterpret_cast<const char*>(u8"Ʌ"), group_search);
	button_collapse_viewer->setFixedWidth(QLabel(button_collapse_viewer->text()).sizeHint().width() * 3);
	button_collapse_viewer->setAutoDefault(false);
	
	m_search_line = new QLineEdit(group_search);
	m_search_line->setFixedWidth(QLabel(QString("This is the very length of the lineedit due to hidpi reasons.").chopped(4)).sizeHint().width());
	m_search_line->setPlaceholderText(tr("Search..."));
	m_search_line->setMaxLength(4096);

	QPushButton* button_search = new QPushButton(tr("Search"), group_search);
	button_search->setEnabled(false);

	m_chkbox_case_insensitive = new QCheckBox(tr("Case Insensitive"), group_search);
	m_chkbox_case_insensitive->setCheckable(true);
	m_chkbox_case_insensitive->setToolTip(tr("When using string mode, the characters' case will not matter both in string and in memory."
		"\nWarning: this may reduce performance of the search."));

	m_cbox_input_mode = new QComboBox(group_search);
	m_cbox_input_mode->addItem(tr("Select search mode(s).."), QVariant::fromValue(+no_mode));
	m_cbox_input_mode->addItem(tr("Deselect All Modes"), QVariant::fromValue(+clear_modes));
	m_cbox_input_mode->addItem(tr("String"), QVariant::fromValue(+as_string));
	m_cbox_input_mode->addItem(tr("HEX bytes/integer"), QVariant::fromValue(+as_hex));
	m_cbox_input_mode->addItem(tr("Double"), QVariant::fromValue(+as_f64));
	m_cbox_input_mode->addItem(tr("Float"), QVariant::fromValue(+as_f32));
	m_cbox_input_mode->addItem(tr("Instruction"), QVariant::fromValue(+as_inst));
	m_cbox_input_mode->addItem(tr("RegEx Instruction"), QVariant::fromValue(+as_regex_inst));

	QString tooltip = tr("String: search the memory for the specified string."
		"\nHEX bytes/integer: search the memory for hexadecimal values. Spaces, commas, \"0x\", \"0X\", \"\\x\", \"h\", \"H\" ensure separation of bytes but they are not mandatory."
		"\nDouble: reinterpret the string as 64-bit precision floating point value. Values are searched for exact representation, meaning -0 != 0."
		"\nFloat: reinterpret the string as 32-bit precision floating point value. Values are searched for exact representation, meaning -0 != 0."
		"\nInstruction: search an instruction contains the text of the string."
		"\nRegEx: search an instruction containing text that matches the regular expression input.");

	if (m_size != 0x40000/*SPU_LS_SIZE*/)
	{
		m_cbox_input_mode->addItem("SPU Instruction", QVariant::fromValue(+as_fake_spu_inst));
		m_cbox_input_mode->addItem(tr("SPU RegEx-Instruction"), QVariant::fromValue(+as_regex_fake_spu_inst));
		tooltip.append(tr("\nSPU Instruction: Search an SPU instruction contains the text of the string. For searching instructions within embedded SPU images.\nTip: SPU floats are commented along forming instructions."));
	}

	connect(m_cbox_input_mode, QOverload<int>::of(&QComboBox::currentIndexChanged), group_search, [this, button_search](int index)
	{
		if (index < 1 || m_rsx)
		{
			return;
		}

		if ((1u << index) == clear_modes)
		{
			m_modes = {};
		}
		else
		{
			m_modes = search_mode{m_modes | (1 << index)};
		}

		const s32 count = std::popcount(+m_modes);

		if (count == 0)
		{
			button_search->setEnabled(false);
			m_cbox_input_mode->setItemText(0, tr("Select search mode(s).."));
		}
		else
		{
			button_search->setEnabled(true);
			m_cbox_input_mode->setItemText(0, tr("%0 mode(s) selected").arg(count));
		}

		for (u32 i = search_mode_last / 2; i > clear_modes; i /= 2)
		{
			if (i & m_modes && count > 1)
			{
				m_cbox_input_mode->setItemText(std::countr_zero<u32>(i), QString::fromStdString(fmt::format("* %s", search_mode{i})));
			}
			else
			{
				m_cbox_input_mode->setItemText(std::countr_zero<u32>(i), QString::fromStdString(fmt::format("%s", search_mode{i})));
			}
		}

		if (count != 1)
		{
			m_cbox_input_mode->setCurrentIndex(0);
		}
	});

	m_cbox_input_mode->setToolTip(tooltip);

	QVBoxLayout* vbox_search_layout = new QVBoxLayout(group_search);

	QHBoxLayout* hbox_search_panel = new QHBoxLayout(group_search);
	QHBoxLayout* hbox_search_modes = new QHBoxLayout(group_search);

	hbox_search_panel->addWidget(button_collapse_viewer);
	hbox_search_panel->addWidget(m_search_line);
	hbox_search_panel->addWidget(m_cbox_input_mode);
	hbox_search_panel->addWidget(m_chkbox_case_insensitive);
	hbox_search_panel->addWidget(button_search);

	vbox_search_layout->addLayout(hbox_search_panel);
	vbox_search_layout->addLayout(hbox_search_modes);
	group_search->setLayout(vbox_search_layout);

	hbox_memory_search->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
	hbox_memory_search->addSpacing(20);
	hbox_memory_search->addWidget(group_search);
	hbox_memory_search->addSpacing(20);

	// Merge and display everything
	vbox_panel->addSpacing(10);

	auto get_row = [row = 0]() mutable
	{
		return row++;
	};

	vbox_panel->addLayout(hbox_tools, get_row());
	vbox_panel->addSpacing(5);
	vbox_panel->addLayout(m_hbox_mem_panel, get_row());

	// TODO: RSX memory searcher
	if (!m_rsx)
	{
		vbox_panel->addLayout(hbox_memory_search, get_row());
		vbox_panel->addSpacing(15);
	}
	else
	{
		group_search->deleteLater();
	}

	vbox_panel->setSizeConstraint(QLayout::SetNoConstraint);
	setLayout(vbox_panel);

	// Events
	connect(m_addr_line, &QLineEdit::returnPressed, this, [this]()
	{
		bool ok = false;
		const u32 addr = normalize_hex_qstring(m_addr_line->text()).toULong(&ok, 16);
		if (ok) m_addr = addr;

		scroll(0); // Refresh
	});
	connect(sb_words, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [=, this]()
	{
		m_colcount = 1 << sb_words->value();
		ShowMemory();
	});

	connect(b_prev, &QAbstractButton::clicked, this, [this]() { scroll(-1); });
	connect(b_next, &QAbstractButton::clicked, this, [this]() { scroll(1); });
	connect(b_fprev, &QAbstractButton::clicked, this, [this]() { scroll(m_rowcount * -1); });
	connect(b_fnext, &QAbstractButton::clicked, this, [this]() { scroll(m_rowcount); });
	connect(b_img, &QAbstractButton::clicked, this, [=, this]()
	{
		const color_format format = cbox_img_mode->currentData().value<color_format>();
		const int sizex = sb_img_size_x->value();
		const int sizey = sb_img_size_y->value();
		ShowImage(this, m_addr, format, sizex, sizey, false);
	});

	QTimer* auto_refresh_timer = new QTimer(this);

	connect(auto_refresh_timer, &QTimer::timeout, this, [this]()
	{
		ShowMemory();
	});

	connect(button_auto_refresh, &QAbstractButton::clicked, this, [=, this]()
	{
		const bool is_checked = button_auto_refresh->text() != " ";
		if (auto_refresh_timer->isActive() != is_checked)
		{
			return;
		}

		if (is_checked)
		{
			button_auto_refresh->setText(QStringLiteral(" "));
			auto_refresh_timer->stop();
		}
		else
		{
			button_auto_refresh->setText(reinterpret_cast<const char*>(u8"█"));
			ShowMemory();
			auto_refresh_timer->start(16);
		}
	});

	if (!m_rsx)
	{
		connect(button_search, &QAbstractButton::clicked, this, [this]()
		{
			if (m_search_thread && m_search_thread->isRunning())
			{
				// Prevent spamming (search is costly on performance)
				return;
			}

			if (m_search_thread)
			{
				m_search_thread->deleteLater();
				m_search_thread = nullptr;
			}

			std::string wstr = m_search_line->text().toStdString();

			if (wstr.empty() || wstr.size() >= 4096u)
			{
				gui_log.error("String is empty or too long (size=%u)", wstr.size());
				return;
			}

			m_search_thread = QThread::create([this, wstr, m_modes = m_modes]()
			{
				gui_log.notice("Searching for %s (mode: %s)", wstr, m_modes);

				u64 found = 0;

				for (int modes = m_modes; modes; modes &= modes - 1)
				{
					found += OnSearch(wstr, modes & ~(modes - 1));
				}

				gui_log.success("Search completed (found %u matches)", +found);
			});

			m_search_thread->start();
		});

		connect(button_collapse_viewer, &QAbstractButton::clicked, this, [this, button_collapse_viewer, m_previous_row_count = -1]() mutable
		{
			const bool is_collapsing = button_collapse_viewer->text() == reinterpret_cast<const char*>(u8"Ʌ");
			button_collapse_viewer->setText(is_collapsing ? "V" : reinterpret_cast<const char*>(u8"Ʌ"));

			if (is_collapsing)
			{
				m_previous_row_count = std::exchange(m_rowcount, 0);
				setMinimumHeight(0);
			}
			else
			{
				m_rowcount = std::exchange(m_previous_row_count, 0);
				setMaximumHeight(16777215); // Default Qt value
			}

			ShowMemory();

			QTimer::singleShot(0, this, [this, button_collapse_viewer]()
			{
				const bool is_collapsing = button_collapse_viewer->text() != reinterpret_cast<const char*>(u8"Ʌ");

				// singleShot to evaluate properly after the event
				const int height_hint = sizeHint().height();
				resize(size().width(), height_hint);

				if (is_collapsing)
				{
					setMinimumHeight(height_hint);
					setMaximumHeight(height_hint + 1);
				}
				else
				{
					setMinimumHeight(m_min_height);
				}
			});
		});
	}

	// Set the minimum height of one row
	m_rowcount = 1;
	ShowMemory();
	m_min_height = sizeHint().height();
	setMinimumHeight(m_min_height);

	m_rowcount = 16;
	ShowMemory();

	setFixedWidth(sizeHint().width());

	// Fill the QTextEdits
	scroll(0);

	// Show by default
	show();

	// Expected to be created by IDM, emulation stop will close it
	const u32 id = idm::last_id();
	auto handle_ptr = idm::get_unlocked<memory_viewer_handle>(id);

	connect(this, &memory_viewer_panel::finished, this, [handle_ptr = std::move(handle_ptr), id, this](int)
	{
		if (m_search_thread)
		{
			m_search_thread->wait();
			m_search_thread->deleteLater();
			m_search_thread = nullptr;
		}

		idm::remove_verify<memory_viewer_handle>(id, handle_ptr);
	});

	if (!g_fxo->try_get<memory_viewer_fxo>())
	{
		g_fxo->init<memory_viewer_fxo>();
	}

	auto& fxo = g_fxo->get<memory_viewer_fxo>();
	fxo.last_opened[m_type] = this;

	connect(this, &memory_viewer_panel::destroyed, this, [this]()
	{
		if (auto fxo = g_fxo->try_get<memory_viewer_fxo>())
		{
			auto it = fxo->last_opened.find(m_type);
			if (it != fxo->last_opened.end() && it->second == this)
				fxo->last_opened.erase(it);
		}
	});
}

memory_viewer_panel::~memory_viewer_panel()
{
}

void memory_viewer_panel::wheelEvent(QWheelEvent *event)
{
	// Set some scroll speed modifiers:
	u32 step_size = 1;
	if (event->modifiers().testFlag(Qt::ControlModifier))
		step_size *= m_rowcount;

	const QPoint num_steps = event->angleDelta() / 8 / 15; // http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta

	scroll(step_size * (0 - num_steps.y()));
}

void memory_viewer_panel::scroll(s32 steps)
{
	m_addr += m_colcount * 4 * steps; // Add steps
	m_addr &= m_addr_mask; // Mask it
	m_addr -= m_addr % (m_colcount * 4); // Align by amount of bytes in a row

	m_addr_line->setText(QString::fromStdString(fmt::format("%08x", m_addr)));

	ShowMemory();
}

void memory_viewer_panel::resizeEvent(QResizeEvent *event)
{
	QDialog::resizeEvent(event);

	const int font_height  = m_fontMetrics->height();
	const QMargins margins = layout()->contentsMargins();

	int free_height = event->size().height()
		- (layout()->count() * (margins.top() + margins.bottom())) - c_pad_memory_labels;

	for (int i = 0; i < layout()->count(); i++)
	{
		const auto it = layout()->itemAt(i);
		if (it != m_hbox_mem_panel) // Do not take our memory layout into account
			free_height -= it->sizeHint().height();
	}

	const u32 new_row_count = std::max(0, free_height) / font_height;

	if (m_rowcount != new_row_count)
	{
		m_rowcount = new_row_count;

		QTimer::singleShot(0, [this]()
		{
			// Prevent recursion of events
			ShowMemory();
		});
	}
}

std::string memory_viewer_panel::getHeaderAtAddr(u32 addr) const
{
	if (m_type == thread_class::spu) return {};

	// Check if its an SPU Local Storage beginning
	const u32 spu_boundary = utils::align<u32>(addr, SPU_LS_SIZE);

	if (spu_boundary <= addr + m_colcount * 4 - 1)
	{
		shared_ptr<named_thread<spu_thread>> spu;

		if (const u32 raw_spu_index = (spu_boundary - RAW_SPU_BASE_ADDR) / SPU_LS_SIZE; raw_spu_index < 5)
		{
			spu = idm::get_unlocked<named_thread<spu_thread>>(spu_thread::find_raw_spu(raw_spu_index));

			if (spu && spu->get_type() == spu_type::threaded)
			{
				spu.reset();
			}
		}
		else if (const u32 spu_index = (spu_boundary - SPU_FAKE_BASE_ADDR) / SPU_LS_SIZE; spu_index < spu_thread::id_count)
		{
			spu = idm::get_unlocked<named_thread<spu_thread>>(spu_thread::id_base | spu_index);

			if (spu && spu->get_type() != spu_type::threaded)
			{
				spu.reset();
			}
		}

		if (spu)
		{
			return spu->get_name();
		}
	}

	return {};
}

void* memory_viewer_panel::to_ptr(u32 addr, u32 size) const
{
	if (m_type >= thread_class::spu && !m_get_cpu())
	{
		return nullptr;
	}

	if (!size)
	{
		return nullptr;
	}

	switch (m_type)
	{
	case thread_class::general:
	case thread_class::ppu:
	{
		if (vm::check_addr(addr, 0, size))
		{
			return vm::get_super_ptr(addr);
		}

		break;
	}
	case thread_class::spu:
	{
		if (size <= SPU_LS_SIZE && SPU_LS_SIZE - size >= (addr % SPU_LS_SIZE))
		{
			return m_spu_shm->map_self() + (addr % SPU_LS_SIZE);
		}

		break;
	}
	case thread_class::rsx:
	{
		u32 final_addr = 0;

		constexpr u32 local_mem = rsx::constants::local_mem_base;

		if (size > 0x2000'0000 || local_mem + 0x1000'0000 - size < addr)
		{
			break;
		}

		for (u32 i = addr; i >> 20 <= (addr + size - 1) >> 20; i += 0x100000)
		{
			const u32 temp = rsx::get_address(i - (i >= local_mem ? local_mem : 0), i < local_mem ? CELL_GCM_LOCATION_MAIN : CELL_GCM_LOCATION_LOCAL, true);

			if (!temp)
			{
				// Failure
				final_addr = 0;
				break;
			}

			if (!final_addr)
			{
				// First time, save starting address for later checks
				final_addr = temp;
			}
			else if (final_addr != temp - (i - addr))
			{
				// TODO: Non-contiguous memory
				final_addr = 0;
				break;
			}
		}

		if (vm::check_addr(final_addr, 0, size))
		{
			return vm::get_super_ptr(final_addr);
		}

		break;
	}
	}

	return nullptr;
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
			const u32 addr = (m_addr + (row - spu_passed) * m_colcount * 4) & m_addr_mask;
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
					t_mem_addr_str += QString::fromStdString(fmt::format("%08x", addr));

					std::string str(i == 1 ? header : "");

					const u32 expected_str_size = m_colcount * 13 - 2;

					// Truncate or enlarge string to a fixed size
					str.resize(expected_str_size);
					std::replace(str.begin(), str.end(), '\0', i == 1 ? ' ' : '=');

					t_mem_hex_str += QString::fromStdString(str);

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

			t_mem_addr_str += QString::fromStdString(fmt::format("%08x", (m_addr + (row - spu_passed) * m_colcount * 4) & m_addr_mask));
		}

		for (u32 col = 0; col < m_colcount; col++)
		{
			if (col)
			{
				t_mem_hex_str += "  ";
			}

			const u32 addr = (m_addr + (row - spu_passed) * m_colcount * 4 + col * 4) & m_addr_mask;

			if (const auto ptr = this->to_ptr(addr))
			{
				const be_t<u32> rmem = read_from_ptr<be_t<u32>>(static_cast<const u8*>(ptr));
				t_mem_hex_str += QString::fromStdString(fmt::format("%02x %02x %02x %02x",
					static_cast<u8>(rmem >> 24),
					static_cast<u8>(rmem >> 16),
					static_cast<u8>(rmem >> 8),
					static_cast<u8>(rmem >> 0)));

				std::string str{reinterpret_cast<const char*>(&rmem), 4};

				for (auto& ch : str)
				{
					if (!std::isprint(static_cast<u8>(ch))) ch = '.';
				}

				t_mem_ascii_str += QString::fromStdString(std::move(str));
			}
			else
			{
				t_mem_hex_str += "?? ?? ?? ??";
				t_mem_ascii_str += "????";
			}
		}
	}

	m_mem_addr->setVisible(m_rowcount != 0);
	m_mem_hex->setVisible(m_rowcount != 0);
	m_mem_ascii->setVisible(m_rowcount != 0);

	if (t_mem_addr_str != m_mem_addr->text())
		m_mem_addr->setText(t_mem_addr_str);

	if (t_mem_hex_str != m_mem_hex->text())
		m_mem_hex->setText(t_mem_hex_str);

	if (t_mem_ascii_str != m_mem_ascii->text())
		m_mem_ascii->setText(t_mem_ascii_str);

	auto mask_height = [&](int height)
	{
		return m_rowcount != 0 ? height + c_pad_memory_labels : 0;
	};

	// Adjust Text Boxes (also helps with window resize)
	QSize textSize = m_fontMetrics->size(0, m_mem_addr->text());
	m_mem_addr->setFixedSize(textSize.width() + 10, mask_height(textSize.height()));

	textSize = m_fontMetrics->size(0, m_mem_hex->text());
	m_mem_hex->setFixedSize(textSize.width() + 10, mask_height(textSize.height()));

	textSize = m_fontMetrics->size(0, m_mem_ascii->text());
	m_mem_ascii->setFixedSize(textSize.width() + 10, mask_height(textSize.height()));
}

void memory_viewer_panel::SetPC(const uint pc)
{
	m_addr = pc;
}

void memory_viewer_panel::keyPressEvent(QKeyEvent* event)
{
	if (!isActiveWindow())
	{
		QDialog::keyPressEvent(event);
		return;
	}

	if (event->modifiers() == Qt::ControlModifier)
	{
		switch (const auto key = event->key())
		{
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
		{
			scroll(key == Qt::Key_PageDown ? m_rowcount : 0u - m_rowcount);
			break;
		}
		case Qt::Key_F5:
		{
			if (event->isAutoRepeat())
			{
				break;
			}

			// Single refresh
			ShowMemory();
			break;
		}
		case Qt::Key_F:
		{
			m_addr_line->setFocus();
			break;
		}
		default: break;
		}
	}

	QDialog::keyPressEvent(event);
}

void memory_viewer_panel::ShowImage(QWidget* parent, u32 addr, color_format format, u32 width, u32 height, bool flipv) const
{
	u32 texel_bytes = 4;

	switch (format)
	{
	case color_format::RGB:
	{
		texel_bytes = 3;
		break;
	}
	case color_format::G8:
	{
		texel_bytes = 1;
		break;
	}
	default: break;
	}

	// If exceeds 32-bits it is invalid as well, UINT32_MAX always fails checks
	const u32 memsize = utils::mul_saturate<u32>(utils::mul_saturate<u32>(texel_bytes, width), height);
	if (memsize == 0)
	{
		gui_log.error("Can not show image. memsize is 0 (texel_bytes=%d, width=%d, height=%d)", texel_bytes, width, height);
		return;
	}

	const auto originalBuffer  = static_cast<u8*>(this->to_ptr(addr, memsize));

	if (!originalBuffer)
	{
		gui_log.error("Can not show image. originalBuffer is null (addr=%d, memsize=%d)", addr, memsize);
		return;
	}

	const auto convertedBuffer = new (std::nothrow) u8[memsize / texel_bytes * u64{4}];

	if (!convertedBuffer)
	{
		// OOM or invalid memory address, give up
		gui_log.error("Can not show image. convertedBuffer is null (addr=%d, memsize=%d)", addr, memsize);
		return;
	}

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
			for (u32 x = 0, x_new = 0; x < pitch && x_new < pitch_new; x += 3, x_new += 4)
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
	case color_format::G8:
	{
		const u32 pitch = width * 1;
		const u32 pitch_new = width * 4;
		for (u32 y = 0; y < height; y++)
		{
			const u32 offset = y * pitch;
			const u32 offset_new = y * pitch_new;
			for (u32 x = 0; x < pitch; x++)
			{
				const u8 color = originalBuffer[offset + x];
				convertedBuffer[offset_new + x * 4 + 0] = color;
				convertedBuffer[offset_new + x * 4 + 1] = color;
				convertedBuffer[offset_new + x * 4 + 2] = color;
				convertedBuffer[offset_new + x * 4 + 3] = 255;
			}
		}
		break;
	}
	case color_format::G32MAX:
	{
		// Special: whitens as 4-byte groups tend to have a higher value, in order to perceive memory contents
		// May be used to search for instructions or floats for example

		const u32 pitch = width * 4;

		for (u32 y = 0; y < height; y++)
		{
			const u32 offset = y * pitch;
			for (u32 x = 0; x < pitch; x += 4)
			{
				const u8 color = std::max({originalBuffer[offset + x + 0], originalBuffer[offset + x + 1], originalBuffer[offset + x + 2], originalBuffer[offset + x + 3]});
				convertedBuffer[offset + x + 0] = color;
				convertedBuffer[offset + x + 1] = color;
				convertedBuffer[offset + x + 2] = color;
				convertedBuffer[offset + x + 3] = 255;
			}
		}

		break;
	}
	}

	// Flip vertically
	if (flipv && width > 0 && height > 1 && memsize > 1)
	{
		const u32 pitch = width * 4;
		std::vector<u8> tmp_row(pitch);

		for (u32 y = 0; y < height / 2; y++)
		{
			u8* row_top = &convertedBuffer[y * pitch];
			u8* row_bottom = &convertedBuffer[(height - y - 1) * pitch];

			// Swap rows
			std::memcpy(tmp_row.data(), row_top, pitch);
			std::memcpy(row_top, row_bottom, pitch);
			std::memcpy(row_bottom, tmp_row.data(), pitch);
		}
	}

	std::unique_ptr<QImage> image = std::make_unique<QImage>(convertedBuffer, width, height, QImage::Format_ARGB32, [](void* buffer){ delete[] static_cast<u8*>(buffer); }, convertedBuffer);
	if (image->isNull()) return;

	QLabel* canvas = new QLabel();
	canvas->setFixedSize(width, height);
	canvas->setAttribute(Qt::WA_Hover);
	canvas->setPixmap(QPixmap::fromImage(*image));

	QLabel* image_title = new QLabel();

	QVBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(image_title);
	layout->addWidget(canvas);

	struct image_viewer : public QDialog
	{
		QLabel* const m_canvas;
		QLabel* const m_image_title;
		const std::unique_ptr<QImage> m_image;
		const u32 m_addr;
		const int m_addr_scale = 1;
		const u32 m_pitch;
		const u32 m_width;
		const u32 m_height;
		int m_canvas_scale = 1;

		image_viewer(QWidget* parent, QLabel* canvas, QLabel* image_title, std::unique_ptr<QImage> image, u32 addr, u32 addr_scale, u32 pitch, u32 width, u32 height) noexcept
			: QDialog(parent)
			, m_canvas(canvas)
			, m_image_title(image_title)
			, m_image(std::move(image))
			, m_addr(addr)
			, m_addr_scale(addr_scale)
			, m_pitch(pitch)
			, m_width(width)
			, m_height(height)
		{
		}

		bool eventFilter(QObject* object, QEvent* event) override
		{
			if (object == m_canvas && (event->type() == QEvent::HoverMove || event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave))
			{
				const QPointF xy = static_cast<QHoverEvent*>(event)->position() / m_canvas_scale;
				set_window_name_by_coordinates(xy.x(), xy.y());
				return false;
			}

			if (object == m_canvas && event->type() == QEvent::MouseButtonDblClick && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
			{
				QLineEdit* addr_line = static_cast<memory_viewer_panel*>(parent())->m_addr_line;

				const QPointF xy = static_cast<QMouseEvent*>(event)->position() / m_canvas_scale;
				addr_line->setText(QString::fromStdString(fmt::format("%08x", get_pointed_addr(xy.x(), xy.y()))));
				Q_EMIT addr_line->returnPressed();
				close();
				return false;
			}

			return QDialog::eventFilter(object, event);
		}

		u32 get_pointed_addr(u32 x, u32 y) const
		{
			return m_addr + m_addr_scale * (y * m_pitch + x) / m_canvas_scale;
		}

		void set_window_name_by_coordinates(int x, int y)
		{
			if (x < 0 || y < 0)
			{
				m_image_title->setText(QString::fromStdString(fmt::format("[-, -]: NA")));
				return;
			}

			m_image_title->setText(QString::fromStdString(fmt::format("[x:%d, y:%d]: 0x%x", x, y, get_pointed_addr(x, y))));
		}

		void keyPressEvent(QKeyEvent* event) override
		{
			if (!isActiveWindow())
			{
				QDialog::keyPressEvent(event);
				return;
			}

			if (event->modifiers() == Qt::ControlModifier)
			{
				switch (const auto key = event->key())
				{
				case Qt::Key_Equal: // Also plus
				case Qt::Key_Plus:
				case Qt::Key_Minus:
				{
					m_canvas_scale = std::clamp(m_canvas_scale + (key == Qt::Key_Minus ? -1 : 1), 1, 5);

					const QSize fixed_size(m_width * m_canvas_scale, m_height * m_canvas_scale);

					// Fast transformation makes it not blurry, does not use bilinear filtering
					m_canvas->setPixmap(QPixmap::fromImage(m_image->scaled(fixed_size.width(), fixed_size.height(), Qt::KeepAspectRatio, Qt::FastTransformation)));

					m_canvas->setFixedSize(fixed_size);

					QTimer::singleShot(0, this, [this]()
					{
						// sizeHint() evaluates properly after events have been processed
						setFixedSize(sizeHint());
					});

					break;
				}
				}
			}

			QDialog::keyPressEvent(event);
		}
	};

	image_viewer* f_image_viewer = new image_viewer(parent, canvas, image_title, std::move(image), addr, texel_bytes, width, width, height);
	canvas->installEventFilter(f_image_viewer);
	f_image_viewer->setWindowTitle(QString::fromStdString(fmt::format("Raw Image @ 0x%x", addr)));
	f_image_viewer->setLayout(layout);
	f_image_viewer->setAttribute(Qt::WA_DeleteOnClose);
	f_image_viewer->show();

	QTimer::singleShot(0, f_image_viewer, [f_image_viewer]()
	{
		// sizeHint() evaluates properly after events have been processed
		f_image_viewer->setFixedSize(f_image_viewer->sizeHint());
	});
}

void memory_viewer_panel::ShowAtPC(u32 pc, std::function<cpu_thread*()> func)
{
	if (Emu.IsStopped())
		return;

	cpu_thread* cpu = func ? func() : nullptr;
	thread_class type = cpu ? cpu->get_class() : thread_class::ppu;

	if (type == thread_class::spu)
	{
		idm::make<memory_viewer_handle>(nullptr, nullptr, pc, std::move(func));
		return;
	}

	if (const auto* fxo = g_fxo->try_get<memory_viewer_fxo>())
	{
		auto it = fxo->last_opened.find(type);

		if (it != fxo->last_opened.end())
		{
			memory_viewer_panel* panel = it->second;

			if (panel)
			{
				panel->SetPC(pc);
				panel->scroll(0);

				if (!panel->isVisible())
					panel->show();
				
				panel->raise();

				return;
			}
		}
	}

	idm::make<memory_viewer_handle>(nullptr, nullptr, pc, std::move(func));
}
