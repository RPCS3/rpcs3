#include "debugger_frame.h"

#include <QSplitter>
#include <QApplication>
#include <QFontDatabase>
#include <QCompleter>

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }
extern bool user_asked_for_frame_capture;

debugger_frame::debugger_frame(QWidget *parent) : QDockWidget(tr("Debugger"), parent)
{
	pSize = 10;

	update = new QTimer(this);
	connect(update, &QTimer::timeout, this, &debugger_frame::UpdateUI);
	EnableUpdateTimer(true);

	body = new QWidget(this);
	mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	mono.setPointSize(pSize);
	QFontMetrics* fontMetrics = new QFontMetrics(mono);

	QVBoxLayout* vbox_p_main = new QVBoxLayout();
	QHBoxLayout* hbox_b_main = new QHBoxLayout();

	m_list = new debugger_list(this);
	m_choice_units = new QComboBox(this);
	m_choice_units->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_choice_units->setMaxVisibleItems(30);
	m_choice_units->setMaximumWidth(500);
	m_choice_units->setEditable(true);
	m_choice_units->setInsertPolicy(QComboBox::NoInsert);
	m_choice_units->lineEdit()->setPlaceholderText("Choose a thread");
	connect(m_choice_units->lineEdit(), &QLineEdit::editingFinished, [&] {
		m_choice_units->clearFocus();
	});
	m_choice_units->completer()->setCompletionMode(QCompleter::PopupCompletion);
	m_choice_units->completer()->setMaxVisibleItems(30);
	m_choice_units->completer()->setFilterMode(Qt::MatchContains);

	m_go_to_addr = new QPushButton(tr("Go To Address"), this);
	m_go_to_pc = new QPushButton(tr("Go To PC"), this);
	m_btn_capture = new QPushButton(tr("Capture"), this);
	m_btn_step = new QPushButton(tr("Step"), this);
	m_btn_run = new QPushButton(Run, this);

	EnableButtons(!Emu.IsStopped());

	hbox_b_main->addWidget(m_go_to_addr);
	hbox_b_main->addWidget(m_go_to_pc);
	hbox_b_main->addWidget(m_btn_capture);
	hbox_b_main->addWidget(m_btn_step);
	hbox_b_main->addWidget(m_btn_run);
	hbox_b_main->addWidget(m_choice_units);
	hbox_b_main->addStretch();

	//Registers
	m_regs = new QTextEdit(this);
	m_regs->setLineWrapMode(QTextEdit::NoWrap);
	m_regs->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

	m_list->setFont(mono);
	m_regs->setFont(mono);

	QSplitter* splitter = new QSplitter(this);
	splitter->addWidget(m_list);
	splitter->addWidget(m_regs);

	QHBoxLayout* hbox_w_list = new QHBoxLayout();
	hbox_w_list->addWidget(splitter);

	vbox_p_main->addLayout(hbox_b_main);
	vbox_p_main->addLayout(hbox_w_list);

	body->setLayout(vbox_p_main);
	setWidget(body);
	
	m_list->setWindowTitle(tr("ASM"));
	for (uint i = 0; i < m_list->m_item_count; ++i)
	{
		m_list->insertItem(i, new QListWidgetItem(""));
	}
	m_list->setSizeAdjustPolicy(QListWidget::AdjustToContents);

	connect(m_go_to_addr, &QAbstractButton::clicked, this, &debugger_frame::Show_Val);
	connect(m_go_to_pc, &QAbstractButton::clicked, this, &debugger_frame::Show_PC);
	connect(m_btn_capture, &QAbstractButton::clicked, [=]() { user_asked_for_frame_capture = true; });
	connect(m_btn_step, &QAbstractButton::clicked, this, &debugger_frame::DoStep);
	connect(m_btn_run, &QAbstractButton::clicked, [=](){
		if (const auto cpu = this->cpu.lock())
		{
			if (m_btn_run->text() == Run && cpu->state.test_and_reset(cpu_flag::dbg_pause))
			{
				if (!test(cpu->state, cpu_flag::dbg_pause + cpu_flag::dbg_global_pause))
				{
					cpu->notify();
				}
			}
			else
			{
				cpu->state += cpu_flag::dbg_pause;
			}
		}
		UpdateUI();
	});
	connect(m_choice_units, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &debugger_frame::UpdateUI);
	connect(m_choice_units, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &debugger_frame::OnSelectUnit);
	connect(this, &QDockWidget::visibilityChanged, this, &debugger_frame::EnableUpdateTimer);

	m_list->ShowAddr(CentrePc(m_list->m_pc));
	UpdateUnitList();
}

void debugger_frame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	DebugFrameClosed();
}

