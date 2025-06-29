#include "debugger_list.h"
#include "gui_settings.h"
#include "qt_utils.h"
#include "breakpoint_handler.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/RSX/RSXDisAsm.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/System.h"

#include "util/asm.hpp"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QLabel>

#include <memory>

debugger_list::debugger_list(QWidget* parent, std::shared_ptr<gui_settings> gui_settings, breakpoint_handler* handler)
	: QListWidget(parent)
	, m_gui_settings(std::move(gui_settings))
	, m_ppu_breakpoint_handler(handler)
{
	setWindowTitle(tr("ASM"));

	for (uint i = 0; i < m_item_count; ++i)
	{
		insertItem(i, new QListWidgetItem(""));
	}

	setSizeAdjustPolicy(QListWidget::AdjustToContents);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	connect(this, &QListWidget::currentRowChanged, this, [this](int row)
	{
		if (row < 0)
		{
			m_selected_instruction = -1;
			m_showing_selected_instruction = false;
			return;
		}

		u32 pc = m_start_addr;

		const auto cpu = m_disasm && !Emu.IsStopped() ? m_disasm->get_cpu() : nullptr;

		for (; cpu && cpu->get_class() == thread_class::rsx && row; row--)
		{
			// If scrolling forwards (downwards), we can skip entire commands
			pc += std::max<u32>(m_disasm->disasm(pc), 4);
		}

		m_selected_instruction = pc + row * 4;
	});
}

void debugger_list::UpdateCPUData(std::shared_ptr<CPUDisAsm> disasm)
{
	if ((!m_disasm) != (!disasm) || (m_disasm && disasm->get_cpu() != m_disasm->get_cpu()))
	{
		m_selected_instruction = -1;
		m_showing_selected_instruction = false;
	}

	m_disasm = std::move(disasm);
}

u32 debugger_list::GetStartAddress(u32 address)
{
	const auto cpu = m_disasm && !Emu.IsStopped() ? m_disasm->get_cpu() : nullptr;

	const u32 steps = m_item_count / 3;
	const u32 inst_count_jump_on_step = std::min<u32>(steps, 4);

	const bool is_spu = IsSpu();
	const u32 address_mask = (is_spu ? 0x3fffc : ~3);

	u32 result = address & address_mask;

	if (cpu && cpu->get_class() == thread_class::rsx)
	{
		if (auto [count, res] = static_cast<rsx::thread*>(cpu)->try_get_pc_of_x_cmds_backwards(steps, address); count == steps)
		{
			result = res;
		}
	}
	else
	{
		result = (address - (steps * 4)) & address_mask;
	}

	u32 upper_bound = (m_start_addr + (steps * 4)) & address_mask;

	if (cpu && cpu->get_class() == thread_class::rsx)
	{
		if (auto [count, res] = static_cast<rsx::thread*>(cpu)->try_get_pc_of_x_cmds_backwards(0 - steps, m_start_addr); count == steps)
		{
			upper_bound = res;
		}
	}

	bool goto_addr = false;

	if (upper_bound > m_start_addr)
	{
		goto_addr = address < m_start_addr || address >= upper_bound;
	}
	else
	{
		// Overflowing bounds case
		goto_addr = address < m_start_addr && address >= upper_bound;
	}

	if (goto_addr)
	{
		m_pc = address;

		if (address > upper_bound && address - upper_bound < inst_count_jump_on_step * 4)
		{
			m_start_addr = result + inst_count_jump_on_step * 4;
		}
		else
		{
			m_start_addr = result;
		}
	}

	return m_start_addr;
}

bool debugger_list::IsSpu() const
{
	const auto cpu = m_disasm && !Emu.IsStopped() ? m_disasm->get_cpu() : nullptr;

	return (cpu && cpu->get_class() == thread_class::spu) || (m_disasm && !cpu);
}

