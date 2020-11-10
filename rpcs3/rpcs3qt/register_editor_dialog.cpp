#include "register_editor_dialog.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Memory/vm_reservation.h"

#include "Emu/Cell/lv2/sys_ppu_thread.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <charconv>

constexpr auto qstr = QString::fromStdString;
inline std::string sstr(const QString& _in) { return _in.toStdString(); }
inline std::string sstr(const QVariant& _in) { return sstr(_in.toString()); }

enum registers : int
{
	ppu_r0,
	ppu_r31 = ppu_r0 + 31,
	ppu_f0,
	ppu_f31 = ppu_f0 + 31,
	ppu_ff0,
	ppu_ff31 = ppu_ff0 + 31,
	ppu_v0,
	ppu_v31 = ppu_v0 + 31,
	spu_r0 = ::align(ppu_v31 + 1u, 128),
	spu_r127 = spu_r0 + 127,
	PPU_CR,
	PPU_LR,
	PPU_CTR,
	PPU_XER,
	PPU_VSCR,
	PPU_PRIO,
	PPU_PRIO2, // sys_mutex special priority protocol stuff
	PPU_FPSCR,
	PPU_VRSAVE,
	MFC_PEVENTS,
	MFC_EVENTS_MASK,
	MFC_EVENTS_COUNT,
	MFC_TAG_UPD,
	MFC_TAG_MASK,
	MFC_ATOMIC_STAT,
	SPU_SRR0,
	SPU_SNR1,
	SPU_SNR2,
	SPU_OUT_MBOX,
	SPU_OUT_INTR_MBOX,
	SPU_FPSCR,
	RESERVATION_LOST,
	PC,
};

register_editor_dialog::register_editor_dialog(QWidget *parent, u32 _pc, const std::shared_ptr<cpu_thread>& _cpu, CPUDisAsm* _disasm)
	: QDialog(parent)
	, m_pc(_pc)
	, m_disasm(_disasm)
	, cpu(_cpu)
{
	setWindowTitle(tr("Edit registers"));
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout* vbox_panel = new QVBoxLayout();
	QHBoxLayout* hbox_panel = new QHBoxLayout();
	QVBoxLayout* vbox_left_panel = new QVBoxLayout();
	QVBoxLayout* vbox_right_panel = new QVBoxLayout();
	QHBoxLayout* hbox_button_panel = new QHBoxLayout();

	QLabel* t1_text = new QLabel(tr("Register:     "), this);
	QLabel* t2_text = new QLabel(tr("Value (Hex):"), this);

	QPushButton* button_ok = new QPushButton(tr("&Ok"));
	QPushButton* button_cancel = new QPushButton(tr("&Cancel"));
	button_ok->setFixedWidth(80);
	button_cancel->setFixedWidth(80);

	m_register_combo = new QComboBox(this);
	m_value_line = new QLineEdit(this);
	m_value_line->setFixedWidth(200);

	// Layouts
	vbox_left_panel->addWidget(t1_text);
	vbox_left_panel->addWidget(t2_text);

	vbox_right_panel->addWidget(m_register_combo);
	vbox_right_panel->addWidget(m_value_line);

	hbox_button_panel->addWidget(button_ok);
	hbox_button_panel->addWidget(button_cancel);
	hbox_button_panel->setAlignment(Qt::AlignCenter);

	if (1)
	{
		if (_cpu->id_type() == 1)
		{
			for (int i = ppu_r0; i <= ppu_r31; i++) m_register_combo->addItem(qstr(fmt::format("r%d", i % 32)), i);
			for (int i = ppu_f0; i <= ppu_f31; i++) m_register_combo->addItem(qstr(fmt::format("f%d", i % 32)), i);
			for (int i = ppu_ff0; i <= ppu_ff31; i++) m_register_combo->addItem(qstr(fmt::format("ff%d", i % 32)), i);
			for (int i = ppu_v0; i <= ppu_v31; i++) m_register_combo->addItem(qstr(fmt::format("v%d", i % 32)), i);
			m_register_combo->addItem("CR", +PPU_CR);
			m_register_combo->addItem("LR", +PPU_LR);
			m_register_combo->addItem("CTR", PPU_CTR);
			m_register_combo->addItem("VRSAVE", +PPU_VRSAVE);
			//m_register_combo->addItem("XER", +PPU_XER);
			//m_register_combo->addItem("FPSCR", +PPU_FPSCR);
			//m_register_combo->addItem("VSCR", +PPU_VSCR);
			m_register_combo->addItem("Priority", +PPU_PRIO);
			//m_register_combo->addItem("Priority 2", +PPU_PRIO2);
		}
		else if (_cpu->id_type() == 2)
		{
			for (int i = spu_r0; i <= spu_r127; i++) m_register_combo->addItem(qstr(fmt::format("r%d", i % 128)), i);
			m_register_combo->addItem("MFC Pending Events", +MFC_PEVENTS);
			m_register_combo->addItem("MFC Events Mask", +MFC_EVENTS_MASK);
			m_register_combo->addItem("MFC Events Count", +MFC_EVENTS_COUNT);
			m_register_combo->addItem("MFC Tag Mask", +MFC_TAG_MASK);
			//m_register_combo->addItem("MFC Tag Update", +MFC_TAG_UPD);
			//m_register_combo->addItem("MFC Atomic Status", +MFC_ATOMIC_STAT);
			m_register_combo->addItem("SPU SNR1", +SPU_SNR1);
			m_register_combo->addItem("SPU SNR2", +SPU_SNR2);
			m_register_combo->addItem("SPU Out Mailbox", +SPU_OUT_MBOX);
			m_register_combo->addItem("SPU Out-Intr Mailbox", +SPU_OUT_INTR_MBOX);
			m_register_combo->addItem("SRR0", +SPU_SRR0);
		}

		m_register_combo->addItem("Reservation Clear", +RESERVATION_LOST);
		m_register_combo->addItem("PC", +PC);
	}

	// Main Layout
	hbox_panel->addLayout(vbox_left_panel);
	hbox_panel->addSpacing(10);
	hbox_panel->addLayout(vbox_right_panel);
	vbox_panel->addLayout(hbox_panel);
	vbox_panel->addSpacing(10);
	vbox_panel->addLayout(hbox_button_panel);
	setLayout(vbox_panel);
	setModal(true);

	// Events
	connect(button_ok, &QAbstractButton::clicked, this, [=, this](){OnOkay(_cpu); accept();});
	connect(button_cancel, &QAbstractButton::clicked, this, &register_editor_dialog::reject);
	connect(m_register_combo, &QComboBox::currentTextChanged, this, [this](const QString&)
	{
		if (auto qvar = m_register_combo->currentData(); qvar.canConvert(QMetaType::Int))
		{
			updateRegister(qvar.toInt());
		}
	});

	updateRegister(m_register_combo->currentData().toInt());
}

