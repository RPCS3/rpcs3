#include "debugger_list.h"
#include "register_editor_dialog.h"
#include "instruction_editor_dialog.h"

#include <QApplication>
#include <QWheelEvent>
#include <QMouseEvent>

constexpr auto qstr = QString::fromStdString;

debugger_list::debugger_list(QWidget* parent, breakpoint_handler* handler)
	: QListWidget(parent), m_brkpoint_handler(handler)
{
	m_pc = 0;
	m_item_count = 30;
};

u32 debugger_list::GetPc() const
{
	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		return 0;
	}

	return cpu->id_type() == 1 ? static_cast<ppu_thread*>(cpu.get())->cia : static_cast<SPUThread*>(cpu.get())->pc;
}

void debugger_list::ShowAddr(u32 addr)
{
	m_pc = addr;

	const auto cpu = this->cpu.lock();

	if (!cpu)
	{
		for (uint i = 0; i<m_item_count; ++i, m_pc += 4)
		{
			item(i)->setText(qstr(fmt::format("[%08x] illegal address", m_pc)));
		}
	}
	else
	{
		bool hasBreakpoint = m_brkpoint_handler->HasBreakpoint(m_pc);
		const bool is_spu = cpu->id_type() != 1;
		const u32 cpu_offset = is_spu ? static_cast<SPUThread&>(*cpu).offset : 0;
		const u32 address_limits = is_spu ? 0x3ffff : ~0;
		m_pc &= address_limits;
		m_disasm->offset = (u8*)vm::base(cpu_offset);
		for (uint i = 0, count = 4; i<m_item_count; ++i, m_pc = (m_pc + count) & address_limits)
		{
			if (!vm::check_addr(cpu_offset + m_pc, 4))
			{
				item(i)->setText((hasBreakpoint ? ">>> " : "    ") + qstr(fmt::format("[%08x] illegal address", m_pc)));
				count = 4;
				continue;
			}

			count = m_disasm->disasm(m_disasm->dump_pc = m_pc);

			item(i)->setText((hasBreakpoint ? ">>> " : "    ") + qstr(m_disasm->last_opcode));

			if (test(cpu->state & cpu_state_pause) && m_pc == GetPc())
			{
				item(i)->setTextColor(m_text_color_pc);
				item(i)->setBackgroundColor(m_color_pc);
			}
			else if (hasBreakpoint)
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

void debugger_list::UpdateCPUData(std::weak_ptr<cpu_thread> cpu, std::shared_ptr<CPUDisAsm> disasm)
{
	this->cpu = cpu;
	m_disasm = disasm;
}

void debugger_list::keyPressEvent(QKeyEvent* event)
{
	if (!isActiveWindow())
	{
		return;
	}

	const auto cpu = this->cpu.lock();
	long i = currentRow();

	if (i < 0 || !cpu)
	{
		return;
	}

	const u32 start_pc = m_pc - m_item_count * 4;
	const u32 pc = start_pc + i * 4;

	if (event->key() == Qt::Key_Space && QApplication::keyboardModifiers() & Qt::ControlModifier)
	{
		Q_EMIT RequestCPUStep();
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
		//ConLog.Write("pc=0x%llx", pc);

		// Let debugger_frame know about breakpoint.
		Q_EMIT BreakpointRequested(pc);

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

	ShowAddr(m_pc - m_item_count * 4);
}
