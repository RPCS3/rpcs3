#include "debugger_frame.h"

#include <QScrollBar>
#include <QApplication>
#include <QFontDatabase>
#include <QCompleter>
#include <QMenu>

constexpr auto qstr = QString::fromStdString;
extern bool user_asked_for_frame_capture;

debugger_frame::debugger_frame(std::shared_ptr<gui_settings> settings, QWidget *parent)
	: QDockWidget(tr("Debugger"), parent), xgui_settings(settings)
{
	m_update = new QTimer(this);
	connect(m_update, &QTimer::timeout, this, &debugger_frame::UpdateUI);
	EnableUpdateTimer(true);

	m_mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	m_mono.setPointSize(10);

	QVBoxLayout* vbox_p_main = new QVBoxLayout();
	QHBoxLayout* hbox_b_main = new QHBoxLayout();

	m_brkpt_handler = new breakpoint_handler();
	m_list = new debugger_list(this, m_brkpt_handler);

	connect(m_list, &debugger_list::RequestCPUStep, this, &debugger_frame::DoStep);
	connect(m_list, &debugger_list::BreakpointRequested, this, &debugger_frame::HandleBreakpointRequest);

	m_breakpoints_list = new QListWidget(this);
	m_breakpoints_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_breakpoints_list->setContextMenuPolicy(Qt::CustomContextMenu);
	m_breakpoints_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_choice_units = new QComboBox(this);
	m_choice_units->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_choice_units->setMaxVisibleItems(30);
	m_choice_units->setMaximumWidth(500);
	m_choice_units->setEditable(true);
	m_choice_units->setInsertPolicy(QComboBox::NoInsert);
	m_choice_units->lineEdit()->setPlaceholderText("Choose a thread");
	m_choice_units->completer()->setCompletionMode(QCompleter::PopupCompletion);
	m_choice_units->completer()->setMaxVisibleItems(30);
	m_choice_units->completer()->setFilterMode(Qt::MatchContains);

	m_breakpoints_list_delete = new QAction("Delete", m_breakpoints_list);
	m_breakpoints_list_delete->setShortcut(Qt::Key_Delete);
	m_breakpoints_list_delete->setShortcutContext(Qt::WidgetShortcut);
	m_breakpoints_list->addAction(m_breakpoints_list_delete);

	m_go_to_addr = new QPushButton(tr("Go To Address"), this);
	m_go_to_pc = new QPushButton(tr("Go To PC"), this);
	m_btn_capture = new QPushButton(tr("Capture"), this);
	m_btn_step = new QPushButton(tr("Step"), this);
	m_btn_run = new QPushButton(Run, this);

	EnableButtons(!Emu.IsStopped());
	ChangeColors();

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

	m_list->setFont(m_mono);
	m_regs->setFont(m_mono);

	m_right_splitter = new QSplitter(this);
	m_right_splitter->setOrientation(Qt::Vertical);
	m_right_splitter->addWidget(m_regs);
	m_right_splitter->addWidget(m_breakpoints_list);
	m_right_splitter->setStretchFactor(0, 1);

	m_splitter = new QSplitter(this);
	m_splitter->addWidget(m_list);
	m_splitter->addWidget(m_right_splitter);

	QHBoxLayout* hbox_w_list = new QHBoxLayout();
	hbox_w_list->addWidget(m_splitter);

	vbox_p_main->addLayout(hbox_b_main);
	vbox_p_main->addLayout(hbox_w_list);

	QWidget* body = new QWidget(this);
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

	connect(m_btn_capture, &QAbstractButton::clicked, [=]()
	{
		user_asked_for_frame_capture = true;
	});

	connect(m_btn_step, &QAbstractButton::clicked, this, &debugger_frame::DoStep);

	connect(m_btn_run, &QAbstractButton::clicked, [=]()
	{
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

	connect(m_choice_units->lineEdit(), &QLineEdit::editingFinished, [&]
	{
		m_choice_units->clearFocus();
	});

	connect(m_choice_units, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &debugger_frame::UpdateUI);
	connect(m_choice_units, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &debugger_frame::OnSelectUnit);
	connect(this, &QDockWidget::visibilityChanged, this, &debugger_frame::EnableUpdateTimer);

	connect(m_breakpoints_list, &QListWidget::itemDoubleClicked, this, &debugger_frame::OnBreakpointListDoubleClicked);
	connect(m_breakpoints_list, &QListWidget::customContextMenuRequested, this, &debugger_frame::OnBreakpointListRightClicked);
	connect(m_breakpoints_list_delete, &QAction::triggered, this, &debugger_frame::OnBreakpointListDelete);

	m_list->ShowAddr(CentrePc(m_list->m_pc));
	UpdateUnitList();
}

void debugger_frame::SaveSettings()
{
	xgui_settings->SetValue(gui::d_splitterState, m_splitter->saveState());
}

void debugger_frame::ChangeColors()
{
	if (m_list)
	{
		m_list->m_color_bp = gui::get_Label_Color("debugger_frame_breakpoint", QPalette::Background);
		m_list->m_color_pc = gui::get_Label_Color("debugger_frame_pc", QPalette::Background);
		m_list->m_text_color_bp = gui::get_Label_Color("debugger_frame_breakpoint");;
		m_list->m_text_color_pc = gui::get_Label_Color("debugger_frame_pc");;
	}
}

void debugger_frame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	Q_EMIT DebugFrameClosed();
}

void debugger_frame::showEvent(QShowEvent * event)
{
	// resize splitter widgets
	QByteArray state = xgui_settings->GetValue(gui::d_splitterState).toByteArray();

	if (state.isEmpty()) // resize 2:1
	{
		const int width_right = width() / 3;
		const int width_left = width() - width_right;
		m_splitter->setSizes({ width_left, width_right });
	}
	else
	{
		m_splitter->restoreState(state);
	}
	QDockWidget::showEvent(event);
}

void debugger_frame::hideEvent(QHideEvent * event)
{
	// save splitter state or it will resume its initial state on next show
	xgui_settings->SetValue(gui::d_splitterState, m_splitter->saveState());
	QDockWidget::hideEvent(event);
}

u32 debugger_frame::GetPc() const
{
	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		return 0;
	}

	return cpu->id_type() == 1 ? static_cast<ppu_thread*>(cpu.get())->cia : static_cast<SPUThread*>(cpu.get())->pc;
}