void register_editor_dialog::updateRegister(int reg)
{
	const auto cpu = this->cpu.lock();
	std::string str = sstr(tr("Error parsing register value!"));

	if (!cpu)
	{
	}
	else if (cpu->id_type() == 1)
	{
		const auto& ppu = *static_cast<const ppu_thread*>(cpu.get());

		if (reg >= ppu_r0 && reg <= ppu_v31)
		{
			const u32 reg_index = reg % 32;

			if (reg >= ppu_r0 && reg <= ppu_r31) str = fmt::format("%016llx", ppu.gpr[reg_index]);
			else if (reg >= ppu_ff0 && reg <= ppu_ff31) str = fmt::format("%g", ppu.fpr[reg_index]);
			else if (reg >= ppu_f0 && reg <= ppu_f31) str = fmt::format("%016llx", std::bit_cast<u64>(ppu.fpr[reg_index]));
			else if (reg >= ppu_v0 && reg <= ppu_v31)  str = fmt::format("%016llx%016llx", ppu.vr[reg_index]._u64[1], ppu.vr[reg_index]._u64[0]);
		}
		else if (reg == PPU_CR)  str = fmt::format("%08x", ppu.cr.pack());
		else if (reg == PPU_LR)  str = fmt::format("%016llx", ppu.lr);
		else if (reg == PPU_CTR) str = fmt::format("%016llx", ppu.ctr);
		else if (reg == PPU_VRSAVE) str = fmt::format("%08x", ppu.vrsave);
		else if (reg == PPU_PRIO) str = fmt::format("%08x", +ppu.prio);
		else if (reg == RESERVATION_LOST) str = sstr(ppu.raddr ? tr("Lose reservation on OK") : tr("Reservation is inactive"));
		else if (reg == PC) str = fmt::format("%08x", ppu.cia);
	}
	else if (cpu->id_type() == 2)
	{
		const auto& spu = *static_cast<const spu_thread*>(cpu.get());

		if (reg >= spu_r0 && reg <= spu_r127)
		{
			const u32 reg_index = reg % 128;
			str = fmt::format("%016llx%016llx", spu.gpr[reg_index]._u64[1], spu.gpr[reg_index]._u64[0]);
		}
		else if (reg == MFC_PEVENTS) str = fmt::format("%08x", +spu.ch_events.load().events);
		else if (reg == MFC_EVENTS_MASK) str = fmt::format("%08x", +spu.ch_events.load().mask);
		else if (reg == MFC_EVENTS_COUNT) str = fmt::format("%u", +spu.ch_events.load().count);
		else if (reg == MFC_TAG_MASK) str = fmt::format("%08x", spu.ch_tag_mask);
		else if (reg == SPU_SRR0) str = fmt::format("%08x", spu.srr0);
		else if (reg == SPU_SNR1) str = fmt::format("%s", spu.ch_snr1);
		else if (reg == SPU_SNR2) str = fmt::format("%s", spu.ch_snr2);
		else if (reg == SPU_OUT_MBOX) str = fmt::format("%s", spu.ch_out_mbox);
		else if (reg == SPU_OUT_INTR_MBOX) str = fmt::format("%s", spu.ch_out_intr_mbox);
		else if (reg == RESERVATION_LOST) str = sstr(spu.raddr ? tr("Lose reservation on OK") : tr("Reservation is inactive"));
		else if (reg == PC) str = fmt::format("%08x", spu.pc);
	}

	m_value_line->setText(qstr(str));
}