#include <map>

std::map<u32, bool> g_breakpoints;

extern void ppu_breakpoint(u32 addr);

u32 debugger_frame::GetPc() const
{
	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		return 0;
	}

	switch (g_system)
	{
	case system_type::ps3: return cpu->id_type() == 1 ? static_cast<ppu_thread*>(cpu.get())->cia : static_cast<SPUThread*>(cpu.get())->pc;
	case system_type::psv: return static_cast<ARMv7Thread*>(cpu.get())->PC;
	}

	return 0xabadcafe;
}

u32 debugger_frame::CentrePc(u32 pc) const
{
	return pc/* - ((m_item_count / 2) * 4)*/;
}

void debugger_frame::UpdateUI()
{
	UpdateUnitList();

	if (m_noThreadSelected) return;

	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		if (m_last_pc != -1 || m_last_stat)
		{
			m_last_pc = -1;
			m_last_stat = 0;
			DoUpdate();

			m_btn_run->setEnabled(false);
			m_btn_step->setEnabled(false);
		}
	}
	else
	{
		const auto cia = GetPc();
		const auto state = cpu->state.load();

		if (m_last_pc != cia || m_last_stat != static_cast<u32>(state))
		{
			m_last_pc = cia;
			m_last_stat = static_cast<u32>(state);
			DoUpdate();

			if (test(state & cpu_flag::dbg_pause))
			{
				m_btn_run->setText(Run);
				m_btn_step->setEnabled(true);
			}
			else
			{
				m_btn_run->setText(Pause);
				m_btn_step->setEnabled(false);
			}
		}
	}

	if (Emu.IsStopped())
	{
		g_breakpoints.clear();
	}
}

void debugger_frame::UpdateUnitList()
{
	const u64 threads_created = cpu_thread::g_threads_created;
	const u64 threads_deleted = cpu_thread::g_threads_deleted;

	if (threads_created != m_threads_created || threads_deleted != m_threads_deleted)
	{
		m_threads_created = threads_created;
		m_threads_deleted = threads_deleted;
	}
	else
	{
		// Nothing to do
		return;
	}

	QVariant old_cpu = m_choice_units->currentData();

	m_choice_units->clear();
	m_choice_units->addItem(NoThread);

	const auto on_select = [&](u32, cpu_thread& cpu)
	{
		QVariant var_cpu = qVariantFromValue((void *)&cpu);
		m_choice_units->addItem(qstr(cpu.get_name()), var_cpu);
		if (old_cpu == var_cpu) m_choice_units->setCurrentIndex(m_choice_units->count() - 1);
	};

	{
		const QSignalBlocker blocker(m_choice_units);

		idm::select<ppu_thread>(on_select);
		idm::select<ARMv7Thread>(on_select);
		idm::select<RawSPUThread>(on_select);
		idm::select<SPUThread>(on_select);
	}

	OnSelectUnit();

	m_choice_units->update();
}

void debugger_frame::OnSelectUnit()
{
	if (m_choice_units->count() < 1 || m_current_choice == m_choice_units->currentText()) return;

	m_current_choice = m_choice_units->currentText();
	m_noThreadSelected = m_current_choice == NoThread;
	m_list->m_noThreadSelected = m_noThreadSelected;

	m_disasm.reset();
	cpu.reset();

	if (!m_noThreadSelected)
	{
		const auto on_select = [&](u32, cpu_thread& cpu)
		{
			cpu_thread* data = (cpu_thread *)m_choice_units->currentData().value<void *>();
			return data == &cpu;
		};

		if (auto ppu = idm::select<ppu_thread>(on_select))
		{
			m_disasm = std::make_unique<PPUDisAsm>(CPUDisAsm_InterpreterMode);
			cpu = ppu.ptr;
		}
		else if (auto spu1 = idm::select<SPUThread>(on_select))
		{
			m_disasm = std::make_unique<SPUDisAsm>(CPUDisAsm_InterpreterMode);
			cpu = spu1.ptr;
		}
		else if (auto rspu = idm::select<RawSPUThread>(on_select))
		{
			m_disasm = std::make_unique<SPUDisAsm>(CPUDisAsm_InterpreterMode);
			cpu = rspu.ptr;
		}
		else if (auto arm = idm::select<ARMv7Thread>(on_select))
		{
			m_disasm = std::make_unique<ARMv7DisAsm>(CPUDisAsm_InterpreterMode);
			cpu = arm.ptr;
		}
	}

	DoUpdate();
}

void debugger_frame::DoUpdate()
{
	Show_PC();
	WriteRegs();
}

void debugger_frame::WriteRegs()
{
	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		m_regs->clear();
		return;
	}

	m_regs->clear();
	m_regs->setText(qstr(cpu->dump()));
}

