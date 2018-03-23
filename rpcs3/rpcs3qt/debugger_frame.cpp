#include "debugger_frame.h"
#include "qt_utils.h"

#include <QScrollBar>
#include <QApplication>
#include <QFontDatabase>
#include <QCompleter>
#include <QMenu>
#include <QJSEngine>

constexpr auto qstr = QString::fromStdString;
extern bool user_asked_for_frame_capture;

debugger_frame::debugger_frame(std::shared_ptr<gui_settings> settings, QWidget *parent)
	: custom_dock_widget(tr("Debugger"), parent), xgui_settings(settings)
{
	m_update = new QTimer(this);
	connect(m_update, &QTimer::timeout, this, &debugger_frame::UpdateUI);
	EnableUpdateTimer(true);

	m_mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	m_mono.setPointSize(10);

	QVBoxLayout* vbox_p_main = new QVBoxLayout();
	QHBoxLayout* hbox_b_main = new QHBoxLayout();

	m_list = new debugger_list(this, settings);
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
	m_btn_step_over = new QPushButton(tr("Step Over"), this);
	m_btn_run = new QPushButton(RunString, this);

	EnableButtons(!Emu.IsStopped());

	ChangeColors();

	hbox_b_main->addWidget(m_go_to_addr);
	hbox_b_main->addWidget(m_go_to_pc);
	hbox_b_main->addWidget(m_btn_capture);
	hbox_b_main->addWidget(m_btn_step);
	hbox_b_main->addWidget(m_btn_step_over);
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

	connect(m_go_to_addr, &QAbstractButton::clicked, this, &debugger_frame::ShowGotoAddressDialog);
	connect(m_go_to_pc, &QAbstractButton::clicked, this, &debugger_frame::Show_PC);

	connect(m_btn_capture, &QAbstractButton::clicked, [=]()
	{
		user_asked_for_frame_capture = true;
	});

	connect(m_btn_step, &QAbstractButton::clicked, this, &debugger_frame::DoStep);
	connect(m_btn_step_over, &QAbstractButton::clicked, [=]() { DoStep(true); });

	connect(m_btn_run, &QAbstractButton::clicked, [=]()
	{
		if (const auto cpu = this->cpu.lock())
		{
			if (m_btn_run->text() == RunString && cpu->state.test_and_reset(cpu_flag::dbg_pause))
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

	connect(m_breakpoints_list, &QListWidget::itemDoubleClicked, this, &debugger_frame::OnBreakpointList_doubleClicked);
	connect(m_breakpoints_list, &QListWidget::customContextMenuRequested, this, &debugger_frame::OnBreakpointList_rightClicked);
	connect(m_breakpoints_list_delete, &QAction::triggered, this, &debugger_frame::OnBreakpointList_delete);

	m_list->ShowAddress(m_list->m_pc);
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
		m_list->m_color_bp = gui::utils::get_label_color("debugger_frame_breakpoint", QPalette::Background);
		m_list->m_color_pc = gui::utils::get_label_color("debugger_frame_pc", QPalette::Background);
		m_list->m_text_color_bp = gui::utils::get_label_color("debugger_frame_breakpoint");;
		m_list->m_text_color_pc = gui::utils::get_label_color("debugger_frame_pc");;
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
		m_splitter->setSizes({width_left, width_right});
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

	return cpu->id_type() == 1 ? static_cast<ppu_thread*>(cpu.get())->cia : static_cast<SPUThread*>(cpu.get())->pc;
}

void debugger_frame::UpdateUI()
{
	UpdateUnitList();

	if (m_no_thread_selected) return;

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
			m_btn_step_over->setEnabled(false);
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
				m_btn_run->setText(RunString);
				m_btn_step->setEnabled(true);
				m_btn_step_over->setEnabled(true);
			}
			else
			{
				m_btn_run->setText(PauseString);
				m_btn_step->setEnabled(false);
				m_btn_step_over->setEnabled(false);
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
	m_choice_units->addItem(NoThreadString);

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
	m_no_thread_selected = m_current_choice == NoThreadString;
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
	}

	DoUpdate();
}

void debugger_frame::DoUpdate()
{
	// Check if we need to disable a step over bp
	if (m_last_step_over_breakpoint != -1 && GetPc() == m_last_step_over_breakpoint)
	{
		ppu_breakpoint(m_last_step_over_breakpoint);
		m_last_step_over_breakpoint = -1;
	}

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

void debugger_frame::ShowGotoAddressDialog()
{
	QDialog* diag = new QDialog(this);
	diag->setWindowTitle(tr("Enter expression"));
	diag->setModal(true);

	// Panels
	QVBoxLayout* vbox_panel(new QVBoxLayout());
	QHBoxLayout* hbox_address_preview_panel(new QHBoxLayout());
	QHBoxLayout* hbox_expression_input_panel = new QHBoxLayout();
	QHBoxLayout* hbox_button_panel(new QHBoxLayout());

	// Address preview
	QLabel* address_preview_label(new QLabel(diag));
	address_preview_label->setFont(m_mono);

	// Address expression input
	QLineEdit* expression_input(new QLineEdit(diag));
	expression_input->setFont(m_mono);
	expression_input->setMaxLength(18);
	expression_input->setFixedWidth(190);

	// Ok/Cancel
	QPushButton* button_ok = new QPushButton(tr("Ok"));
	QPushButton* button_cancel = new QPushButton(tr("Cancel"));

	hbox_address_preview_panel->addWidget(address_preview_label);

	hbox_expression_input_panel->addWidget(expression_input);

	hbox_button_panel->addWidget(button_ok);
	hbox_button_panel->addWidget(button_cancel);

	vbox_panel->addLayout(hbox_address_preview_panel);
	vbox_panel->addSpacing(8);
	vbox_panel->addLayout(hbox_expression_input_panel);
	vbox_panel->addSpacing(8);
	vbox_panel->addLayout(hbox_button_panel);

	diag->setLayout(vbox_panel);

	const auto cpu = this->cpu.lock();

	if (cpu)
	{
		unsigned long pc = cpu ? GetPc() : 0x0;
		address_preview_label->setText("Address: " + QString("0x%1").arg(pc, 8, 16, QChar('0')));
		expression_input->setPlaceholderText(QString("0x%1").arg(pc, 8, 16, QChar('0')));
	}
	else
	{
		expression_input->setPlaceholderText("0x00000000");
		address_preview_label->setText("Address: 0x00000000");
	}

	auto l_changeLabel = [=]()
	{
		if (expression_input->text().isEmpty())
		{
			address_preview_label->setText("Address: " + expression_input->placeholderText());
		}
		else
		{
			ulong ul_addr = EvaluateExpression(expression_input->text());
			address_preview_label->setText("Address: " + QString("0x%1").arg(ul_addr, 8, 16, QChar('0')));
		}
	};

	connect(expression_input, &QLineEdit::textChanged, l_changeLabel);
	connect(button_ok, &QAbstractButton::clicked, diag, &QDialog::accept);
	connect(button_cancel, &QAbstractButton::clicked, diag, &QDialog::reject);

	diag->move(QCursor::pos());

	if (diag->exec() == QDialog::Accepted)
	{
		u32 address = cpu ? GetPc() : 0x0;

		if (expression_input->text().isEmpty())
		{
			address_preview_label->setText(expression_input->placeholderText());
		}
		else
		{
			address = EvaluateExpression(expression_input->text());
			address_preview_label->setText(expression_input->text());
		}

		m_list->ShowAddress(address);
	}

	diag->deleteLater();
}

u64 debugger_frame::EvaluateExpression(const QString& expression)
{
	auto thread = cpu.lock();

	// Parse expression
	QJSEngine scriptEngine;
	scriptEngine.globalObject().setProperty("pc", GetPc());

	if (thread->id_type() == 1)
	{
		auto ppu = static_cast<ppu_thread*>(thread.get());

		for (int i = 0; i < 32; ++i)
		{
			scriptEngine.globalObject().setProperty(QString("r%1hi").arg(i), QJSValue((u32)(ppu->gpr[i] >> 32)));
			scriptEngine.globalObject().setProperty(QString("r%1").arg(i), QJSValue((u32)(ppu->gpr[i])));
		}

		scriptEngine.globalObject().setProperty("lrhi", QJSValue((u32)(ppu->lr >> 32 )));
		scriptEngine.globalObject().setProperty("lr", QJSValue((u32)(ppu->lr)));
		scriptEngine.globalObject().setProperty("ctrhi", QJSValue((u32)(ppu->ctr >> 32)));
		scriptEngine.globalObject().setProperty("ctr", QJSValue((u32)(ppu->ctr)));
		scriptEngine.globalObject().setProperty("cia", QJSValue(ppu->cia));
	}
	else
	{
		auto spu = static_cast<SPUThread*>(thread.get());

		for (int i = 0; i < 128; ++i)
		{
			scriptEngine.globalObject().setProperty(QString("r%1hi").arg(i), QJSValue(spu->gpr[i]._u32[0]));
			scriptEngine.globalObject().setProperty(QString("r%1lo").arg(i), QJSValue(spu->gpr[i]._u32[1]));
			scriptEngine.globalObject().setProperty(QString("r%1hilo").arg(i), QJSValue(spu->gpr[i]._u32[2]));
			scriptEngine.globalObject().setProperty(QString("r%1hihi").arg(i), QJSValue(spu->gpr[i]._u32[3]));
		}
	}

	return static_cast<ulong>(scriptEngine.evaluate(expression).toNumber());
}

void debugger_frame::Show_PC()
{
	m_list->ShowAddress(GetPc());
}

void debugger_frame::DoStep(bool stepOver)
{
	if (const auto cpu = this->cpu.lock())
	{
		bool should_step_over = stepOver && cpu->id_type() == 1;

		if (test(cpu_flag::dbg_pause, cpu->state.fetch_op([&](bs_t<cpu_flag>& state)
		{
			if (!should_step_over)
				state += cpu_flag::dbg_step;

			state -= cpu_flag::dbg_pause;
		})))
		{
			if (should_step_over)
			{
				u32 current_instruction_pc = GetPc();

				// Set breakpoint on next instruction
				u32 next_instruction_pc = current_instruction_pc + 4;
				ppu_breakpoint(next_instruction_pc);

				// Undefine previous step over breakpoint if it hasnt been already
				// This can happen when the user steps over a branch that doesn't return to itself
				if (m_last_step_over_breakpoint != -1)
					ppu_breakpoint(m_last_step_over_breakpoint);

				m_last_step_over_breakpoint = next_instruction_pc;
			}

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
	m_btn_step_over->setEnabled(enable);
	m_btn_run->setEnabled(enable);
}

void debugger_frame::ClearBreakpoints()
{
	//This is required to actually delete the breakpoints, not just visually.
	for (std::map<u32, bool>::iterator it = g_breakpoints.begin(); it != g_breakpoints.end(); it++)
		m_list->RemoveBreakPoint(it->first, false);

	g_breakpoints.clear();
}

void debugger_frame::OnBreakpointList_doubleClicked()
{
	u32 address = m_breakpoints_list->currentItem()->data(Qt::UserRole).value<u32>();
	m_list->ShowAddress(address);
	m_list->setCurrentRow(16);
}

void debugger_frame::OnBreakpointList_rightClicked(const QPoint &pos)
{
	if (m_breakpoints_list->itemAt(pos) == NULL)
		return;

	QMenu* menu = new QMenu();

	if (m_breakpoints_list->selectedItems().count() == 1)
	{
		menu->addAction("Rename");
		menu->addSeparator();
	}

	menu->addAction(m_breakpoints_list_delete);

	QAction* selectedItem = menu->exec(QCursor::pos());
	if (selectedItem != NULL)
	{
		if (selectedItem->text() == "Rename")
		{
			QListWidgetItem* currentItem = m_breakpoints_list->selectedItems().at(0);

			currentItem->setFlags(currentItem->flags() | Qt::ItemIsEditable);
			m_breakpoints_list->editItem(currentItem);
		}
	}
}

void debugger_frame::OnBreakpointList_delete()
{
	int selectedCount = m_breakpoints_list->selectedItems().count();

	for (int i = selectedCount - 1; i >= 0; i--)
	{
		m_list->RemoveBreakPoint(m_breakpoints_list->item(i)->data(Qt::UserRole).value<u32>());
	}
}

debugger_list::debugger_list(debugger_frame* parent, std::shared_ptr<gui_settings> settings) : QListWidget(parent)
{
	m_pc = 0;
	m_item_count = 30;
	m_debugFrame = parent;
	m_center_shown_addresses = settings->GetValue(gui::d_centerPC).toBool();
};

u32 debugger_list::GetCenteredAddress(u32 address)
{
	return address - ((m_item_count / 2) * 4);
}

void debugger_list::ShowAddress(u32 addr)
{
	if (m_center_shown_addresses)
	{
		m_pc = GetCenteredAddress(addr);
	}
	else
	{
		m_pc = addr;
	}

	const auto cpu = m_debugFrame->cpu.lock();

	if (!cpu)
	{
		for (uint i = 0; i < m_item_count; ++i, m_pc += 4)
		{
			item(i)->setText(qstr(fmt::format("[%08x] illegal address", m_pc)));
		}
	}
	else
	{
		const bool is_spu = cpu->id_type() != 1;
		const u32 cpu_offset = is_spu ? static_cast<SPUThread&>(*cpu).offset : 0;
		const u32 address_limits = is_spu ? 0x3ffff : ~0;
		m_pc &= address_limits;
		m_debugFrame->m_disasm->offset = (u8*)vm::base(cpu_offset);
		for (uint i = 0, count = 4; i<m_item_count; ++i, m_pc = (m_pc + count) & address_limits)
		{
			if (!vm::check_addr(cpu_offset + m_pc, 4))
			{
				item(i)->setText((IsBreakPoint(m_pc) ? ">>> " : "    ") + qstr(fmt::format("[%08x] illegal address", m_pc)));
				count = 4;
				continue;
			}

			count = m_debugFrame->m_disasm->disasm(m_debugFrame->m_disasm->dump_pc = m_pc);

			item(i)->setText((IsBreakPoint(m_pc) ? ">>> " : "    ") + qstr(m_debugFrame->m_disasm->last_opcode));

			if (test(cpu->state & cpu_state_pause) && m_pc == m_debugFrame->GetPc())
			{
				item(i)->setTextColor(m_text_color_pc);
				item(i)->setBackgroundColor(m_color_pc);
			}
			else if (IsBreakPoint(m_pc))
			{
				item(i)->setTextColor(m_text_color_bp);
				item(i)->setBackgroundColor(m_color_bp);
			}
			else
			{
				item(i)->setTextColor(palette().color(foregroundRole()));
				item(i)->setBackgroundColor(palette().color(backgroundRole()));
			}
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

	const auto cpu = m_debugFrame->cpu.lock();
	const u32 cpu_offset = cpu->id_type() != 1 ? static_cast<SPUThread&>(*cpu).offset : 0;
	m_debugFrame->m_disasm->offset = (u8*)vm::base(cpu_offset);

	m_debugFrame->m_disasm->disasm(m_debugFrame->m_disasm->dump_pc = pc);

	QString breakpointItemText = qstr(m_debugFrame->m_disasm->last_opcode);

	breakpointItemText.remove(10, 13);

	QListWidgetItem* breakpointItem = new QListWidgetItem(breakpointItemText);
	breakpointItem->setTextColor(m_text_color_bp);
	breakpointItem->setBackgroundColor(m_color_bp);
	QVariant pcVariant;
	pcVariant.setValue(pc);
	breakpointItem->setData(Qt::UserRole, pcVariant);
	m_debugFrame->m_breakpoints_list->addItem(breakpointItem);
}

void debugger_list::RemoveBreakPoint(u32 pc, bool eraseFromMap)
{
	if (eraseFromMap)
		g_breakpoints.erase(pc);

	ppu_breakpoint(pc);

	int breakpointsListCount = m_debugFrame->m_breakpoints_list->count();

	for (int i = 0; i < breakpointsListCount; i++)
	{
		QListWidgetItem* currentItem = m_debugFrame->m_breakpoints_list->item(i);

		if (currentItem->data(Qt::UserRole).value<u32>() == pc)
		{
			delete m_debugFrame->m_breakpoints_list->takeItem(i);
			break;
		}
	}

	ShowAddress(m_pc - (m_item_count) * 4);

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

	if (QApplication::keyboardModifiers() & Qt::ControlModifier)
	{
		switch (event->key())
		{
		case Qt::Key_G:
			m_debugFrame->ShowGotoAddressDialog();
			return;
		}
	}
	else
	{
		switch (event->key())
		{
		case Qt::Key_PageUp:   ShowAddress(m_pc - (m_item_count * 2) * 4); return;
		case Qt::Key_PageDown: ShowAddress(m_pc); return;
		case Qt::Key_Up:       ShowAddress(m_pc - (m_item_count + 1) * 4); return;
		case Qt::Key_Down:     ShowAddress(m_pc - (m_item_count - 1) * 4); return;
		case Qt::Key_E:
		{
			instruction_editor_dialog* dlg = new instruction_editor_dialog(this, pc, cpu, m_debugFrame->m_disasm.get());
			dlg->show();
			return;
		}
		case Qt::Key_R:
		{
			register_editor_dialog* dlg = new register_editor_dialog(this, pc, cpu, m_debugFrame->m_disasm.get());
			dlg->show();
			return;
		}

		case Qt::Key_F10:
			m_debugFrame->DoStep(true);
			return;

		case Qt::Key_F11:
			m_debugFrame->DoStep(false);
			return;
		}
	}
}

void debugger_list::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && !Emu.IsStopped() && !m_no_thread_selected)
	{
		long i = currentRow();
		if (i < 0) return;

		const u32 start_pc = m_pc - m_item_count * 4;
		const u32 pc = start_pc + i * 4;

		if (IsBreakPoint(pc))
		{
			RemoveBreakPoint(pc);
		}
		else
		{
			const auto cpu = m_debugFrame->cpu.lock();

			if (cpu->id_type() == 1 && vm::check_addr(pc))
			{
				AddBreakPoint(pc);
			}
		}

		ShowAddress(start_pc);
	}
}

void debugger_list::wheelEvent(QWheelEvent* event)
{
	QPoint numSteps = event->angleDelta() / 8 / 15;	// http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
	const int value = numSteps.y();

	ShowAddress(m_pc - (event->modifiers() == Qt::ControlModifier ? m_item_count * (value + 1) : m_item_count + value) * 4);
}

void debugger_list::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);

	if (count() < 1 || visualItemRect(item(0)).height() < 1)
	{
		return;
	}

	m_item_count = (rect().height() - frameWidth() * 2) / visualItemRect(item(0)).height();

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

	ShowAddress(m_pc - m_item_count * 4);
}
