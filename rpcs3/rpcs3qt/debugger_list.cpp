#include "debugger_list.h"
#include "gui_settings.h"
#include "breakpoint_handler.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/System.h"

#include <QMouseEvent>
#include <QWheelEvent>

#include <memory>

constexpr auto qstr = QString::fromStdString;

debugger_list::debugger_list(QWidget* parent, std::shared_ptr<gui_settings> settings, breakpoint_handler* handler)
	: QListWidget(parent)
	, xgui_settings(settings)
	, m_breakpoint_handler(handler)
{
	setWindowTitle(tr("ASM"));
	for (uint i = 0; i < m_item_count; ++i)
	{
		insertItem(i, new QListWidgetItem(""));
	}
	setSizeAdjustPolicy(QListWidget::AdjustToContents);
}

void debugger_list::UpdateCPUData(std::weak_ptr<cpu_thread> cpu, std::shared_ptr<CPUDisAsm> disasm)
{
	this->cpu = cpu;
	m_disasm = disasm;
}

u32 debugger_list::GetPc() const
{
	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		return 0;
	}

	return cpu->id_type() == 1 ? static_cast<ppu_thread*>(cpu.get())->cia : static_cast<spu_thread*>(cpu.get())->pc;
}

u32 debugger_list::GetCenteredAddress(u32 address) const
{
	return address - ((m_item_count / 2) * 4);
}

void debugger_list::ShowAddress(u32 addr, bool force)
{
	auto IsBreakpoint = [this](u32 pc)
	{
		return m_breakpoint_handler->HasBreakpoint(pc);
	};

	const bool center_pc = xgui_settings->GetValue(gui::d_centerPC).toBool();

	// How many spaces addr can move down without us needing to move the entire view
	const u32 addr_margin = (m_item_count / (center_pc ? 2 : 1) - 4); // 4 is just a buffer of 4 spaces at the bottom

	if (force || addr - m_pc > addr_margin * 4) // 4 is the number of bytes in each instruction
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

	const auto cpu = this->cpu.lock();

	const auto default_foreground = palette().color(foregroundRole());
	const auto default_background = palette().color(backgroundRole());

	if (!cpu)
	{
		for (uint i = 0; i < m_item_count; ++i)
		{
			item(i)->setText(qstr(fmt::format("   [%08x]  ?? ?? ?? ??:", 0)));
			item(i)->setForeground(default_foreground);
			item(i)->setBackground(default_background);
		}
	}
	else
	{
		const bool is_spu = cpu->id_type() != 1;
		const auto cpu_offset = cpu->id_type() != 1 ? static_cast<spu_thread&>(*cpu).ls : vm::g_sudo_addr;
		const u32 address_limits = (is_spu ? 0x3fffc : ~3);
		m_pc &= address_limits;
		m_disasm->offset = cpu_offset;
		u32 pc = m_pc;

		for (uint i = 0, count = 4; i<m_item_count; ++i, pc = (pc + count) & address_limits)
		{
			if (cpu->is_paused() && pc == GetPc())
			{
				item(i)->setForeground(m_text_color_pc);
				item(i)->setBackground(m_color_pc);
			}
			else if (IsBreakpoint(pc))
			{
				item(i)->setForeground(m_text_color_bp);
				item(i)->setBackground(m_color_bp);
			}
			else
			{
				item(i)->setForeground(default_foreground);
				item(i)->setBackground(default_background);
			}

			if (!is_spu && !vm::check_addr(pc, 4))
			{
				item(i)->setText((IsBreakpoint(pc) ? ">> " : "   ") + qstr(fmt::format("[%08x]  ?? ?? ?? ??:", pc)));
				count = 4;
				continue;
			}

			if (!is_spu && !vm::check_addr(pc, 4, vm::page_executable))
			{
				const u32 data = *vm::get_super_ptr<atomic_be_t<u32>>(pc);
				item(i)->setText((IsBreakpoint(pc) ? ">> " : "   ") + qstr(fmt::format("[%08x]  %02x %02x %02x %02x:", pc,
				static_cast<u8>(data >> 24),
				static_cast<u8>(data >> 16),
				static_cast<u8>(data >> 8),
				static_cast<u8>(data >> 0))));
				count = 4;
				continue;
			}

			count = m_disasm->disasm(m_disasm->dump_pc = pc);

			item(i)->setText((IsBreakpoint(pc) ? ">> " : "   ") + qstr(m_disasm->last_opcode));
		}
	}

	setLineWidth(-1);
}

void debugger_list::keyPressEvent(QKeyEvent* event)
{
	if (!isActiveWindow() || currentRow() < 0 || !this->cpu.lock())
	{
		return;
	}

	switch (event->key())
	{
	case Qt::Key_PageUp:   ShowAddress(m_pc - (m_item_count * 2) * 4); return;
	case Qt::Key_PageDown: ShowAddress(m_pc); return;
	case Qt::Key_Up:       ShowAddress(m_pc - (m_item_count + 1) * 4); return;
	case Qt::Key_Down:     ShowAddress(m_pc - (m_item_count - 1) * 4); return;
	default: break;
	}
}

void debugger_list::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && !Emu.IsStopped() && !m_no_thread_selected)
	{
		int i = currentRow();
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
	const QPoint numSteps = event->angleDelta() / 8 / 15;	// http://doc.qt.io/qt-5/qwheelevent.html#pixelDelta
	const int value = numSteps.y();
	const auto direction = (event->modifiers() == Qt::ControlModifier);

	ShowAddress(m_pc + (direction ? value : -value) * 4, true);
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

	ShowAddress(m_pc);
}