void debugger_frame::OnUpdate()
{
	//WriteRegs();
}

void debugger_frame::Show_Val()
{
	QDialog* diag = new QDialog(this);
	diag->setWindowTitle(tr("Set value"));
	diag->setModal(true);

	QPushButton* button_ok = new QPushButton(tr("Ok"));
	QPushButton* button_cancel = new QPushButton(tr("Cancel"));
	QVBoxLayout* vbox_panel(new QVBoxLayout());
	QHBoxLayout* hbox_text_panel(new QHBoxLayout());
	QHBoxLayout* hbox_button_panel(new QHBoxLayout());
	QLineEdit* p_pc(new QLineEdit(diag));
	p_pc->setFont(mono);
	p_pc->setMaxLength(8);
	p_pc->setFixedWidth(75);
	QLabel* addr(new QLabel(diag));
	addr->setFont(mono);
	
	hbox_text_panel->addWidget(addr);
	hbox_text_panel->addWidget(p_pc);

	hbox_button_panel->addWidget(button_ok);
	hbox_button_panel->addWidget(button_cancel);
	
	vbox_panel->addLayout(hbox_text_panel);
	vbox_panel->addSpacing(8);
	vbox_panel->addLayout(hbox_button_panel);
	
	diag->setLayout(vbox_panel);

	const auto cpu = this->cpu.lock();

	if (cpu) 
	{
		unsigned long pc = cpu ? GetPc() : 0x0;
		bool ok;
		addr->setText("Address: " + QString("%1").arg(pc, 8, 16, QChar('0')));	// set address input line to 8 digits
		p_pc->setPlaceholderText(QString("%1").arg(pc, 8, 16, QChar('0')));
	}
	else
	{
		p_pc->setPlaceholderText("00000000");
		addr->setText("Address: 00000000");
	}

	auto l_changeLabel = [=]()
	{
		if (p_pc->text().isEmpty())
		{
			addr->setText("Address: " + p_pc->placeholderText());
		}
		else
		{
			bool ok;
			ulong ul_addr = p_pc->text().toULong(&ok, 16);
			addr->setText("Address: " + QString("%1").arg(ul_addr, 8, 16, QChar('0'))); // set address input line to 8 digits
		}
	};

	connect(p_pc, &QLineEdit::textChanged, l_changeLabel);
	connect(button_ok, &QAbstractButton::clicked, diag, &QDialog::accept);
	connect(button_cancel, &QAbstractButton::clicked, diag, &QDialog::reject);

	diag->move(QCursor::pos());

	if (diag->exec() == QDialog::Accepted)
	{
		unsigned long pc = cpu ? GetPc() : 0x0;
		if (p_pc->text().isEmpty())
		{
			addr->setText(p_pc->placeholderText());
		}
		else
		{
			bool ok;
			pc = p_pc->text().toULong(&ok, 16);
			addr->setText(p_pc->text());
		}
		m_list->ShowAddr(CentrePc(pc));
	}
}

void debugger_frame::Show_PC()
{
	m_list->ShowAddr(CentrePc(GetPc()));
}

void debugger_frame::DoStep()
{
	if (const auto cpu = this->cpu.lock())
	{
		if (test(cpu_flag::dbg_pause, cpu->state.fetch_op([](bs_t<cpu_flag>& state)
		{
			state += cpu_flag::dbg_step;
			state -= cpu_flag::dbg_pause;
		})))
		{
			cpu->notify();
		}
	}
	UpdateUI();
}

void debugger_frame::EnableUpdateTimer(bool enable)
{
	enable ? update->start(50) : update->stop();
}

void debugger_frame::EnableButtons(bool enable)
{
	m_go_to_addr->setEnabled(enable);
	m_go_to_pc->setEnabled(enable);
	m_btn_step->setEnabled(enable);
	m_btn_run->setEnabled(enable);
}

debugger_list::debugger_list(debugger_frame* parent) : QListWidget(parent)
{
	m_pc = 0;
	m_item_count = 30;
	m_debugFrame = parent;
};