u32 debugger_frame::CentrePc(u32 pc) const
{
	return pc - ((m_list->m_item_count / 2) * 4);
}

void debugger_frame::UpdateUI()
{
	UpdateUnitList();

	if (m_no_thread_selected)
	{
		return;
	}

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
	m_no_thread_selected = m_current_choice == NoThread;
	m_list->m_no_thread_selected = m_no_thread_selected;

	m_disasm.reset();
	cpu.reset();

	if (!m_no_thread_selected)
	{
		const auto on_select = [&](u32, cpu_thread& cpu)
		{
			cpu_thread* data = (cpu_thread *)m_choice_units->currentData().value<void *>();
			return data == &cpu;
		};

		if (auto ppu = idm::select<ppu_thread>(on_select))
		{
			m_disasm = std::make_shared<PPUDisAsm>(CPUDisAsm_InterpreterMode);
			cpu = ppu.ptr;
		}
		else if (auto spu1 = idm::select<SPUThread>(on_select))
		{
			m_disasm = std::make_shared<SPUDisAsm>(CPUDisAsm_InterpreterMode);
			cpu = spu1.ptr;
		}
		else if (auto rspu = idm::select<RawSPUThread>(on_select))
		{
			m_disasm = std::make_shared<SPUDisAsm>(CPUDisAsm_InterpreterMode);
			cpu = rspu.ptr;
		}
	}

	m_list->UpdateCPUData(this->cpu, m_disasm);
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
	int loc = m_regs->verticalScrollBar()->value();
	m_regs->clear();
	m_regs->setText(qstr(cpu->dump()));
	m_regs->verticalScrollBar()->setValue(loc);
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
	p_pc->setFont(m_mono);
	p_pc->setMaxLength(8);
	p_pc->setFixedWidth(90);
	QLabel* addr(new QLabel(diag));
	addr->setFont(m_mono);

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

	diag->deleteLater();
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
	enable ? m_update->start(50) : m_update->stop();
}

