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

void debugger_list::ShowAddress(u32 addr)
{
	auto IsBreakpoint = [this](u32 pc)
	{
		return m_breakpoint_handler->HasBreakpoint(pc);
	};

	if (xgui_settings->GetValue(gui::d_centerPC).toBool())
	{
		m_pc = GetCenteredAddress(addr);
	}
	else
	{
		m_pc = addr;
	}

	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		for (uint i = 0; i < m_item_count; ++i, m_pc += 4)
		{
			item(i)->setText(qstr(fmt::format("   [%08x] illegal address", m_pc)));
		}
	}
	else
	{
		const bool is_spu = cpu->id_type() != 1;
		const u32 cpu_offset = is_spu ? static_cast<spu_thread&>(*cpu).offset : 0;
		const u32 address_limits = (is_spu ? 0x3fffc : ~3);
		m_pc &= address_limits;
		m_disasm->offset = vm::get_super_ptr(cpu_offset);
		for (uint i = 0, count = 4; i<m_item_count; ++i, m_pc = (m_pc + count) & address_limits)
		{
			if (!vm::check_addr(cpu_offset + m_pc, 4))
			{
				item(i)->setText((IsBreakpoint(m_pc) ? ">> " : "   ") + qstr(fmt::format("[%08x] illegal address", m_pc)));
				count = 4;
				continue;
			}

			count = m_disasm->disasm(m_disasm->dump_pc = m_pc);

			item(i)->setText((IsBreakpoint(m_pc) ? ">> " : "   ") + qstr(m_disasm->last_opcode));

			if (cpu->is_paused() && m_pc == GetPc())
			{
				item(i)->setForeground(m_text_color_pc);
				item(i)->setBackground(m_color_pc);
			}
			else if (IsBreakpoint(m_pc))
			{
				item(i)->setForeground(m_text_color_bp);
				item(i)->setBackground(m_color_bp);
			}
			else
			{
				item(i)->setForeground(palette().color(foregroundRole()));
				item(i)->setBackground(palette().color(backgroundRole()));
			}
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

		const u32 start_pc = m_pc - m_item_count * 4;
		const u32 pc = start_pc + i * 4;

		// Let debugger_frame know about breakpoint.
		// Other option is to add to breakpoint manager directly and have a signal there instead.
		// Either the flow goes from debugger_list->breakpoint_manager->debugger_frame, or it goes debugger_list->debugger_frame, and I felt this was easier to read for now.
		Q_EMIT BreakpointRequested(pc);

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
