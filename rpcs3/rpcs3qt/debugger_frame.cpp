#include "debugger_frame.h"
#include "register_editor_dialog.h"
#include "instruction_editor_dialog.h"
#include "gui_settings.h"
#include "debugger_list.h"
#include "breakpoint_list.h"
#include "breakpoint_handler.h"
#include "qt_utils.h"

#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"

#include <QKeyEvent>
#include <QScrollBar>
#include <QApplication>
#include <QFontDatabase>
#include <QCompleter>
#include <QMenu>
#include <QVBoxLayout>
#include <QTimer>
#include <atomic>

constexpr auto qstr = QString::fromStdString;
extern std::atomic<bool> user_asked_for_frame_capture;

debugger_frame::debugger_frame(std::shared_ptr<gui_settings> settings, QWidget *parent)
	: custom_dock_widget(tr("Debugger"), parent), xgui_settings(settings)
{
	setContentsMargins(0, 0, 0, 0);

	m_update = new QTimer(this);
	connect(m_update, &QTimer::timeout, this, &debugger_frame::UpdateUI);
	EnableUpdateTimer(true);

	m_mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	m_mono.setPointSize(10);

	QVBoxLayout* vbox_p_main = new QVBoxLayout();
	vbox_p_main->setContentsMargins(5, 5, 5, 5);

	QHBoxLayout* hbox_b_main = new QHBoxLayout();
	hbox_b_main->setContentsMargins(0, 0, 0, 0);

	m_breakpoint_handler = new breakpoint_handler();
	m_debugger_list = new debugger_list(this, settings, m_breakpoint_handler);
	m_debugger_list->installEventFilter(this);

	m_breakpoint_list = new breakpoint_list(this, m_breakpoint_handler);

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

	m_go_to_addr = new QPushButton(tr("Go To Address"), this);
	m_go_to_pc = new QPushButton(tr("Go To PC"), this);
	m_btn_capture = new QPushButton(tr("RSX Capture"), this);
	m_btn_step = new QPushButton(tr("Step"), this);
	m_btn_step_over = new QPushButton(tr("Step Over"), this);
	m_btn_run = new QPushButton(RunString, this);

	EnableButtons(false);

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

	m_debugger_list->setFont(m_mono);
	m_regs->setFont(m_mono);

	m_right_splitter = new QSplitter(this);
	m_right_splitter->setOrientation(Qt::Vertical);
	m_right_splitter->addWidget(m_regs);
	m_right_splitter->addWidget(m_breakpoint_list);
	m_right_splitter->setStretchFactor(0, 1);

	m_splitter = new QSplitter(this);
	m_splitter->addWidget(m_debugger_list);
	m_splitter->addWidget(m_right_splitter);

	QHBoxLayout* hbox_w_list = new QHBoxLayout();
	hbox_w_list->addWidget(m_splitter);

	vbox_p_main->addLayout(hbox_b_main);
	vbox_p_main->addLayout(hbox_w_list);

	QWidget* body = new QWidget(this);
	body->setLayout(vbox_p_main);
	setWidget(body);

	connect(m_go_to_addr, &QAbstractButton::clicked, this, &debugger_frame::ShowGotoAddressDialog);
	connect(m_go_to_pc, &QAbstractButton::clicked, this, &debugger_frame::ShowPC);

	connect(m_btn_capture, &QAbstractButton::clicked, [this]()
	{
		user_asked_for_frame_capture = true;
	});

	connect(m_btn_step, &QAbstractButton::clicked, this, &debugger_frame::DoStep);
	connect(m_btn_step_over, &QAbstractButton::clicked, [this]() { DoStep(true); });

	connect(m_btn_run, &QAbstractButton::clicked, [this]()
	{
		if (const auto cpu = this->cpu.lock())
		{
			if (m_btn_run->text() == RunString && cpu->state.test_and_reset(cpu_flag::dbg_pause))
			{
				if (!(cpu->state & (cpu_flag::dbg_pause + cpu_flag::dbg_global_pause)))
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

	connect(m_debugger_list, &debugger_list::BreakpointRequested, m_breakpoint_list, &breakpoint_list::HandleBreakpointRequest);
	connect(m_breakpoint_list, &breakpoint_list::RequestShowAddress, m_debugger_list, &debugger_list::ShowAddress);

	m_debugger_list->ShowAddress(m_debugger_list->m_pc);
	UpdateUnitList();
}

void debugger_frame::SaveSettings()
{
	xgui_settings->SetValue(gui::d_splitterState, m_splitter->saveState());
}

void debugger_frame::ChangeColors()
{
	if (m_debugger_list)
	{
		m_debugger_list->m_color_bp = m_breakpoint_list->m_color_bp = gui::utils::get_label_color("debugger_frame_breakpoint", QPalette::Window);
		m_debugger_list->m_color_pc = gui::utils::get_label_color("debugger_frame_pc", QPalette::Window);
		m_debugger_list->m_text_color_bp = m_breakpoint_list->m_text_color_bp = gui::utils::get_label_color("debugger_frame_breakpoint");;
		m_debugger_list->m_text_color_pc = gui::utils::get_label_color("debugger_frame_pc");;
	}
}

bool debugger_frame::eventFilter(QObject* object, QEvent* ev)
{
	// There's no overlap between keys so returning true wouldn't matter.
	if (object == m_debugger_list && ev->type() == QEvent::KeyPress)
	{
		keyPressEvent(static_cast<QKeyEvent*>(ev));
	}
	return false;
}

void debugger_frame::closeEvent(QCloseEvent *event)
{
	QDockWidget::closeEvent(event);
	Q_EMIT DebugFrameClosed();
}

void debugger_frame::showEvent(QShowEvent * event)
{
	// resize splitter widgets
	if (!m_splitter->restoreState(xgui_settings->GetValue(gui::d_splitterState).toByteArray()))
	{
		const int width_right = width() / 3;
		const int width_left = width() - width_right;
		m_splitter->setSizes({width_left, width_right});
	}

	QDockWidget::showEvent(event);
}

void debugger_frame::hideEvent(QHideEvent * event)
{
	// save splitter state or it will resume its initial state on next show
	xgui_settings->SetValue(gui::d_splitterState, m_splitter->saveState());
	QDockWidget::hideEvent(event);
}

void debugger_frame::keyPressEvent(QKeyEvent* event)
{
	const auto cpu = this->cpu.lock();
	int i = m_debugger_list->currentRow();

	if (!isActiveWindow() || i < 0 || !cpu || m_no_thread_selected)
	{
		return;
	}

	const u32 start_pc = m_debugger_list->m_pc - m_debugger_list->m_item_count * 4;
	const u32 pc = start_pc + i * 4;

	if (QApplication::keyboardModifiers() & Qt::ControlModifier)
	{
		switch (event->key())
		{
		case Qt::Key_G:
		{
			ShowGotoAddressDialog();
			return;
		}
		}
	}
	else
	{
		switch (event->key())
		{
		case Qt::Key_E:
		{
			instruction_editor_dialog* dlg = new instruction_editor_dialog(this, pc, cpu, m_disasm.get());
			dlg->show();
			return;
		}
		case Qt::Key_R:
		{
			register_editor_dialog* dlg = new register_editor_dialog(this, pc, cpu, m_disasm.get());
			dlg->show();
			return;
		}

		case Qt::Key_F10:
		{
			DoStep(true);
			return;
		}
		case Qt::Key_F11:
		{
			DoStep(false);
			return;
		}
		}
	}
}

u32 debugger_frame::GetPc() const
{
	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		return 0;
	}

	return cpu->id_type() == 1 ? static_cast<ppu_thread*>(cpu.get())->cia : static_cast<spu_thread*>(cpu.get())->pc;
}

void debugger_frame::UpdateUI()
{
	UpdateUnitList();

	m_btn_capture->setEnabled(Emu.IsRunning() || Emu.IsPaused());

	if (m_no_thread_selected) return;

	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		if (m_last_pc != umax || m_last_stat)
		{
			m_last_pc = -1;
			m_last_stat = 0;
			DoUpdate();
		}

		m_btn_run->setEnabled(false);
		m_btn_step->setEnabled(false);
		m_btn_step_over->setEnabled(false);
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

			if (state & cpu_flag::dbg_pause)
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
		QVariant var_cpu = QVariant::fromValue<void*>(&cpu);
		m_choice_units->addItem(qstr(cpu.get_name()), var_cpu);
		if (old_cpu == var_cpu) m_choice_units->setCurrentIndex(m_choice_units->count() - 1);
	};

	{
		const QSignalBlocker blocker(m_choice_units);

		idm::select<named_thread<ppu_thread>>(on_select);
		idm::select<named_thread<spu_thread>>(on_select);
	}

	OnSelectUnit();

	m_choice_units->update();
}

void debugger_frame::OnSelectUnit()
{
	if (m_choice_units->count() < 1 || m_current_choice == m_choice_units->currentText()) return;

	m_current_choice = m_choice_units->currentText();
	m_no_thread_selected = m_current_choice == NoThreadString;
	m_debugger_list->m_no_thread_selected = m_no_thread_selected;

	m_disasm.reset();
	cpu.reset();

	if (!m_no_thread_selected)
	{
		const auto on_select = [&](u32, cpu_thread& cpu)
		{
			cpu_thread* data = static_cast<cpu_thread*>(m_choice_units->currentData().value<void*>());
			return data == &cpu;
		};

		if (auto ppu = idm::select<named_thread<ppu_thread>>(on_select))
		{
			m_disasm = std::make_unique<PPUDisAsm>(CPUDisAsm_InterpreterMode);
			cpu = ppu.ptr;
		}
		else if (auto spu1 = idm::select<named_thread<spu_thread>>(on_select))
		{
			m_disasm = std::make_unique<SPUDisAsm>(CPUDisAsm_InterpreterMode);
			cpu = spu1.ptr;
		}
	}

	EnableButtons(!m_no_thread_selected);

	m_debugger_list->UpdateCPUData(this->cpu, m_disasm);
	m_breakpoint_list->UpdateCPUData(this->cpu, m_disasm);
	DoUpdate();
	UpdateUI();
}

void debugger_frame::DoUpdate()
{
	// Check if we need to disable a step over bp
	if (m_last_step_over_breakpoint != umax && GetPc() == m_last_step_over_breakpoint)
	{
		m_breakpoint_handler->RemoveBreakpoint(m_last_step_over_breakpoint);
		m_last_step_over_breakpoint = -1;
	}

	ShowPC();
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
		address_preview_label->setText(QString("Address: 0x%1").arg(pc, 8, 16, QChar('0')));
		expression_input->setPlaceholderText(QString("0x%1").arg(pc, 8, 16, QChar('0')));
	}
	else
	{
		expression_input->setPlaceholderText("0x00000000");
		address_preview_label->setText("Address: 0x00000000");
	}

	auto l_changeLabel = [&](const QString& text)
	{
		if (text.isEmpty())
		{
			address_preview_label->setText("Address: " + expression_input->placeholderText());
		}
		else
		{
			ulong ul_addr = EvaluateExpression(text);
			address_preview_label->setText(QString("Address: 0x%1").arg(ul_addr, 8, 16, QChar('0')));
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

		m_debugger_list->ShowAddress(address);
	}

	diag->deleteLater();
}

u64 debugger_frame::EvaluateExpression(const QString& expression)
{
	auto thread = cpu.lock();

	if (!thread) return 0;

	// Parse expression(or at least used to, was nuked to remove the need for QtJsEngine)
	const QString fixed_expression = QRegExp("^[A-Fa-f0-9]+$").exactMatch(expression) ? "0x" + expression : expression;
	return static_cast<ulong>(fixed_expression.toULong(nullptr, 0));
}

void debugger_frame::ClearBreakpoints()
{
	m_breakpoint_list->ClearBreakpoints();
}

void debugger_frame::ShowPC()
{
	m_debugger_list->ShowAddress(GetPc());
}

void debugger_frame::DoStep(bool stepOver)
{
	if (const auto cpu = this->cpu.lock())
	{
		bool should_step_over = stepOver && cpu->id_type() == 1;

		if (+cpu_flag::dbg_pause & +cpu->state.fetch_op([&](bs_t<cpu_flag>& state)
		{
			if (!should_step_over)
				state += cpu_flag::dbg_step;

			state -= cpu_flag::dbg_pause;
		}))
		{
			if (should_step_over)
			{
				u32 current_instruction_pc = GetPc();

				// Set breakpoint on next instruction
				u32 next_instruction_pc = current_instruction_pc + 4;
				m_breakpoint_handler->AddBreakpoint(next_instruction_pc);

				// Undefine previous step over breakpoint if it hasnt been already
				// This can happen when the user steps over a branch that doesn't return to itself
				if (m_last_step_over_breakpoint != umax)
				{
					m_breakpoint_handler->RemoveBreakpoint(next_instruction_pc);
				}

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
	if (m_no_thread_selected) enable = false;

	m_go_to_addr->setEnabled(enable);
	m_go_to_pc->setEnabled(enable);
	m_btn_step->setEnabled(enable);
	m_btn_step_over->setEnabled(enable);
	m_btn_run->setEnabled(enable);
}