void debugger_list::ShowAddr(u32 addr)
{
	m_pc = addr;

	const auto cpu = m_debugFrame->cpu.lock();

	if (!cpu)
	{
		for (uint i = 0; i<m_item_count; ++i, m_pc += 4)
		{
			item(i)->setText(qstr(fmt::format("[%08x] illegal address", m_pc)));
		}
	}
	else
	{
		const u32 cpu_offset = g_system == system_type::ps3 && cpu->id_type() != 1 ? static_cast<SPUThread&>(*cpu).offset : 0;
		m_debugFrame->m_disasm->offset = (u8*)vm::base(cpu_offset);
		for (uint i = 0, count = 4; i<m_item_count; ++i, m_pc += count)
		{
			if (!vm::check_addr(cpu_offset + m_pc, 4))
			{
				item(i)->setText((IsBreakPoint(m_pc) ? ">>> " : "    ") + qstr(fmt::format("[%08x] illegal address", m_pc)));
				count = 4;
				continue;
			}

			count = m_debugFrame->m_disasm->disasm(m_debugFrame->m_disasm->dump_pc = m_pc);

			item(i)->setText((IsBreakPoint(m_pc) ? ">>> " : "    ") + qstr(m_debugFrame->m_disasm->last_opcode));

			QColor colour;
			
			if (test(cpu->state & cpu_state_pause) && m_pc == m_debugFrame->GetPc())
			{
				colour = QColor(Qt::green);
			}
			else
			{
				colour = QColor(IsBreakPoint(m_pc) ? Qt::yellow : Qt::white);
			}
			
			item(i)->setBackgroundColor(colour);
		}
	}

	setLineWidth(-1);
}

bool debugger_list::IsBreakPoint(u32 pc)
{
	return g_breakpoints.count(pc) != 0;
}

void debugger_list::AddBreakPoint(u32 pc)
{
	g_breakpoints.emplace(pc, false);
	ppu_breakpoint(pc);
}

void debugger_list::RemoveBreakPoint(u32 pc)
{
	g_breakpoints.erase(pc);
	ppu_breakpoint(pc);
}

void debugger_list::keyPressEvent(QKeyEvent* event)
{
	if (!isActiveWindow())
	{
		return;
	}

	const auto cpu = m_debugFrame->cpu.lock();
	long i = currentRow();

	if (i < 0 || !cpu)
	{
		return;
	}

	const u32 start_pc = m_pc - m_item_count * 4;
	const u32 pc = start_pc + i * 4;

	if (event->key() == Qt::Key_Space && QApplication::keyboardModifiers() & Qt::ControlModifier)
	{
		m_debugFrame->DoStep();
		return;
	}
	else
	{
		switch (event->key())
		{
		case Qt::Key_PageUp:   ShowAddr(m_pc - (m_item_count * 2) * 4); return;
		case Qt::Key_PageDown: ShowAddr(m_pc); return;
		case Qt::Key_Up:       ShowAddr(m_pc - (m_item_count + 1) * 4); return;
		case Qt::Key_Down:     ShowAddr(m_pc - (m_item_count - 1) * 4); return;
		case Qt::Key_E:
		{
			instruction_editor_dialog* dlg = new instruction_editor_dialog(this, pc, cpu, m_debugFrame->m_disasm.get());
			dlg->show();
			m_debugFrame->DoUpdate();
			return;
		}
		case Qt::Key_R:
		{
			register_editor_dialog* dlg = new register_editor_dialog(this, pc, cpu, m_debugFrame->m_disasm.get());
			dlg->show();
			m_debugFrame->DoUpdate();
			return;
		}
		}
	}
}

void debugger_list::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && !Emu.IsStopped() && !m_noThreadSelected)
	{
		long i = currentRow();
		if (i < 0) return;

		const u32 start_pc = m_pc - m_item_count * 4;
		const u32 pc = start_pc + i * 4;
		//ConLog.Write("pc=0x%llx", pc);

		if (IsBreakPoint(pc))
		{
			RemoveBreakPoint(pc);
		}
		else
		{
			const auto cpu = m_debugFrame->cpu.lock();

			if (g_system == system_type::ps3 && cpu->id_type() == 1 && vm::check_addr(pc))
			{
				AddBreakPoint(pc);
			}
		}

		ShowAddr(start_pc);
	}
}

void debugger_list::wheelEvent(QWheelEvent* event)
{
	QPoint numSteps = event->angleDelta() / 8 / 15;	// http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
	const int value = numSteps.y();

	ShowAddr(m_pc - (event->modifiers() == Qt::ControlModifier ? m_item_count * (value + 1) : m_item_count + value) * 4);
}

void debugger_list::resizeEvent(QResizeEvent* event)
{
	if (count() < 1 || visualItemRect(item(0)).height() < 1)
	{
		return;
	}

	m_item_count = (rect().height() - frameWidth()*2) / visualItemRect(item(0)).height();

	clear();

	for (u32 i = 0; i < m_item_count; ++i)
	{
		insertItem(i, new QListWidgetItem(""));
	}

	if (horizontalScrollBar())
	{
		m_item_count--;
		delete item(m_item_count);
	}

	ShowAddr(m_pc - m_item_count * 4);
}
