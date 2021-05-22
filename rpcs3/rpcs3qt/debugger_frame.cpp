#include "debugger_frame.h"
#include "register_editor_dialog.h"
#include "instruction_editor_dialog.h"
#include "memory_viewer_panel.h"
#include "gui_settings.h"
#include "debugger_list.h"
#include "breakpoint_list.h"
#include "breakpoint_handler.h"
#include "call_stack_list.h"
#include "input_dialog.h"
#include "qt_utils.h"

#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/RSXDisAsm.h"
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
#include <QVBoxLayout>
#include <QTimer>
#include <QCheckBox>
#include <charconv>

#include "util/asm.hpp"

constexpr auto qstr = QString::fromStdString;

constexpr auto s_pause_flags = cpu_flag::dbg_pause + cpu_flag::dbg_global_pause;

extern atomic_t<bool> g_debugger_pause_all_threads_on_bp;

debugger_frame::debugger_frame(std::shared_ptr<gui_settings> gui_settings, QWidget *parent)
	: custom_dock_widget(tr("Debugger"), parent)
	, m_gui_settings(std::move(gui_settings))
{
	setContentsMargins(0, 0, 0, 0);

	m_update = new QTimer(this);
	connect(m_update, &QTimer::timeout, this, &debugger_frame::UpdateUI);
	EnableUpdateTimer(true);

	m_mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	m_mono.setPointSize(9);

	QVBoxLayout* vbox_p_main = new QVBoxLayout();
	vbox_p_main->setContentsMargins(5, 5, 5, 5);

	QHBoxLayout* hbox_b_main = new QHBoxLayout();
	hbox_b_main->setContentsMargins(0, 0, 0, 0);

	m_breakpoint_handler = new breakpoint_handler();
	m_breakpoint_list = new breakpoint_list(this, m_breakpoint_handler);

	m_debugger_list = new debugger_list(this, m_gui_settings, m_breakpoint_handler);
	m_debugger_list->installEventFilter(this);

	m_call_stack_list = new call_stack_list(this);

	m_choice_units = new QComboBox(this);
	m_choice_units->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_choice_units->setMaxVisibleItems(30);
	m_choice_units->setMaximumWidth(500);
	m_choice_units->setEditable(true);
	m_choice_units->setInsertPolicy(QComboBox::NoInsert);
	m_choice_units->lineEdit()->setPlaceholderText(tr("Choose a thread"));
	m_choice_units->completer()->setCompletionMode(QCompleter::PopupCompletion);
	m_choice_units->completer()->setMaxVisibleItems(30);
	m_choice_units->completer()->setFilterMode(Qt::MatchContains);

	m_go_to_addr = new QPushButton(tr("Go To Address"), this);
	m_go_to_pc = new QPushButton(tr("Go To PC"), this);
	m_btn_step = new QPushButton(tr("Step"), this);
	m_btn_step_over = new QPushButton(tr("Step Over"), this);
	m_btn_run = new QPushButton(RunString, this);

	EnableButtons(false);

	ChangeColors();

	hbox_b_main->addWidget(m_go_to_addr);
	hbox_b_main->addWidget(m_go_to_pc);
	hbox_b_main->addWidget(m_btn_step);
	hbox_b_main->addWidget(m_btn_step_over);
	hbox_b_main->addWidget(m_btn_run);
	hbox_b_main->addWidget(m_choice_units);
	hbox_b_main->addStretch();

	// Misc state
	m_misc_state = new QTextEdit(this);
	m_misc_state->setLineWrapMode(QTextEdit::NoWrap);
	m_misc_state->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

	// Registers
	m_regs = new QTextEdit(this);
	m_regs->setLineWrapMode(QTextEdit::NoWrap);
	m_regs->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

	m_debugger_list->setFont(m_mono);
	m_misc_state->setFont(m_mono);
	m_regs->setFont(m_mono);
	m_call_stack_list->setFont(m_mono);

	m_right_splitter = new QSplitter(this);
	m_right_splitter->setOrientation(Qt::Vertical);
	m_right_splitter->addWidget(m_misc_state);
	m_right_splitter->addWidget(m_regs);
	m_right_splitter->addWidget(m_call_stack_list);
	m_right_splitter->addWidget(m_breakpoint_list);

	// Set relative sizes for widgets
	m_right_splitter->setStretchFactor(0, 2); // misc state
	m_right_splitter->setStretchFactor(1, 8); // registers
	m_right_splitter->setStretchFactor(2, 3); // call stack
	m_right_splitter->setStretchFactor(3, 1); // breakpoint list

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

	connect(m_btn_step, &QAbstractButton::clicked, this, &debugger_frame::DoStep);
	connect(m_btn_step_over, &QAbstractButton::clicked, [this]() { DoStep(true); });

	connect(m_btn_run, &QAbstractButton::clicked, [this]()
	{
		if (const auto cpu = get_cpu())
		{
			// If paused, unpause.
			// If not paused, add dbg_pause.
			const auto old = cpu->state.fetch_op([](bs_t<cpu_flag>& state)
			{
				if (state & s_pause_flags)
				{
					state -= s_pause_flags;
				}
				else
				{
					state += cpu_flag::dbg_pause;
				}
			});

			// Notify only if no pause flags are set after this change
			if (old & s_pause_flags)
			{
				if (g_debugger_pause_all_threads_on_bp && Emu.IsPaused() && (old & s_pause_flags) == s_pause_flags)
				{
					// Resume all threads were paused by this breakpoint
					Emu.Resume();
				}

				cpu->state.notify_one(s_pause_flags);
			}
		}
		UpdateUI();
	});

	connect(m_choice_units->lineEdit(), &QLineEdit::editingFinished, [&]
	{
		m_choice_units->clearFocus();
	});

	connect(m_choice_units, QOverload<int>::of(&QComboBox::activated), this, &debugger_frame::UpdateUI);
	connect(m_choice_units, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &debugger_frame::OnSelectUnit);
	connect(this, &QDockWidget::visibilityChanged, this, &debugger_frame::EnableUpdateTimer);

	connect(m_debugger_list, &debugger_list::BreakpointRequested, m_breakpoint_list, &breakpoint_list::HandleBreakpointRequest);
	connect(m_breakpoint_list, &breakpoint_list::RequestShowAddress, m_debugger_list, &debugger_list::ShowAddress);

	connect(this, &debugger_frame::CallStackUpdateRequested, m_call_stack_list, &call_stack_list::HandleUpdate);
	connect(m_call_stack_list, &call_stack_list::RequestShowAddress, m_debugger_list, &debugger_list::ShowAddress);

	m_debugger_list->ShowAddress(m_debugger_list->m_pc, false);
	UpdateUnitList();
}

