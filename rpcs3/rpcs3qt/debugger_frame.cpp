#include "debugger_frame.h"
#include "register_editor_dialog.h"
#include "instruction_editor_dialog.h"
#include "memory_viewer_panel.h"
#include "elf_memory_dumping_dialog.h"
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
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"

#include <QKeyEvent>
#include <QScrollBar>
#include <QFontDatabase>
#include <QCompleter>
#include <QVBoxLayout>
#include <QTimer>
#include <QCheckBox>
#include <QMessageBox>
#include <charconv>

#include "util/asm.hpp"

constexpr auto qstr = QString::fromStdString;

constexpr auto s_pause_flags = cpu_flag::dbg_pause + cpu_flag::dbg_global_pause;

extern atomic_t<bool> g_debugger_pause_all_threads_on_bp;

extern const ppu_decoder<ppu_itype> g_ppu_itype;

extern bool is_using_interpreter(u32 id_type)
{
	switch (id_type)
	{
	case 1: return g_cfg.core.ppu_decoder != ppu_decoder_type::llvm;
	case 2: return g_cfg.core.spu_decoder != spu_decoder_type::asmjit && g_cfg.core.spu_decoder != spu_decoder_type::llvm;
	default: return true;
	}
}

extern std::shared_ptr<CPUDisAsm> make_disasm(const cpu_thread* cpu)
{
	switch (cpu->id_type())
	{
	case 1: return std::make_shared<PPUDisAsm>(cpu_disasm_mode::interpreter, vm::g_sudo_addr);
	case 2: return std::make_shared<SPUDisAsm>(cpu_disasm_mode::interpreter, static_cast<const spu_thread*>(cpu)->ls);
	case 0x55: return std::make_shared<RSXDisAsm>(cpu_disasm_mode::interpreter, vm::g_sudo_addr, 0, cpu);
	default: return nullptr;
	}
}

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

	m_ppu_breakpoint_handler = new breakpoint_handler();
	m_breakpoint_list = new breakpoint_list(this, m_ppu_breakpoint_handler);

	m_debugger_list = new debugger_list(this, m_gui_settings, m_ppu_breakpoint_handler);
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
	m_choice_units->installEventFilter(this);

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
	m_misc_state = new QPlainTextEdit(this);
	m_misc_state->setLineWrapMode(QPlainTextEdit::NoWrap);
	m_misc_state->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

	// Registers
	m_regs = new QPlainTextEdit(this);
	m_regs->setLineWrapMode(QPlainTextEdit::NoWrap);
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
	connect(m_go_to_pc, &QAbstractButton::clicked, this, [this]() { ShowPC(true); });

	connect(m_btn_step, &QAbstractButton::clicked, this, &debugger_frame::DoStep);
	connect(m_btn_step_over, &QAbstractButton::clicked, [this]() { DoStep(true); });

	connect(m_btn_run, &QAbstractButton::clicked, this, &debugger_frame::RunBtnPress);

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

	m_debugger_list->RefreshView();
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
		event->accept(); // Restore accepted state
		return false;
	}

	if (object == m_choice_units && event->type() == QEvent::FocusOut)
	{
		if (int index = m_choice_units->currentIndex(); index >= 0)
		{
			// Restore item text automatically on focus-out after search
			m_choice_units->setCurrentText(m_choice_units->itemText(index));
		}
	}

	return false;
}

void debugger_frame::closeEvent(QCloseEvent* event)
{
	SaveSettings();
	m_gui_settings->sync();

	QDockWidget::closeEvent(event);
	Q_EMIT DebugFrameClosed();
}

void debugger_frame::showEvent(QShowEvent* event)
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

void debugger_frame::hideEvent(QHideEvent* event)
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
	dlg->open();
}