void debugger_frame::EnableButtons(bool enable)
{
	m_go_to_addr->setEnabled(enable);
	m_go_to_pc->setEnabled(enable);
	m_btn_step->setEnabled(enable);
	m_btn_run->setEnabled(enable);
}

void debugger_frame::ClearBreakpoints()
{
	while (m_breakpoints_list->count())
	{
		auto* currentItem = m_breakpoints_list->takeItem(0);
		u32 loc = currentItem->data(Qt::UserRole).value<u32>();
		m_brkpt_handler->RemoveBreakpoint(loc);
		delete currentItem;
	}
}

void debugger_frame::RemoveBreakpoint(u32 pc)
{
	m_brkpt_handler->RemoveBreakpoint(pc);

	for (int i = 0; i < m_breakpoints_list->count(); i++)
	{
		QListWidgetItem* currentItem = m_breakpoints_list->item(i);

		if (currentItem->data(Qt::UserRole).value<u32>() == pc)
		{
			delete m_breakpoints_list->takeItem(i);
			break;
		}
	}
	
	// todo clean that up
	m_list->ShowAddr(m_list->m_pc - (m_list->m_item_count) * 4);
}

void debugger_frame::AddBreakpoint(u32 pc)
{
	m_brkpt_handler->AddBreakpoint(pc);

	const auto cpu = this->cpu.lock();
	const u32 cpu_offset = cpu->id_type() != 1 ? static_cast<SPUThread&>(*cpu).offset : 0;
	m_disasm->offset = (u8*)vm::base(cpu_offset);

	m_disasm->disasm(m_disasm->dump_pc = pc);

	QString breakpointItemText = qstr(m_disasm->last_opcode);

	breakpointItemText.remove(10, 13);

	QListWidgetItem* breakpointItem = new QListWidgetItem(breakpointItemText);
	breakpointItem->setTextColor(m_list->m_text_color_bp);
	breakpointItem->setBackgroundColor(m_list->m_color_bp);
	QVariant pcVariant;
	pcVariant.setValue(pc);
	breakpointItem->setData(Qt::UserRole, pcVariant);
	m_breakpoints_list->addItem(breakpointItem);
}

/**
* This helps separate debugger list from frame by allowing the frame to handle breakpoint request at some location.
* If breakpoint exists, we remove it, else add new one.
*/
void debugger_frame::HandleBreakpointRequest(u32 loc)
{
	if (m_brkpt_handler->HasBreakpoint(loc))
	{
		RemoveBreakpoint(loc);
	}
	else
	{
		const auto cpu = this->cpu.lock();

		if (cpu->id_type() == 1 && vm::check_addr(loc))
		{
			AddBreakpoint(loc);
		}
	}
}

void debugger_frame::OnBreakpointListDoubleClicked()
{
	m_list->ShowAddr(CentrePc(m_breakpoints_list->currentItem()->data(Qt::UserRole).value<u32>()));
	m_list->setCurrentRow(16);
}

void debugger_frame::OnBreakpointListRightClicked(const QPoint &pos)
{
	if (!m_breakpoints_list->itemAt(pos))
	{
		return;
	}

	QMenu* menu = new QMenu();

	if (m_breakpoints_list->selectedItems().count() == 1)
	{
		menu->addAction("Rename");
		menu->addSeparator();
	}
	menu->addAction(m_breakpoints_list_delete);

	QAction* selectedItem = menu->exec(QCursor::pos());
	if (selectedItem && selectedItem->text() == "Rename")
	{
		QListWidgetItem* currentItem = m_breakpoints_list->selectedItems().at(0);

		currentItem->setFlags(currentItem->flags() | Qt::ItemIsEditable);
		m_breakpoints_list->editItem(currentItem);
	}
}

void debugger_frame::OnBreakpointListDelete()
{
	int selectedCount = m_breakpoints_list->selectedItems().count();

	for (int i = selectedCount - 1; i >= 0; i--)
	{
		RemoveBreakpoint(m_breakpoints_list->item(i)->data(Qt::UserRole).value<u32>());
	}
}