void register_editor_dialog::OnOkay(const std::shared_ptr<cpu_thread>& _cpu)
{
	const auto cpu = _cpu.get();

	const int reg = m_register_combo->currentData().toInt();
	std::string value = sstr(m_value_line->text());

	auto check_res = [](std::from_chars_result res, const char* end)
	{
		return res.ec == std::errc() && res.ptr == end;
	};

	if (auto len = value.size(); len < 32 && !(reg >= ppu_ff0 && reg <= ppu_ff31))
	{
		const bool is_int = !value.empty() && std::isxdigit(value[0]);

		if (value.starts_with("0x") || value.starts_with("0X"))
		{
			value = value.substr(2);
		}

		if (is_int)
		{
			value.insert(0, 32 - len, '0');
		}
	}
	else if (value.size() > 32u && !(reg >= ppu_ff0 && reg <= ppu_ff31))
	{
		// Invalid integer
		value.clear();
	}

	if (!cpu || value.empty())
	{
	}
	else if (cpu->id_type() == 1)
	{
		auto& ppu = *static_cast<ppu_thread*>(cpu);

		if (reg >= ppu_r0 && reg <= ppu_v31)
		{
			const u32 reg_index = reg % 32;

			if ((reg >= ppu_r0 && reg <= ppu_r31) || (reg >= ppu_f0 && reg <= ppu_f31))
			{
				if (u64 reg_value; check_res(std::from_chars(value.c_str() + 16, value.c_str() + 32, reg_value, 16), value.c_str() + 32))
				{
					if (reg >= ppu_r0 && reg <= ppu_r31) ppu.gpr[reg_index] = reg_value;
					else if (reg >= ppu_f0 && reg <= ppu_f31) ppu.fpr[reg_index] = std::bit_cast<f64>(reg_value);
					return;
				}
			}
			else if (reg >= ppu_ff0 && reg <= ppu_ff31)
			{
				char* end{};
				if (double reg_value = std::strtod(value.c_str(), &end); end != value.c_str())
				{
					ppu.fpr[reg_index] = static_cast<f64>(reg_value);
					return;
				}
			}
			else if (reg >= ppu_v0 && reg <= ppu_v31)
			{
				u64 reg_value0, reg_value1;

				if (check_res(std::from_chars(value.c_str(), value.c_str() + 16, reg_value0, 16), value.c_str() + 16) &&
					check_res(std::from_chars(value.c_str() + 16, value.c_str() + 32, reg_value1, 16), value.c_str() + 32))
				{
					ppu.vr[reg_index] = v128::from64(reg_value0, reg_value1);
					return;
				}
			}
		}
		else if (reg == PPU_LR || reg == PPU_CTR)
		{
			if (u64 reg_value; check_res(std::from_chars(value.c_str() + 16, value.c_str() + 32, reg_value, 16), value.c_str() + 32))
			{
				if (reg == PPU_LR) ppu.lr = reg_value;
				else if (reg == PPU_CTR) ppu.ctr = reg_value;
				return;
			}
		}
		else if (reg == PPU_CR || reg == PPU_VRSAVE || reg == PPU_PRIO || reg == PC)
		{
			if (u32 reg_value; check_res(std::from_chars(value.c_str() + 24, value.c_str() + 32, reg_value, 16), value.c_str() + 32))
			{
				bool ok = true;
				if (reg == PPU_CR) ppu.cr.unpack(reg_value);
				else if (reg == PPU_VRSAVE) ppu.vrsave = reg_value;
				else if (reg == PPU_PRIO && !sys_ppu_thread_set_priority(ppu, ppu.id, reg_value)) {}
				else if (reg == PC && reg_value % 4 == 0 && vm::check_addr(reg_value, vm::page_executable)) ppu.cia = reg_value & -4;
				else ok = false;
				if (ok) return;
			}
		}
		else if (reg == RESERVATION_LOST)
		{
			if (u32 raddr = ppu.raddr) vm::reservation_update(raddr);
			return;
		}
	}
	else if (cpu->id_type() == 2)
	{
		auto& spu = *static_cast<spu_thread*>(cpu);

		if (reg >= spu_r0 && reg <= spu_r127)
		{
			const u32 reg_index = reg % 128;
			u64 reg_value0, reg_value1;

			if (check_res(std::from_chars(value.c_str(), value.c_str() + 16, reg_value0, 16), value.c_str() + 16) &&
				check_res(std::from_chars(value.c_str() + 16, value.c_str() + 32, reg_value1, 16), value.c_str() + 32))
			{
				spu.gpr[reg_index] = v128::from64(reg_value0, reg_value1);
				return;
			}
		}
		else if (reg == MFC_PEVENTS || reg == MFC_EVENTS_MASK || reg == MFC_TAG_MASK || reg == SPU_SRR0 || reg == PC)
		{
			if (u32 reg_value; check_res(std::from_chars(value.c_str() + 24, value.c_str() + 32, reg_value, 16), value.c_str() + 32))
			{
				bool ok = true;
				if (reg == MFC_PEVENTS && !(reg_value & ~SPU_EVENT_IMPLEMENTED)) spu.ch_events.atomic_op([&](typename spu_thread::ch_events_t& events){ events.events = reg_value; });
				else if (reg == MFC_EVENTS_MASK && !(reg_value & ~SPU_EVENT_IMPLEMENTED)) spu.ch_events.atomic_op([&](typename spu_thread::ch_events_t& events){ events.mask = reg_value; });
				else if (reg == MFC_EVENTS_COUNT && reg_value <= 1u) spu.ch_events.atomic_op([&](typename spu_thread::ch_events_t& events){ events.count = reg_value; });
				else if (reg == MFC_TAG_MASK) spu.ch_tag_mask = reg_value;
				else if (reg == SPU_SRR0 && !(reg_value & ~0x3fffc)) spu.srr0 = reg_value;
				else if (reg == PC && !(reg_value & ~0x3fffc)) spu.pc = reg_value;
				else ok = false;

				if (ok) return;
			}
		}
		else if (reg == SPU_SNR1 || reg == SPU_SNR2 || reg == SPU_OUT_MBOX || reg == SPU_OUT_INTR_MBOX)
		{
			const bool count = (value != "empty");
			if (u32 reg_value = 0;
				!count || check_res(std::from_chars(value.c_str() + 24, value.c_str() + 32, reg_value, 16), value.c_str() + 32))
			{
				if (reg == SPU_SNR1) spu.ch_snr1.set_value(reg_value, count);
				else if (reg == SPU_SNR2) spu.ch_snr2.set_value(reg_value, count);
				else if (reg == SPU_OUT_MBOX) spu.ch_out_mbox.set_value(reg_value, count);
				else if (reg == SPU_OUT_INTR_MBOX) spu.ch_out_intr_mbox.set_value(reg_value, count);
				return;
			}
		}
		else if (reg == RESERVATION_LOST)
		{
			if (u32 raddr = spu.raddr) vm::reservation_update(raddr);
			return;
		}
	}

	QMessageBox::critical(this, tr("Error"), tr("This value could not be converted.\nNo changes were made."));
}