void debugger_list::ShowAddress(u32 addr, bool select_addr, bool direct)
{
	const auto cpu = m_disasm && !Emu.IsStopped() ? m_disasm->get_cpu() : nullptr;

	const decltype(spu_thread::local_breakpoints)* spu_bps_list{};

	if (cpu && cpu->get_class() == thread_class::spu)
	{
		spu_bps_list = &static_cast<spu_thread*>(cpu)->local_breakpoints;
	}

	auto IsBreakpoint = [&](u32 pc)
	{
		switch (cpu ? cpu->get_class() : thread_class::general)
		{
		case thread_class::ppu:
		{
			return m_ppu_breakpoint_handler->HasBreakpoint(pc, breakpoint_types::bp_exec);
		}
		case thread_class::spu:
		{
			const u32 pos_at = pc / 4;
			const u32 pos_bit = 1u << (pos_at % 8);

			return !!((*spu_bps_list)[pos_at / 8] & pos_bit);
		}
		default: return false;
		}
	};

	if (select_addr || direct)
	{
		// The user wants to survey a specific memory location, do not interfere from this point forth
		m_follow_thread = false;
	}

	m_dirty_flag = false;

	u32 pc = m_start_addr;

	if (!direct && (m_follow_thread || select_addr))
	{
		pc = GetStartAddress(addr);
	}

	const auto& default_foreground = palette().color(foregroundRole());
	const auto& default_background = palette().color(backgroundRole());

	m_showing_selected_instruction = false;

	if (select_addr)
	{
		m_selected_instruction = addr;
	}

	for (uint i = 0; i < m_item_count; ++i)
	{
		if (auto list_item = item(i); list_item->isSelected())
		{
			list_item->setSelected(false);
		}
	}

	if (!m_disasm || (cpu && cpu->state.all_of(cpu_flag::exit + cpu_flag::wait)))
	{
		for (uint i = 0; i < m_item_count; ++i)
		{
			QListWidgetItem* list_item = item(i);
			list_item->setText(QString::fromStdString(fmt::format("   [%08x]  ?? ?? ?? ??:", 0)));
			list_item->setForeground(default_foreground);
			list_item->setBackground(default_background);
		}
	}
	else
	{
		const bool is_spu = IsSpu();
		const u32 address_limits = (is_spu ? 0x3fffc : ~3);
		const u32 current_pc = (cpu ? cpu->get_pc() : 0);
		m_start_addr &= address_limits;
		pc = m_start_addr;

		for (uint i = 0, count = 4; i < m_item_count; ++i, pc = (pc + count) & address_limits)
		{
			QListWidgetItem* list_item = item(i);

			if (pc == current_pc)
			{
				list_item->setForeground(m_text_color_pc);
				list_item->setBackground(m_color_pc);
			}
			else if (pc == m_selected_instruction)
			{
				// setSelected may invoke a resize event which causes stack overflow, terminate recursion
				if (!list_item->isSelected())
				{
					list_item->setSelected(true);
				}

				m_showing_selected_instruction = true;
			}
			else if (IsBreakpoint(pc))
			{
				list_item->setForeground(m_text_color_bp);
				list_item->setBackground(m_color_bp);
			}
			else
			{
				list_item->setForeground(default_foreground);
				list_item->setBackground(default_background);
			}

			if (cpu && cpu->get_class() == thread_class::ppu && !vm::check_addr(pc, 0))
			{
				list_item->setText((IsBreakpoint(pc) ? ">> " : "   ") + QString::fromStdString(fmt::format("[%08x]  ?? ?? ?? ??:", pc)));
				count = 4;
				continue;
			}

			if (cpu && cpu->get_class() == thread_class::ppu && !vm::check_addr(pc, vm::page_executable))
			{
				const u32 data = *vm::get_super_ptr<atomic_be_t<u32>>(pc);
				list_item->setText((IsBreakpoint(pc) ? ">> " : "   ") + QString::fromStdString(fmt::format("[%08x]  %02x %02x %02x %02x:", pc,
				static_cast<u8>(data >> 24),
				static_cast<u8>(data >> 16),
				static_cast<u8>(data >> 8),
				static_cast<u8>(data >> 0))));
				count = 4;
				continue;
			}

			count = m_disasm->disasm(pc);

			if (!count)
			{
				list_item->setText((IsBreakpoint(pc) ? ">> " : "   ") + QString::fromStdString(fmt::format("[%08x] ???     ?? ??", pc)));
				count = 4;
				continue;
			}

			list_item->setText((IsBreakpoint(pc) ? ">> " : "   ") + QString::fromStdString(m_disasm->last_opcode));
		}
	}

	setLineWidth(-1);
}

void debugger_list::RefreshView()
{
	const bool old = std::exchange(m_follow_thread, false);
	ShowAddress(0, false);
	m_follow_thread = old;
}

void debugger_list::EnableThreadFollowing(bool enable)
{
	m_follow_thread = enable;
}

void debugger_list::scroll(s32 steps)
{
	const auto cpu = m_disasm && !Emu.IsStopped() ? m_disasm->get_cpu() : nullptr;

	for (; cpu && cpu->get_class() == thread_class::rsx && steps > 0; steps--)
	{
		// If scrolling forwards (downwards), we can skip entire commands
		m_start_addr += std::max<u32>(m_disasm->disasm(m_start_addr), 4);
	}

	if (cpu && cpu->get_class() == thread_class::rsx && steps < 0)
	{
		// If scrolling backwards (upwards), try to obtain the start of commands tail
		if (auto [count, res] = static_cast<rsx::thread*>(cpu)->try_get_pc_of_x_cmds_backwards(-steps, m_start_addr); count == 0u - steps)
		{
			steps = 0;
			m_start_addr = res;
		}
	}

	EnableThreadFollowing(false);
	m_start_addr += steps * 4;
	ShowAddress(0, false, true);
}

