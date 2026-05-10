#include "debugger_add_bp_window.h"
#include "Utilities/StrFmt.h"
#include "Utilities/StrUtil.h"
#include "breakpoint_handler.h"
#include "util/types.hpp"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>

debugger_add_bp_window::debugger_add_bp_window(breakpoint_list* bp_list, QWidget* parent)
	: QDialog(parent)
{
	ensure(bp_list);
	
	setWindowTitle(tr("Add a breakpoint"));
	setModal(true);

	QVBoxLayout* vbox_panel = new QVBoxLayout();

	QHBoxLayout* hbox_top = new QHBoxLayout();
	QLabel* l_address = new QLabel(tr("Address"));
	QLineEdit* t_address = new QLineEdit();
	t_address->setPlaceholderText(tr("Address here"));
	t_address->setFocus();

	hbox_top->addWidget(l_address);
	hbox_top->addWidget(t_address);
	vbox_panel->addLayout(hbox_top);

	QHBoxLayout* hbox_bot = new QHBoxLayout();
	QComboBox* co_bptype = new QComboBox(this);
	QStringList qstr_breakpoint_types;

	qstr_breakpoint_types
#ifdef RPCS3_HAS_MEMORY_BREAKPOINTS
		<< tr("Memory Read")
		<< tr("Memory Write")
		<< tr("Memory Read&Write")
#endif
		<< tr("Execution");

	co_bptype->addItems(qstr_breakpoint_types);

	hbox_bot->addWidget(co_bptype);
	vbox_panel->addLayout(hbox_bot);

	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	button_box->button(QDialogButtonBox::Ok)->setText(tr("Add"));

	hbox_buttons->addWidget(button_box);
	vbox_panel->addLayout(hbox_buttons);

	setLayout(vbox_panel);

	connect(button_box, &QDialogButtonBox::accepted, this, [=, this]
		{
			const std::string str_address = t_address->text().toStdString();

			if (str_address.empty())
			{
				QMessageBox::warning(this, tr("Add BP error"), tr("Address is empty!"));
				return;
			}

			// We always want hex
			const std::string parsed_string = (!str_address.starts_with("0x") && !str_address.starts_with("0X")) ? fmt::format("0x%s", str_address) : str_address;
			u64 parsed_address = 0;

			// We don't accept 0
			if (!try_to_uint64(&parsed_address, parsed_string, 1, 0xFF'FF'FF'FF))
			{
				QMessageBox::warning(this, tr("Add BP error"), tr("Address is invalid!"));
				return;
			}

			const u32 address = static_cast<u32>(parsed_address);
			bs_t<breakpoint_types> bp_t{};

#ifdef RPCS3_HAS_MEMORY_BREAKPOINTS
			switch (co_bptype->currentIndex())
			{
			case 0:
				bp_t = breakpoint_types::bp_read;
				break;
			case 1:
				bp_t = breakpoint_types::bp_write;
				break;
			case 2:
				bp_t = breakpoint_types::bp_read + breakpoint_types::bp_write;
				break;
			case 3:
				bp_t = breakpoint_types::bp_exec;
				break;
			default:
				break;
			}
#else
			bp_t = breakpoint_types::bp_exec;
#endif

			if (bp_t)
				bp_list->AddBreakpoint(address, bp_t);

			QDialog::accept();
		});

	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	move(QCursor::pos());
}
