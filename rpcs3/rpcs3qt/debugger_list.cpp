#include "debugger_list.h"
#include "gui_settings.h"
#include "qt_utils.h"
#include "breakpoint_handler.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/RSX/RSXDisAsm.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/System.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QLabel>

#include <memory>

constexpr auto qstr = QString::fromStdString;

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
}

void debugger_list::UpdateCPUData(cpu_thread* cpu, CPUDisAsm* disasm)
{
	if (m_cpu != cpu)
	{
		m_cpu = cpu;
		m_selected_instruction = -1;
	}

	m_disasm = disasm;
}

u32 debugger_list::GetCenteredAddress(u32 address) const
{
	return address - ((m_item_count / 2) * 4);
}

void debugger_list::ShowAddress(u32 addr, bool select_addr, bool force)
{
	const decltype(spu_thread::local_breakpoints)* spu_bps_list;

	if (m_cpu && m_cpu->id_type() == 2)
	{
		spu_bps_list = &static_cast<spu_thread*>(m_cpu)->local_breakpoints;
	}

	auto IsBreakpoint = [&](u32 pc)
	{
		switch (m_cpu ? m_cpu->id_type() : 0)
		{
		case 1: return m_ppu_breakpoint_handler->HasBreakpoint(pc);
		case 2: return (*spu_bps_list)[pc / 4].load();
		default: return false;
		}
	};

	bool center_pc = m_gui_settings->GetValue(gui::d_centerPC).toBool();

	// How many spaces addr can move down without us needing to move the entire view
	const u32 addr_margin = (m_item_count / (center_pc ? 2 : 1) - 4); // 4 is just a buffer of 4 spaces at the bottom

	if (select_addr || force)
	{
		// The user wants to survey a specific memory location, do not interfere from this point forth 
		m_follow_thread = false;
	}

	if (force || ((m_follow_thread || select_addr) && addr - m_pc > addr_margin * 4)) // 4 is the number of bytes in each instruction
	{
		if (center_pc)
		{
			m_pc = GetCenteredAddress(addr);
		}
		else
		{
			m_pc = addr;
		}
	}

	const auto& default_foreground = palette().color(foregroundRole());
	const auto& default_background = palette().color(backgroundRole());

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

	if (!m_cpu || !m_disasm || +m_cpu->state + cpu_flag::exit + cpu_flag::wait == +m_cpu->state)
	{
		for (uint i = 0; i < m_item_count; ++i)
		{
			QListWidgetItem* list_item = item(i);
			list_item->setText(qstr(fmt::format("   [%08x]  ?? ?? ?? ??:", 0)));
			list_item->setForeground(default_foreground);
			list_item->setBackground(default_background);
		}
	}
	else
	{
		const bool is_spu = m_cpu->id_type() == 2;
		const u32 address_limits = (is_spu ? 0x3fffc : ~3);
		m_pc &= address_limits;
		u32 pc = m_pc;

		for (uint i = 0, count = 4; i < m_item_count; ++i, pc = (pc + count) & address_limits)
		{
			QListWidgetItem* list_item = item(i);

			if (m_cpu->is_paused() && pc == m_cpu->get_pc())
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

			if (m_cpu->id_type() == 1 && !vm::check_addr(pc, 0))
			{
				list_item->setText((IsBreakpoint(pc) ? ">> " : "   ") + qstr(fmt::format("[%08x]  ?? ?? ?? ??:", pc)));
				count = 4;
				continue;
			}

			if (m_cpu->id_type() == 1 && !vm::check_addr(pc, vm::page_executable))
			{
				const u32 data = *vm::get_super_ptr<atomic_be_t<u32>>(pc);
				list_item->setText((IsBreakpoint(pc) ? ">> " : "   ") + qstr(fmt::format("[%08x]  %02x %02x %02x %02x:", pc,
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
				list_item->setText((IsBreakpoint(pc) ? ">> " : "   ") + qstr(fmt::format("[%08x] ???     ?? ??", pc)));
				count = 4;
				continue;
			}

			list_item->setText((IsBreakpoint(pc) ? ">> " : "   ") + qstr(m_disasm->last_opcode));
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
	while (m_cpu && m_cpu->id_type() == 0x55 && steps > 0)
	{
		// If scrolling forwards (downwards), we can skip entire commands
		// Backwards is impossible though
		m_pc += std::max<u32>(m_disasm->disasm(m_pc), 4);
		steps--;
	}

	if (m_cpu && m_cpu->id_type() == 0x55 && steps < 0)
	{
		// If scrolling backwards (upwards), try to obtain the start of commands tail 
		if (auto [count, res] = static_cast<rsx::thread*>(m_cpu)->try_get_pc_of_x_cmds_backwards(-steps, m_pc); count == 0u - steps)
		{
			steps = 0;
			m_pc = res;
		}
	}

	EnableThreadFollowing(false);
	ShowAddress(m_pc + (steps * 4), false, true);
}

void debugger_list::keyPressEvent(QKeyEvent* event)
{
	if (!isActiveWindow())
	{
		return;
	}

	switch (event->key())
	{
	case Qt::Key_PageUp:   scroll(0 - m_item_count); return;
	case Qt::Key_PageDown: scroll(m_item_count); return;
	case Qt::Key_Up:       scroll(1); return;
	case Qt::Key_Down:     scroll(-1); return;
	case Qt::Key_I:
	{
		if (event->isAutoRepeat())
		{
			return;
		}
		if (m_cpu && m_cpu->id_type() == 0x55)
		{
			create_rsx_command_detail(m_pc, currentRow());
			return;
		}
		return;
	}
	default: break;
	}
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

void debugger_list::create_rsx_command_detail(u32 pc, int row)
{
	if (row < 0)
	{
		return;
	}

	for (; row > 0; row--)
	{
		// Skip methods
		pc += std::max<u32>(m_disasm->disasm(pc), 4);
	}

	RSXDisAsm rsx_dis = *static_cast<RSXDisAsm*>(m_disasm);
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
		const int i = currentRow();
		if (i < 0) return;

		const u32 pc = m_pc + i * 4;

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