void debugger_frame::keyPressEvent(QKeyEvent* event)
{
	if (!isActiveWindow())
	{
		event->ignore();
		return;
	}

	const auto cpu = get_cpu();
	const int row = m_debugger_list->currentRow();

	switch (event->key())
	{
	case Qt::Key_F1:
	{
		if (event->isAutoRepeat())
		{
			event->ignore();
			return;
		}

		QDialog* dlg = new QDialog(this);
		dlg->setAttribute(Qt::WA_DeleteOnClose);
		dlg->setWindowTitle(tr("Debugger Guide & Shortcuts"));

		QLabel* l = new QLabel(tr(
			"Keys Ctrl+G: Go to typed address."
			"\nKeys Ctrl+B: Open breakpoints settings."
			"\nKeys Ctrl+C: Copy instruction contents."
			"\nKeys Ctrl+F: Find thread."
			"\nKeys Alt+S: Capture SPU images of selected SPU or generalized form when used from PPU."
			"\nKeys Alt+S: Launch a memory viewer pointed to the current RSX semaphores location when used from RSX."
			"\nKeys Alt+R: Load last saved SPU state capture."
			"\nKey D: SPU MFC commands logger, MFC debug setting must be enabled."
			"\nKey D: Also PPU calling history logger, interpreter and non-zero call history size must be used."
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
		dlg->open();
		return;
	}
	default: break;
	}

	if (event->modifiers() == Qt::ControlModifier)
	{
		switch (const auto key = event->key())
		{
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
		{
			if (event->isAutoRepeat())
			{
				event->ignore();
				break;
			}

			const int count = m_choice_units->count();
			const int cur_index = m_choice_units->currentIndex();

			if (count && cur_index >= 0)
			{
				// Wrap around
				// Adding count so the result would not be negative, that would alter the remainder operation
				m_choice_units->setCurrentIndex((cur_index + count + (key == Qt::Key_PageUp ? -1 : 1)) % count);
			}

			return;
		}
		case Qt::Key_F:
		{
			m_choice_units->clearEditText();
			m_choice_units->setFocus();
			return;
		}
		default: break;
		}
	}

	if (!cpu)
	{
		event->ignore();
		return;
	}

	const u32 address_limits = (cpu->id_type() == 2 ? 0x3fffc : ~3);
	const u32 pc = (m_debugger_list->m_pc & address_limits);
	const u32 selected = (m_debugger_list->m_showing_selected_instruction ? m_debugger_list->m_selected_instruction : cpu->get_pc()) & address_limits;

	const auto modifiers = event->modifiers();

	if (modifiers == Qt::ControlModifier)
	{
		if (event->isAutoRepeat())
		{
			event->ignore();
			return;
		}

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
			if (event->isAutoRepeat())
			{
				break;
			}

			auto get_max_allowed = [&](QString title, QString description, u32 limit) -> u32
			{
				input_dialog dlg(4, "", title, description.arg(limit), QString::number(limit), this);

				QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
				mono.setPointSize(8);
				dlg.set_input_font(mono, false);
				dlg.set_clear_button_enabled(false);
				dlg.set_button_enabled(QDialogButtonBox::StandardButton::Ok, false);
				dlg.set_validator(new QRegularExpressionValidator(QRegularExpression("^[1-9][0-9]*$"), &dlg));

				u32 max = 0;

				connect(&dlg, &input_dialog::text_changed, [&](const QString& changed)
				{
					bool ok = false;
					const u32 dummy = changed.toUInt(&ok, 10);
					ok = ok && dummy && dummy <= limit;
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

				return max;
			};

			auto copy_overlapping_list = [&] <typename T> (u64& index, u64 max, const std::vector<T>& in, std::vector<T>& out, bool& emptied)
			{
				max = std::min<u64>(max, in.size());

				const u64 current_pos = index % in.size();
				const u64 last_elements = std::min<u64>(current_pos, max);
				const u64 overlapped_old_elements = std::min<u64>(index, max) - last_elements;

				out.resize(overlapped_old_elements + last_elements);

				// Save list contents (only the relavant parts)
				std::copy(in.end() - overlapped_old_elements, in.end(), out.begin());
				std::copy_n(in.begin() + current_pos - last_elements, last_elements, out.begin() + overlapped_old_elements);

				// Check if max elements to log is larger/equal to current list size
				if ((emptied = index && max >= index))
				{
					// Empty list when possible (further calls' history logging will not log any call before this)
					index = 0;
				}
			};


			if (cpu->id_type() == 2 && g_cfg.core.mfc_debug)
			{
				const u32 max = get_max_allowed(tr("Max MFC cmds logged"), tr("Decimal only, max allowed is %0."), spu_thread::max_mfc_dump_idx);

				// Preallocate in order to save execution time when inside suspend_all.
				std::vector<mfc_cmd_dump> copy(max);

				bool emptied = false;

				cpu_thread::suspend_all(nullptr, {}, [&]
				{
					const auto spu = static_cast<spu_thread*>(cpu);
					copy_overlapping_list(spu->mfc_dump_idx, max, spu->mfc_history, copy, emptied);
				});

				std::string ret;

				u32 i = 0;
				for (auto it = copy.rbegin(); it != copy.rend(); it++, i++)
				{
					auto& dump = *it;

					const u32 pc = std::exchange(dump.cmd.eah, 0);
					fmt::append(ret, "\n(%d) PC 0x%05x: [%s] (%s)", i, pc, dump.cmd, spu_block_hash{dump.block_hash});

					if (dump.cmd.cmd == MFC_PUTLLC_CMD)
					{
						fmt::append(ret, " %s", dump.cmd.tag == MFC_PUTLLC_SUCCESS ? "(passed)" : "(failed)");
					}

					auto load = [&](usz index)
					{
						be_t<u32> data{};
						std::memcpy(&data, dump.data + index * sizeof(data), sizeof(data));
						return data;
					};

					for (usz i = 0; i < utils::aligned_div(std::min<u32>(dump.cmd.size, 128), 4); i += 4)
					{
						fmt::append(ret, "\n[0x%02x] %08x %08x %08x %08x", i * sizeof(be_t<u32>)
							, load(i + 0), load(i + 1), load(i + 2), load(i + 3));
					}
				}

				if (ret.empty())
				{
					ret = "No MFC commands have been logged";
				}

				if (emptied)
				{
					ret += "\nPrevious MFC history has been emptied!";
				}

				spu_log.success("SPU MFC dump of '%s': %s", cpu->get_name(), ret);
			}
			else if (cpu->id_type() == 1 && g_cfg.core.ppu_call_history)
			{
				const u32 max = get_max_allowed(tr("Max PPU calls logged"), tr("Decimal only, max allowed is %0."), ppu_thread::call_history_max_size);

				// Preallocate in order to save execution time when inside suspend_all.
				std::vector<u32> copy(max);
				std::vector<typename ppu_thread::syscall_history_t::entry_t> sys_copy(ppu_thread::syscall_history_max_size);

				std::array<bool, 2> emptied{};

				cpu_thread::suspend_all(nullptr, {}, [&]
				{
					auto& list = static_cast<ppu_thread*>(cpu)->call_history;
					auto& sys_list = static_cast<ppu_thread*>(cpu)->syscall_history;
					copy_overlapping_list(list.index, max, list.data, copy, emptied[0]);
					copy_overlapping_list(sys_list.index, max, sys_list.data, sys_copy, emptied[1]);
				});

				std::string ret;

				PPUDisAsm dis_asm(cpu_disasm_mode::normal, vm::g_sudo_addr);
				u32 i = 0;

				for (auto it = copy.rbegin(); it != copy.rend(); it++, i++)
				{
					dis_asm.disasm(*it);
					fmt::append(ret, "\n(%u) 0x%08x: %s", i, *it, dis_asm.last_opcode);
				}

				i = 0;
				for (auto it = sys_copy.rbegin(); it != sys_copy.rend(); it++, i++)
				{
					fmt::append(ret, "\n(%u) 0x%08x: %s, 0x%x, r3=0x%x, r4=0x%x, r5=0x%x, r6=0x%x", i, it->cia, it->func_name, it->error, it->args[0], it->args[1], it->args[2], it->args[3]);
				}

				if (ret.empty())
				{
					ret = "No PPU calls have been logged";
				}

				if (emptied[0])
				{
					ret += "\nPrevious call history has been emptied!";
				}

				if (emptied[1])
				{
					ret += "\nPrevious HLE call history has been emptied!";
				}

				ppu_log.success("PPU calling history dump of '%s': %s", cpu->get_name(), ret);
			}

			return;
		}
		case Qt::Key_E:
		{
			if (event->isAutoRepeat())
			{
				break;
			}

			if (cpu->id_type() == 1 || cpu->id_type() == 2)
			{
				if (!m_inst_editor)
				{
					m_inst_editor = new instruction_editor_dialog(this, selected, m_disasm.get(), make_check_cpu(cpu));
					connect(m_inst_editor, &QDialog::finished, this, [this]() { m_inst_editor = nullptr; });
					m_inst_editor->show();
				}
			}

			return;
		}
		case Qt::Key_F:
		{
			if (event->isAutoRepeat())
			{
				break;
			}

			if (cpu->id_type() == 1)
			{
				static_cast<ppu_thread*>(cpu)->debugger_mode.atomic_op([](ppu_debugger_mode& mode)
				{
					mode = static_cast<ppu_debugger_mode>((static_cast<u32>(mode) + 1) % static_cast<u32>(ppu_debugger_mode::max_mode));
				});

				return;
			}
			if (cpu->id_type() == 2)
			{
				static_cast<spu_thread*>(cpu)->debugger_mode.atomic_op([](spu_debugger_mode& mode)
				{
					mode = static_cast<spu_debugger_mode>((static_cast<u32>(mode) + 1) % static_cast<u32>(spu_debugger_mode::max_mode));
				});

				return;
			}

			break;
		}
		case Qt::Key_R:
		{
			if (event->isAutoRepeat())
			{
				break;
			}

			if (cpu->id_type() == 1 || cpu->id_type() == 2)
			{
				if (cpu->id_type() == 2 && modifiers & Qt::AltModifier)
				{
					static_cast<spu_thread*>(cpu)->try_load_debug_capture();
					return;
				}

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
			if (event->isAutoRepeat())
			{
				break;
			}

			if (modifiers & Qt::AltModifier)
			{
				if (cpu->id_type() == 0x55)
				{
					if (u32 addr = static_cast<rsx::thread*>(cpu)->label_addr)
					{
						// Memory viewer pointing to RSX semaphores
						idm::make<memory_viewer_handle>(this, m_disasm, addr, make_check_cpu(nullptr));
					}

					return;
				}

				if (cpu->id_type() == 1)
				{
					new elf_memory_dumping_dialog(pc, m_gui_settings, this);
					return;
				}

				if (cpu->id_type() != 2)
				{
					return;
				}

				if (!cpu->state.all_of(cpu_flag::wait + cpu_flag::dbg_pause))
				{
					QMessageBox::warning(this, QObject::tr("Pause the SPU Thread!"), QObject::tr("Cannot perform SPU capture due to the thread needing manual pausing!"));
					return;
				}

				static_cast<spu_thread*>(cpu)->capture_state();
				return;
			}

			break;
		}
		case Qt::Key_N:
		{
			// Next instruction according to code flow
			// Known branch targets are selected over next PC for conditional branches
			// Indirect branches (unknown targets, such as function return) do not proceed to any instruction
			std::array<u32, 2> res{umax, umax};

			const u32 selected = (m_debugger_list->m_showing_selected_instruction ? m_debugger_list->m_selected_instruction : cpu->get_pc()) & address_limits;

			switch (cpu->id_type())
			{
			case 2:
			{
				res = op_branch_targets(selected, spu_opcode_t{static_cast<spu_thread*>(cpu)->_ref<u32>(selected)});
				break;
			}
			case 1:
			{
				be_t<ppu_opcode_t> op{};

				if (vm::check_addr(selected, vm::page_executable) && vm::try_access(selected, &op, 4, false))
					res = op_branch_targets(selected, op);

				break;
			}
			default: break;
			}

			if (const usz pos = std::basic_string_view<u32>(res.data(), 2).find_last_not_of(umax); pos != umax)
				m_debugger_list->ShowAddress(res[pos] - std::max(row, 0) * 4, true);

			return;
		}
		case Qt::Key_M:
		{
			if (event->isAutoRepeat())
			{
				break;
			}

			if (m_disasm && cpu->id_type() == 2)
			{
				// Save shared pointer to shared memory handle, ensure the destructor will not be called until the SPUDisAsm is destroyed
				static_cast<SPUDisAsm*>(m_disasm.get())->set_shm(static_cast<const spu_thread*>(cpu)->shm);
			}

			// Memory viewer
			idm::make<memory_viewer_handle>(this, m_disasm, pc, make_check_cpu(cpu));
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

	event->ignore();
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

std::function<cpu_thread*()> debugger_frame::make_check_cpu(cpu_thread* cpu, bool unlocked)
{
	const u32 id = cpu ? cpu->id : umax;
	const u32 type = id >> 24;

	std::shared_ptr<cpu_thread> shared;

	if (g_fxo->is_init<id_manager::id_map<named_thread<ppu_thread>>>() && g_fxo->is_init<id_manager::id_map<named_thread<spu_thread>>>())
	{
		if (unlocked)
		{
			if (type == 1)
			{
				shared = idm::get_unlocked<named_thread<ppu_thread>>(id);
			}
			else if (type == 2)
			{
				shared = idm::get_unlocked<named_thread<spu_thread>>(id);
			}
		}
		else
		{
			if (type == 1)
			{
				shared = idm::get<named_thread<ppu_thread>>(id);
			}
			else if (type == 2)
			{
				shared = idm::get<named_thread<spu_thread>>(id);
			}
		}
	}

	if (shared.get() != cpu)
	{
		shared.reset();
	}

	return [cpu, type, shared = std::move(shared), emulation_id = Emu.GetEmulationIdentifier()]() -> cpu_thread*
	{
		if (emulation_id != Emu.GetEmulationIdentifier())
		{
			// Invalidate all data after Emu.Kill()
			return nullptr;
		}

		if (type == 1 || type == 2)
		{
			// SPU and PPU
			if (!shared || shared->state.all_of(cpu_flag::exit + cpu_flag::wait))
			{
				return nullptr;
			}

			return shared.get();
		}

		// RSX
		if (g_fxo->try_get<rsx::thread>() == cpu)
		{
			return cpu;
		}

		return nullptr;
	};
}

void debugger_frame::UpdateUI()
{
	const auto cpu = get_cpu();

	// Refresh at a high rate during initialization (looks weird otherwise)
	if (m_ui_update_ctr % (cpu || m_ui_update_ctr < 200 || m_debugger_list->m_dirty_flag ? 5 : 50) == 0)
	{
		// If no change to instruction position happened, update instruction list at 20hz
		ShowPC();
	}

	if (m_ui_update_ctr % 20 == 0)
	{
		// Update threads list at 5hz (low priority)
		UpdateUnitList();
	}

	if (!cpu)
	{
		if (m_last_pc != umax || !m_last_query_state.empty())
		{
			UpdateUnitList();
			ShowPC();
			m_last_query_state.clear();
			m_last_pc = -1;
			DoUpdate();
		}
	}
	else if (m_ui_update_ctr % 5 == 0 || m_ui_update_ctr < m_ui_fast_update_permission_deadline)
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

			const bool paused = !!(cpu->state & s_pause_flags);

			if (paused)
			{
				m_btn_run->setText(RunString);
			}
			else
			{
				m_btn_run->setText(PauseString);
			}

			if (m_ui_update_ctr % 5)
			{
				// Call if it hasn't been called before
				ShowPC();
			}

			if (is_using_interpreter(cpu->id_type()))
			{
				m_btn_step->setEnabled(paused);
				m_btn_step_over->setEnabled(paused);
			}
		}
	}

	m_ui_update_ctr++;
}

using data_type = std::function<cpu_thread*()>;

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

	QVariant old_cpu = m_choice_units->currentData();

	bool reselected = false;

	const auto on_select = [&](u32 id, cpu_thread& cpu)
	{
		const QVariant var_cpu = QVariant::fromValue<data_type>(make_check_cpu(std::addressof(cpu), true));

		// Space at the end is to pad a gap on the right
		m_choice_units->addItem(qstr((id >> 24 == 0x55 ? "RSX[0x55555555]" : cpu.get_name()) + ' '), var_cpu);

		if (!reselected && old_cpu.canConvert<data_type>() && old_cpu.value<data_type>()() == std::addressof(cpu))
		{
			m_choice_units->setCurrentIndex(m_choice_units->count() - 1);
			reselected = true;
		}
	};

	{
		const QSignalBlocker blocker(m_choice_units);

		m_choice_units->clear();
		m_choice_units->addItem(NoThreadString);

		if (emu_state != system_state::stopped)
		{
			idm::select<named_thread<ppu_thread>>(on_select);
			idm::select<named_thread<spu_thread>>(on_select);
		}

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
		if (m_goto_dialog) m_goto_dialog->close();
	}

	if (emu_state == system_state::stopped)
	{
		ClearBreakpoints();
		ClearCallStack();
	}

	OnSelectUnit();

	m_choice_units->update();
}

void debugger_frame::OnSelectUnit()
{
	cpu_thread* selected = nullptr;

	if (m_emu_state != system_state::stopped)
	{
		if (const QVariant data = m_choice_units->currentData(); data.canConvert<data_type>())
		{
			selected = data.value<data_type>()();
		}

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

	m_disasm.reset();
	m_cpu.reset();
	m_rsx = nullptr;

	if (selected)
	{
		const u32 cpu_id = selected->id;

		switch (cpu_id >> 24)
		{
		case 1:
		{
			m_cpu = idm::get<named_thread<ppu_thread>>(cpu_id);

			if (selected == m_cpu.get())
			{
				m_disasm = make_disasm(selected);
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
				m_disasm = make_disasm(selected);
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
				m_disasm = make_disasm(m_rsx);
			}

			break;
		}
		default: break;
		}
	}

	EnableButtons(true);

	m_debugger_list->UpdateCPUData(get_cpu(), m_disasm.get());
	m_breakpoint_list->UpdateCPUData(get_cpu(), m_disasm.get());
	ShowPC(true);
	DoUpdate();
	UpdateUI();
}

void debugger_frame::DoUpdate()
{
	// Check if we need to disable a step over bp
	if (const auto cpu0 = get_cpu(); cpu0 && m_last_step_over_breakpoint != umax && cpu0->get_pc() == m_last_step_over_breakpoint)
	{
		m_ppu_breakpoint_handler->RemoveBreakpoint(m_last_step_over_breakpoint);
		m_last_step_over_breakpoint = -1;
	}

	WritePanels();
}

void debugger_frame::WritePanels()
{
	const auto cpu = get_cpu();

	if (!cpu)
	{
		m_misc_state->clear();
		m_regs->clear();
		ClearCallStack();
		return;
	}

	int loc = m_misc_state->verticalScrollBar()->value();
	int hloc = m_misc_state->horizontalScrollBar()->value();
	m_misc_state->clear();
	m_misc_state->setPlainText(qstr(cpu->dump_misc()));
	m_misc_state->verticalScrollBar()->setValue(loc);
	m_misc_state->horizontalScrollBar()->setValue(hloc);

	loc = m_regs->verticalScrollBar()->value();
	hloc = m_regs->horizontalScrollBar()->value();
	m_regs->clear();
	m_last_reg_state.clear();
	cpu->dump_regs(m_last_reg_state, m_dump_reg_func_data);
	m_regs->setPlainText(qstr(m_last_reg_state));
	m_regs->verticalScrollBar()->setValue(loc);
	m_regs->horizontalScrollBar()->setValue(hloc);

	Q_EMIT CallStackUpdateRequested(cpu->dump_callstack_list());
}

void debugger_frame::ShowGotoAddressDialog()
{
	if (m_goto_dialog)
	{
		m_goto_dialog->move(QCursor::pos());
		m_goto_dialog->show();
		m_goto_dialog->setFocus();
		return;
	}

	m_goto_dialog = new QDialog(this);
	m_goto_dialog->setWindowTitle(tr("Go To Address"));

	// Panels
	QVBoxLayout* vbox_panel(new QVBoxLayout());
	QHBoxLayout* hbox_expression_input_panel = new QHBoxLayout();
	QHBoxLayout* hbox_button_panel(new QHBoxLayout());

	// Address expression input
	QLineEdit* expression_input(new QLineEdit(m_goto_dialog));
	expression_input->setFont(m_mono);
	expression_input->setMaxLength(18);

	if (const auto thread = get_cpu(); !thread || thread->id_type() != 2)
	{
		expression_input->setValidator(new QRegularExpressionValidator(QRegularExpression("^(0[xX])?0*[a-fA-F0-9]{0,8}$"), this));
	}
	else
	{
		expression_input->setValidator(new QRegularExpressionValidator(QRegularExpression("^(0[xX])?0*[a-fA-F0-9]{0,5}$"), this));
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

	m_goto_dialog->setLayout(vbox_panel);

	const auto cpu_check = make_check_cpu(get_cpu());
	const auto cpu = cpu_check();
	const QFont font = expression_input->font();

	// -1 from get_pc() turns into 0
	const u32 pc = cpu ? utils::align<u32>(cpu->get_pc(), 4) : 0;
	expression_input->setPlaceholderText(QString("0x%1").arg(pc, 16, 16, QChar('0')));
	expression_input->setFixedWidth(gui::utils::get_label_width(expression_input->placeholderText(), &font));

	connect(button_ok, &QAbstractButton::clicked, m_goto_dialog, &QDialog::accept);
	connect(button_cancel, &QAbstractButton::clicked, m_goto_dialog, &QDialog::reject);

	m_goto_dialog->move(QCursor::pos());
	m_goto_dialog->setAttribute(Qt::WA_DeleteOnClose);

	connect(m_goto_dialog, &QDialog::finished, this, [this, cpu, cpu_check, expression_input](int result)
	{
		// Check if the thread has not been destroyed and is still the focused since
		// This also works if no thread is selected and has been selected before
		if (result == QDialog::Accepted && cpu == get_cpu() && cpu == cpu_check())
		{
			PerformGoToRequest(expression_input->text());
		}

		m_goto_dialog = nullptr;
	});

	m_goto_dialog->show();
}

void debugger_frame::PerformGoToRequest(const QString& text_argument)
{
	const bool asterisk_prefixed = text_argument.startsWith(QChar('*'));
	const u64 address = EvaluateExpression(asterisk_prefixed ? text_argument.sliced(1, text_argument.size() - 1) : text_argument);

	if (address != umax)
	{
		// Try to read from OPD entry if prefixed by asterisk
		if (asterisk_prefixed)
		{
			if (auto cpu = get_cpu())
			{
				if (auto ppu = cpu->try_get<ppu_thread>())
				{
					const vm::ptr<u32> func_ptr = vm::cast(static_cast<u32>(address));

					be_t<u32> code_addr{};

					if (func_ptr.try_read(code_addr))
					{
						m_debugger_list->ShowAddress(code_addr, true);
					}
				}
			}

			return;
		}

		m_debugger_list->ShowAddress(static_cast<u32>(address), true);
	}
}

void debugger_frame::PerformGoToThreadRequest(const QString& text_argument)
{
	const u64 thread_id = EvaluateExpression(text_argument);

	if (thread_id != umax)
	{
		for (int i = 0; i < m_choice_units->count(); i++)
		{
			QVariant cpu = m_choice_units->itemData(i);

			if (cpu.canConvert<data_type>())
			{
				if (cpu_thread* ptr = cpu.value<data_type>()(); ptr && ptr->id == thread_id)
				{
					// Success
					m_choice_units->setCurrentIndex(i);
					return;
				}
			}
		}
	}
}

void debugger_frame::PerformAddBreakpointRequest(u32 addr)
{
	m_debugger_list->BreakpointRequested(addr, true);
}

u64 debugger_frame::EvaluateExpression(const QString& expression)
{
	bool ok = false;

	// Parse expression (or at least used to, was nuked to remove the need for QtJsEngine)
	const QString fixed_expression = QRegularExpression(QRegularExpression::anchoredPattern("a .*|^[A-Fa-f0-9]+$")).match(expression).hasMatch() ? "0x" + expression : expression;
	const u64 res = static_cast<u64>(fixed_expression.toULong(&ok, 16));

	if (ok) return res;
	if (const auto thread = get_cpu(); thread && expression.isEmpty()) return thread->get_pc();
	return umax;
}

void debugger_frame::ClearBreakpoints() const
{
	m_breakpoint_list->ClearBreakpoints();
}

void debugger_frame::ClearCallStack()
{
	Q_EMIT CallStackUpdateRequested({});
}

void debugger_frame::ShowPC(bool user_requested)
{
	const auto cpu0 = get_cpu();

	const u32 pc = (cpu0 ? cpu0->get_pc() : 0);

	if (user_requested)
	{
		m_debugger_list->EnableThreadFollowing();
	}

	m_debugger_list->ShowAddress(pc, false);
}

void debugger_frame::DoStep(bool step_over)
{
	if (const auto cpu = get_cpu())
	{
		bool should_step_over = step_over && cpu->id_type() == 1;

		// If stepping over, lay at the same spot and wait for the thread to finish the call
		// If not, fixate on the current pointed instruction
		m_debugger_list->EnableThreadFollowing(!should_step_over);

		if (should_step_over)
		{
			m_debugger_list->ShowAddress(cpu->get_pc() + 4, false);
		}

		if (step_over && cpu->id_type() == 0x55)
		{
			const bool was_paused = cpu->is_paused();
			static_cast<rsx::thread*>(cpu)->pause_on_draw = true;

			if (was_paused)
			{
				RunBtnPress();
				m_debugger_list->EnableThreadFollowing(true);
			}

			return;
		}

		if (const auto _state = +cpu->state; _state & s_pause_flags && _state & cpu_flag::wait && !(_state & cpu_flag::dbg_step))
		{
			if (should_step_over && cpu->id_type() == 1)
			{
				const u32 current_instruction_pc = cpu->get_pc();

				vm::ptr<u32> inst_ptr = vm::cast(current_instruction_pc);
				be_t<u32> result{};

				if (inst_ptr.try_read(result))
				{
					ppu_opcode_t ppu_op{result};
					const ppu_itype::type itype = g_ppu_itype.decode(ppu_op.opcode);

					should_step_over = (itype & ppu_itype::branch && ppu_op.lk);
				}
			}

			if (should_step_over)
			{
				const u32 current_instruction_pc = cpu->get_pc();

				// Set breakpoint on next instruction
				const u32 next_instruction_pc = current_instruction_pc + 4;
				m_ppu_breakpoint_handler->AddBreakpoint(next_instruction_pc);

				// Undefine previous step over breakpoint if it hasn't been already
				// This can happen when the user steps over a branch that doesn't return to itself
				if (m_last_step_over_breakpoint != umax)
				{
					m_ppu_breakpoint_handler->RemoveBreakpoint(next_instruction_pc);
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

			cpu->state.notify_one();
		}
	}

	// Tighten up, put the debugger on a wary watch over any thread info changes if there aren't any
	// This allows responsive debugger interaction
	m_ui_fast_update_permission_deadline = m_ui_update_ctr + 5;
}

void debugger_frame::EnableUpdateTimer(bool enable) const
{
	if (m_update->isActive() == enable)
	{
		return;
	}

	enable ? m_update->start(10) : m_update->stop();
}

void debugger_frame::RunBtnPress()
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

			cpu->state.notify_one();
			m_debugger_list->EnableThreadFollowing();
		}
	}

	// Tighten up, put the debugger on a wary watch over any thread info changes if there aren't any
	// This allows responsive debugger interaction
	m_ui_fast_update_permission_deadline = m_ui_update_ctr + 5;
}

void debugger_frame::EnableButtons(bool enable)
{
	const auto cpu = get_cpu();

	if (!cpu) enable = false;

	const bool step = enable && is_using_interpreter(cpu->id_type());

	m_go_to_addr->setEnabled(enable);
	m_go_to_pc->setEnabled(enable);
	m_btn_step->setEnabled(step);
	m_btn_step_over->setEnabled(step);
	m_btn_run->setEnabled(enable);
}