void debugger_list::keyPressEvent(QKeyEvent* event)
{
	const auto cpu = m_disasm && !Emu.IsStopped() ? m_disasm->get_cpu() : nullptr;

	// Always accept event (so it would not bubble upwards, debugger_frame already sees it)
	struct accept_event_t
	{
		QKeyEvent* event;

		~accept_event_t() noexcept
		{
			event->accept();
		}
	} accept_event{event};

	if (!isActiveWindow())
	{
		QListWidget::keyPressEvent(event);
		return;
	}

	if (event->modifiers())
	{
		QListWidget::keyPressEvent(event);
		return;
	}

	switch (event->key())
	{
	case Qt::Key_PageUp:   scroll(0 - m_item_count); return;
	case Qt::Key_PageDown: scroll(m_item_count); return;
	case Qt::Key_Up:       scroll(-1); return;
	case Qt::Key_Down:     scroll(1); return;
	case Qt::Key_I:
	{
		if (event->isAutoRepeat())
		{
			QListWidget::keyPressEvent(event);
			return;
		}

		if (cpu && cpu->get_class() == thread_class::rsx)
		{
			create_rsx_command_detail(m_showing_selected_instruction ? m_selected_instruction : m_pc);
			return;
		}

		break;
	}
	default: break;
	}

	QListWidget::keyPressEvent(event);
}


void debugger_list::showEvent(QShowEvent* event)
{
	if (m_cmd_detail) m_cmd_detail->show();
	QListWidget::showEvent(event);
}

void debugger_list::hideEvent(QHideEvent* event)
{
	if (m_cmd_detail) m_cmd_detail->hide();
	QListWidget::hideEvent(event);
}

void debugger_list::create_rsx_command_detail(u32 pc)
{
	RSXDisAsm rsx_dis = static_cast<RSXDisAsm&>(*m_disasm);
	rsx_dis.change_mode(cpu_disasm_mode::list);

	// Either invalid or not a method
	if (rsx_dis.disasm(pc) <= 4) return;

	if (m_cmd_detail)
	{
		// Edit the existing dialog
		m_detail_label->setText(QString::fromStdString(rsx_dis.last_opcode));
		m_cmd_detail->setFixedSize(m_cmd_detail->sizeHint());
		return;
	}

	m_cmd_detail = new QDialog(this);
	m_cmd_detail->setWindowTitle(tr("RSX Command Detail"));

	m_detail_label = new QLabel(QString::fromStdString(rsx_dis.last_opcode), this);
	m_detail_label->setFont(font());
	gui::utils::set_font_size(*m_detail_label, 10);
	m_detail_label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_detail_label);
	m_cmd_detail->setLayout(layout);
	m_cmd_detail->setFixedSize(m_cmd_detail->sizeHint());
	m_cmd_detail->show();

	connect(m_cmd_detail, &QDialog::finished, [this](int)
	{
		// Cleanup
		std::exchange(m_cmd_detail, nullptr)->deleteLater();
	});
}

void debugger_list::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		int i = currentRow();
		if (i < 0) return;

		u32 pc = m_start_addr;

		const auto cpu = m_disasm && !Emu.IsStopped() ? m_disasm->get_cpu() : nullptr;

		for (; cpu && cpu->get_class() == thread_class::rsx && i; i--)
		{
			// If scrolling forwards (downwards), we can skip entire commands
			pc += std::max<u32>(m_disasm->disasm(pc), 4);
		}

		pc += i * 4;
		m_selected_instruction = pc;

		// Let debugger_frame know about breakpoint.
		// Other option is to add to breakpoint manager directly and have a signal there instead.
		// Either the flow goes from debugger_list->breakpoint_manager->debugger_frame, or it goes debugger_list->debugger_frame, and I felt this was easier to read for now.
		Q_EMIT BreakpointRequested(pc);
	}
}

void debugger_list::wheelEvent(QWheelEvent* event)
{
	const QPoint numSteps = event->angleDelta() / 8 / 15; // http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
	const int value = numSteps.y();
	const auto direction = (event->modifiers() == Qt::ControlModifier);

	scroll(direction ? value : -value);
}

void debugger_list::resizeEvent(QResizeEvent* event)
{
	QListWidget::resizeEvent(event);

	if (count() < 1 || visualItemRect(item(0)).height() < 1)
	{
		return;
	}

	const u32 old_size = m_item_count;

	// It is fine if the QWidgetList is a tad bit larger than the frame
	m_item_count = utils::aligned_div<u32>(rect().height() - frameWidth() * 2, visualItemRect(item(0)).height());

	if (old_size <= m_item_count)
	{
		for (u32 i = old_size; i < m_item_count; ++i)
		{
			insertItem(i, new QListWidgetItem(""));
			m_dirty_flag = true;
		}
	}
	else
	{
		for (u32 i = old_size - 1; i >= m_item_count; --i)
		{
			delete takeItem(i);
		}
	}
}