void debugger_frame::SaveSettings() const
{
	m_gui_settings->SetValue(gui::d_splitterState, m_splitter->saveState());
}

void debugger_frame::ChangeColors() const
{
	if (m_debugger_list)
	{
		m_debugger_list->m_color_bp = m_breakpoint_list->m_color_bp = gui::utils::get_label_color("debugger_frame_breakpoint", QPalette::Window);
		m_debugger_list->m_color_pc = gui::utils::get_label_color("debugger_frame_pc", QPalette::Window);
		m_debugger_list->m_text_color_bp = m_breakpoint_list->m_text_color_bp = gui::utils::get_label_color("debugger_frame_breakpoint");
		m_debugger_list->m_text_color_pc = gui::utils::get_label_color("debugger_frame_pc");
	}
}

bool debugger_frame::eventFilter(QObject* object, QEvent* event)
{
	// There's no overlap between keys so returning true wouldn't matter.
	if (object == m_debugger_list && event->type() == QEvent::KeyPress)
	{
		keyPressEvent(static_cast<QKeyEvent*>(event));
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
	if (!m_splitter->restoreState(m_gui_settings->GetValue(gui::d_splitterState).toByteArray()))
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
	m_gui_settings->SetValue(gui::d_splitterState, m_splitter->saveState());
	QDockWidget::hideEvent(event);
}

void debugger_frame::open_breakpoints_settings()
{
	QDialog* dlg = new QDialog(this);
	dlg->setWindowTitle(tr("Breakpoint Settings"));
	dlg->setModal(true);

	QCheckBox* check_box = new QCheckBox(tr("Pause All Threads On Hit"), dlg);
	check_box->setCheckable(true);
	check_box->setChecked(g_debugger_pause_all_threads_on_bp.load());
	check_box->setToolTip(tr("When set: a breakpoint hit will pause the emulation instead of the current thread."
		"\nApplies on all breakpoints in all threads regardless if set before or after changing this setting."));

	connect(check_box, &QCheckBox::clicked, dlg, [](bool checked) { g_debugger_pause_all_threads_on_bp = checked; });

	QPushButton* button_ok = new QPushButton(tr("OK"), dlg);
	connect(button_ok, &QAbstractButton::clicked, dlg, &QDialog::accept);

	QHBoxLayout* hbox_layout = new QHBoxLayout(dlg);
	hbox_layout->addWidget(check_box);
	hbox_layout->addWidget(button_ok);
	dlg->setLayout(hbox_layout);
	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->exec();
}

void debugger_frame::keyPressEvent(QKeyEvent* event)
{
	if (!isActiveWindow())
	{
		return;
	}

	const auto cpu = get_cpu();
	const int row = m_debugger_list->currentRow();

	switch (event->key())
	{
	case Qt::Key_F1:
	{
		QDialog* dlg = new QDialog(this);
		dlg->setAttribute(Qt::WA_DeleteOnClose);
		dlg->setWindowTitle(tr("Debugger Guide & Shortcuts"));

		QLabel* l = new QLabel(tr(
			"Keys Ctrl+G: Go to typed address."
			"\nKeys Ctrl+B: Open breakpoints settings."
			"\nKeys Alt+S: Capture SPU images of selected SPU."
			"\nKey D: SPU MFC commands logger, MFC debug setting must be enabled."
			"\nKey E: Instruction Editor: click on the instruction you want to modify, then press E."
			"\nKey F: Dedicated floating point mode switch for SPU threads."
			"\nKey R: Registers Editor for selected thread."
			"\nKey N: Show next instruction the thread will execute after marked instruction, does nothing if target is not predictable."
			"\nKey M: Show the Memory Viewer with initial address pointing to the marked instruction."
			"\nKey I: Show RSX method detail."
			"\nKey F10: Perform step-over on instructions. (skip function calls)"
			"\nKey F11: Perform single-stepping on instructions."
			"\nKey F1: Show this help dialog."
			"\nKey Up: Scroll one instruction upwards. (address is decremented)"
			"\nKey Down: Scroll one instruction downwards. (address is incremented)"
			"\nKey Page-Up: Scroll upwards with steps count equal to the viewed instruction count."
			"\nKey Page-Down: Scroll downwards with steps count equal to the viewed instruction count."
			"\nDouble-click: Set breakpoints."));

		gui::utils::set_font_size(*l, 9);

		QVBoxLayout* layout = new QVBoxLayout();
		layout->addWidget(l);
		dlg->setLayout(layout);
		dlg->setFixedSize(dlg->sizeHint());
		dlg->move(QCursor::pos());
		dlg->exec();
		return;
	}
	default: break;
	}

	if (!cpu)
	{
		return;
	}

	const u32 address_limits = (cpu->id_type() == 2 ? 0x3fffc : ~3);
	const u32 pc = (row >= 0 ? m_debugger_list->m_pc + row * 4 : cpu->get_pc()) & address_limits;

	const auto modifiers = QApplication::keyboardModifiers();

	if (modifiers & Qt::ControlModifier)
	{
		switch (event->key())
		{
		case Qt::Key_G:
		{
			ShowGotoAddressDialog();
			return;
		}
		case Qt::Key_B:
		{
			open_breakpoints_settings();
			return;
		}
		default: break;
		}
	}
	else
	{
		switch (event->key())
		{
		case Qt::Key_D:
		{
			if (cpu->id_type() == 2 && g_cfg.core.mfc_debug && !cpu->is_stopped())
			{
				input_dialog dlg(4, "", tr("Max MFC cmds logged"),
					tr("Decimal only, max allowed is 1820."), "0", this);

				QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
				mono.setPointSize(8);
				dlg.set_input_font(mono, false);
				dlg.set_clear_button_enabled(false);
				dlg.set_button_enabled(QDialogButtonBox::StandardButton::Ok, false);
				dlg.set_validator(new QRegExpValidator(QRegExp("^[1-9][0-9]*$")));

				u32 max = 0;

				connect(&dlg, &input_dialog::text_changed, [&](const QString& changed)
				{
					bool ok = false;
					const u32 dummy = changed.toUInt(&ok, 10);
					ok = ok && dummy && dummy <= spu_thread::max_mfc_dump_idx;
					dlg.set_button_enabled(QDialogButtonBox::StandardButton::Ok, ok);

					if (ok)
					{
						max = dummy;
					}
				});

				if (dlg.exec() != QDialog::Accepted)
				{
					max = 0;
				}

				const auto spu = static_cast<spu_thread*>(cpu);

				const auto ptr = reinterpret_cast<const mfc_cmd_dump*>(vm::g_stat_addr + spu->vm_offset());

				std::string ret;

				for (u64 count = 0, idx = spu->mfc_dump_idx - 1; idx != umax && count < max; count++, idx--)
				{
					auto dump = ptr[idx % spu_thread::max_mfc_dump_idx];

					const u32 pc = std::exchange(dump.cmd.eah, 0);
					fmt::append(ret, "\n(%d) PC 0x%05x: [%s]", count, pc, dump.cmd);

					if (dump.cmd.cmd == MFC_PUTLLC_CMD)
					{
						fmt::append(ret, " %s", dump.cmd.tag == MFC_PUTLLC_SUCCESS ? "(passed)" : "(failed)");
					}

					const auto data = reinterpret_cast<const be_t<u32>*>(dump.data);

					for (usz i = 0; i < utils::aligned_div(std::min<u32>(dump.cmd.size, 128), 4); i += 4)
					{
						fmt::append(ret, "\n[0x%02x] %08x %08x %08x %08x", i * sizeof(data[0])
							, data[i + 0], data[i + 1], data[i + 2], data[i + 3]);
					}
				}

				if (ret.empty())
				{
					ret = "No MFC commands have been logged";
				}

				spu_log.warning("SPU MFC dump of '%s': %s", spu->get_name(), ret);
			}
			return;
		}
		case Qt::Key_E:
		{
			if (cpu->id_type() == 1 || cpu->id_type() == 2)
			{
				if (!m_inst_editor)
				{
					m_inst_editor = new instruction_editor_dialog(this, pc, m_disasm.get(), make_check_cpu(cpu));
					connect(m_inst_editor, &QDialog::finished, this, [this]() { m_inst_editor = nullptr; });
					m_inst_editor->show();
				}
			}
			return;
		}
		case Qt::Key_F:
		{
			if (cpu->id_type() != 2)
			{
				return;
			}

			static_cast<spu_thread*>(cpu)->debugger_float_mode ^= 1; // Switch mode
			return;
		}
		case Qt::Key_R:
		{
			if (cpu->id_type() == 1 || cpu->id_type() == 2)
			{
				if (!m_reg_editor)
				{
					m_reg_editor = new register_editor_dialog(this, m_disasm.get(), make_check_cpu(cpu));
					connect(m_reg_editor, &QDialog::finished, this, [this]() { m_reg_editor = nullptr; });
					m_reg_editor->show();
				}
			}
			return;
		}
		case Qt::Key_S:
		{
			if (modifiers & Qt::AltModifier)
			{
				if (cpu->id_type() != 2)
				{
					return;
				}

				static_cast<spu_thread*>(cpu)->capture_local_storage();
			}
			return;
		}
		case Qt::Key_N:
		{
			// Next instruction according to code flow
			// Known branch targets are selected over next PC for conditional branches
			// Indirect branches (unknown targets, such as function return) do not proceed to any instruction
			std::array<u32, 2> res{umax, umax};

			switch (cpu->id_type())
			{
			case 2:
			{
				res = op_branch_targets(pc, spu_opcode_t{static_cast<spu_thread*>(cpu)->_ref<u32>(pc)});
				break;
			}
			case 1:
			{
				be_t<ppu_opcode_t> op{};

				if (vm::check_addr(pc, vm::page_executable) && vm::try_access(pc, &op, 4, false))
					res = op_branch_targets(pc, op);

				break;
			}
			default: break;
			}

			if (const size_t pos = std::basic_string_view<u32>(res.data(), 2).find_last_not_of(umax); pos != umax)
				m_debugger_list->ShowAddress(res[pos] - std::max(row, 0) * 4, true, true);

			return;
		}
		case Qt::Key_M:
		{
			// Memory viewer
			idm::make<memory_viewer_handle>(this, pc, make_check_cpu(cpu));
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
		default: break;
		}
	}
}

cpu_thread* debugger_frame::get_cpu()
{
	// Wait flag is raised by the thread itself, acknowledging exit
	if (m_cpu)
	{
		if (+m_cpu->state + cpu_flag::wait + cpu_flag::exit != +m_cpu->state)
		{
			return m_cpu.get();
		}
	}

	// m_rsx is raw pointer, when emulation is stopped it won't be cleared
	// Therefore need to do invalidation checks manually

	if (m_emu_state == system_state::stopped)
	{
		m_rsx = nullptr;
		return m_rsx;
	}

	if (!g_fxo->is_init<rsx::thread>())
	{
		m_rsx = nullptr;
		return m_rsx;
	}

	if (m_rsx && !m_rsx->ctrl)
	{
		m_rsx = nullptr;
		return m_rsx;
	}

	return m_rsx;
}

std::function<cpu_thread*()> debugger_frame::make_check_cpu(cpu_thread* cpu)
{
	const u32 id = cpu ? cpu->id : umax;
	const u32 type = id >> 24;

	std::shared_ptr<cpu_thread> shared = type == 1 ? static_cast<std::shared_ptr<cpu_thread>>(idm::get<named_thread<ppu_thread>>(id)) :
		type == 2 ? idm::get<named_thread<spu_thread>>(id) : nullptr;

	if (shared.get() != cpu)
	{
		shared.reset();
	}

	return [&rsx = m_rsx, cpu, type, shared = std::move(shared)]() -> cpu_thread*
	{
		if (type == 1 || type == 2)
		{
			// SPU and PPU
			if (!shared || (shared->state & cpu_flag::exit && shared->state & cpu_flag::wait))
			{
				return nullptr;
			}

			return shared.get();
		}

		// RSX
		if (rsx == cpu)
		{
			return cpu;
		}

		return nullptr;
	};
}

void debugger_frame::UpdateUI()
{
	UpdateUnitList();

	const auto cpu = get_cpu();

	if (!cpu)
	{
		if (m_last_pc != umax || !m_last_query_state.empty())
		{
			m_last_query_state.clear();
			m_last_pc = -1;
			DoUpdate();
		}
	}
	else
	{
		const auto cia = cpu->get_pc();
		const auto size_context = cpu->id_type() == 1 ? sizeof(ppu_thread) :
			cpu->id_type() == 2 ? sizeof(spu_thread) : sizeof(cpu_thread);

		if (m_last_pc != cia || m_last_query_state.size() != size_context || std::memcmp(m_last_query_state.data(), static_cast<void *>(cpu), size_context))
		{
			// Copy thread data
			m_last_query_state.resize(size_context);
			std::memcpy(m_last_query_state.data(), static_cast<void *>(cpu), size_context);

			m_last_pc = cia;
			DoUpdate();

			if (cpu->state & s_pause_flags)
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

using data_type = std::pair<cpu_thread*, u32>;

Q_DECLARE_METATYPE(data_type);

void debugger_frame::UpdateUnitList()
{
	const u64 threads_created = cpu_thread::g_threads_created;
	const u64 threads_deleted = cpu_thread::g_threads_deleted;
	const system_state emu_state = Emu.GetStatus();

	if (threads_created != m_threads_created || threads_deleted != m_threads_deleted || emu_state != m_emu_state)
	{
		m_threads_created = threads_created;
		m_threads_deleted = threads_deleted;
		m_emu_state = emu_state;
	}
	else
	{
		// Nothing to do
		return;
	}

	//const int old_size = m_choice_units->count();
	QVariant old_cpu = m_choice_units->currentData();

	bool reselected = false;

	const auto on_select = [&](u32 id, cpu_thread& cpu)
	{
		if (emu_state == system_state::stopped) return;

		const QVariant var_cpu = QVariant::fromValue<std::pair<cpu_thread*, u32>>(std::make_pair(&cpu, id));

		// Space at the end is to pad a gap on the right
		m_choice_units->addItem(qstr((id >> 24 == 0x55 ? "RSX[0x55555555]" : cpu.get_name()) + ' '), var_cpu);

		if (!reselected && old_cpu == var_cpu)
		{
			m_choice_units->setCurrentIndex(m_choice_units->count() - 1);
			reselected = true;
		}
	};

	{
		const QSignalBlocker blocker(m_choice_units);

		m_choice_units->clear();
		m_choice_units->addItem(NoThreadString);

		idm::select<named_thread<ppu_thread>>(on_select);
		idm::select<named_thread<spu_thread>>(on_select);

		if (const auto render = g_fxo->try_get<rsx::thread>(); emu_state != system_state::stopped && render && render->ctrl)
		{
			on_select(render->id, *render);
		}
	}

	// Close dialogs which are tied to the specific thread selected
	if (!reselected)
	{
		if (m_reg_editor) m_reg_editor->close();
		if (m_inst_editor) m_inst_editor->close();
	}

	OnSelectUnit();

	m_choice_units->update();
}

void debugger_frame::OnSelectUnit()
{
	auto [selected, cpu_id] = m_choice_units->currentData().value<std::pair<cpu_thread*, u32>>();

	if (m_emu_state != system_state::stopped)
	{
		if (selected && m_cpu.get() == selected)
		{
			// They match, nothing to do.
			return;
		}

		if (selected && m_rsx == selected)
		{
			return;
		}

		if (!selected && !m_rsx && !m_cpu)
		{
			return;
		}
	}
	else
	{
		selected = nullptr;
	}

	m_disasm.reset();
	m_cpu.reset();
	m_rsx = nullptr;

	if (selected)
	{
		switch (cpu_id >> 24)
		{
		case 1:
		{
			m_cpu = idm::get<named_thread<ppu_thread>>(cpu_id);

			if (selected == m_cpu.get())
			{
				m_disasm = std::make_shared<PPUDisAsm>(cpu_disasm_mode::interpreter, vm::g_sudo_addr);
			}
			else
			{
				m_cpu.reset();
				selected = nullptr;
			}

			break;
		}
		case 2:
		{
			m_cpu = idm::get<named_thread<spu_thread>>(cpu_id);

			if (selected == m_cpu.get())
			{
				m_disasm = std::make_shared<SPUDisAsm>(cpu_disasm_mode::interpreter, static_cast<const spu_thread*>(m_cpu.get())->ls);
			}
			else
			{
				m_cpu.reset();
				selected = nullptr;
			}

			break;
		}
		case 0x55:
		{
			m_rsx = static_cast<rsx::thread*>(selected);

			if (get_cpu())
			{
				m_disasm = std::make_shared<RSXDisAsm>(cpu_disasm_mode::interpreter, vm::g_sudo_addr, m_rsx);
			}

			break;
		}
		default: break;
		}
	}

	EnableButtons(true);

	m_debugger_list->UpdateCPUData(get_cpu(), m_disasm.get());
	m_breakpoint_list->UpdateCPUData(get_cpu(), m_disasm.get());
	DoUpdate();
	UpdateUI();
}

void debugger_frame::DoUpdate()
{
	// Check if we need to disable a step over bp
	if (const auto cpu0 = get_cpu(); cpu0 && m_last_step_over_breakpoint != umax && cpu0->get_pc() == m_last_step_over_breakpoint)
	{
		m_breakpoint_handler->RemoveBreakpoint(m_last_step_over_breakpoint);
		m_last_step_over_breakpoint = -1;
	}

	ShowPC();
	WritePanels();
}

void debugger_frame::WritePanels()
{
	const auto cpu = get_cpu();

	if (!cpu)
	{
		m_misc_state->clear();
		m_regs->clear();
		return;
	}

	int loc = m_misc_state->verticalScrollBar()->value();
	m_misc_state->clear();
	m_misc_state->setText(qstr(cpu->dump_misc()));
	m_misc_state->verticalScrollBar()->setValue(loc);

	loc = m_regs->verticalScrollBar()->value();
	m_regs->clear();
	m_regs->setText(qstr(cpu->dump_regs()));
	m_regs->verticalScrollBar()->setValue(loc);

	Q_EMIT CallStackUpdateRequested(cpu->dump_callstack_list());
}

void debugger_frame::ShowGotoAddressDialog()
{
	QDialog* dlg = new QDialog(this);
	dlg->setWindowTitle(tr("Go To Address"));
	dlg->setModal(true);

	// Panels
	QVBoxLayout* vbox_panel(new QVBoxLayout());
	QHBoxLayout* hbox_expression_input_panel = new QHBoxLayout();
	QHBoxLayout* hbox_button_panel(new QHBoxLayout());

	// Address expression input
	QLineEdit* expression_input(new QLineEdit(dlg));
	expression_input->setFont(m_mono);
	expression_input->setMaxLength(18);

	if (const auto thread = get_cpu(); !thread || thread->id_type() != 2)
	{
		expression_input->setValidator(new QRegExpValidator(QRegExp("^(0[xX])?0*[a-fA-F0-9]{0,8}$")));
	}
	else
	{
		expression_input->setValidator(new QRegExpValidator(QRegExp("^(0[xX])?0*[a-fA-F0-9]{0,5}$")));
	}

	// Ok/Cancel
	QPushButton* button_ok = new QPushButton(tr("OK"));
	QPushButton* button_cancel = new QPushButton(tr("Cancel"));

	hbox_expression_input_panel->addWidget(expression_input);

	hbox_button_panel->addWidget(button_ok);
	hbox_button_panel->addWidget(button_cancel);

	vbox_panel->addLayout(hbox_expression_input_panel);
	vbox_panel->addSpacing(8);
	vbox_panel->addLayout(hbox_button_panel);

	dlg->setLayout(vbox_panel);

	const auto cpu = get_cpu();
	const QFont font = expression_input->font();

	// -1 from get_pc() turns into 0
	const u32 pc = cpu ? utils::align<u32>(cpu->get_pc(), 4) : 0;
	expression_input->setPlaceholderText(QString("0x%1").arg(pc, 16, 16, QChar('0')));
	expression_input->setFixedWidth(gui::utils::get_label_width(expression_input->placeholderText(), &font));

	connect(button_ok, &QAbstractButton::clicked, dlg, &QDialog::accept);
	connect(button_cancel, &QAbstractButton::clicked, dlg, &QDialog::reject);

	dlg->move(QCursor::pos());

	if (dlg->exec() == QDialog::Accepted)
	{
		const u32 address = EvaluateExpression(expression_input->text());
		m_debugger_list->ShowAddress(address, true);
	}

	dlg->deleteLater();
}

u64 debugger_frame::EvaluateExpression(const QString& expression)
{
	bool ok = false;

	// Parse expression(or at least used to, was nuked to remove the need for QtJsEngine)
	const QString fixed_expression = QRegExp("^[A-Fa-f0-9]+$").exactMatch(expression) ? "0x" + expression : expression;
	const u64 res = static_cast<u64>(fixed_expression.toULong(&ok, 16));

	if (ok) return res;
	if (const auto thread = get_cpu()) return thread->get_pc();
	return 0;
}

void debugger_frame::ClearBreakpoints() const
{
	m_breakpoint_list->ClearBreakpoints();
}

void debugger_frame::ClearCallStack()
{
	Q_EMIT CallStackUpdateRequested({});
}

void debugger_frame::ShowPC()
{
	const auto cpu0 = get_cpu();

	const u32 pc = (cpu0 ? cpu0->get_pc() : 0);

	m_debugger_list->ShowAddress(pc, true);
}

void debugger_frame::DoStep(bool step_over)
{
	if (const auto cpu = get_cpu())
	{
		bool should_step_over = step_over && cpu->id_type() == 1;

		if (const auto _state = +cpu->state; _state & s_pause_flags && _state & cpu_flag::wait && !(_state & cpu_flag::dbg_step))
		{
			if (should_step_over)
			{
				const u32 current_instruction_pc = cpu->get_pc();

				// Set breakpoint on next instruction
				const u32 next_instruction_pc = current_instruction_pc + 4;
				m_breakpoint_handler->AddBreakpoint(next_instruction_pc);

				// Undefine previous step over breakpoint if it hasnt been already
				// This can happen when the user steps over a branch that doesn't return to itself
				if (m_last_step_over_breakpoint != umax)
				{
					m_breakpoint_handler->RemoveBreakpoint(next_instruction_pc);
				}

				m_last_step_over_breakpoint = next_instruction_pc;
			}

			cpu->state.atomic_op([&](bs_t<cpu_flag>& state)
			{
				state -= s_pause_flags;

				if (!should_step_over)
				{
					if (u32* ptr = cpu->get_pc2())
					{
						state += cpu_flag::dbg_step;
						*ptr = cpu->get_pc();
					}
				}
			});

			cpu->state.notify_one(s_pause_flags);
		}
	}

	UpdateUI();
}

void debugger_frame::EnableUpdateTimer(bool enable) const
{
	enable ? m_update->start(50) : m_update->stop();
}

void debugger_frame::EnableButtons(bool enable)
{
	if (!get_cpu()) enable = false;

	m_go_to_addr->setEnabled(enable);
	m_go_to_pc->setEnabled(enable);
	m_btn_step->setEnabled(enable);
	m_btn_step_over->setEnabled(enable);
	m_btn_run->setEnabled(enable);
}
