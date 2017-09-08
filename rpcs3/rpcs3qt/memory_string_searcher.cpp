
#include "memory_string_searcher.h"

memory_string_searcher::memory_string_searcher(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("String Searcher"));
	setAttribute(Qt::WA_DeleteOnClose);
	setFixedSize(QSize(545, 64));

	m_addr_line = new QLineEdit(this);
	m_addr_line->setFixedWidth(460);
	m_addr_line->setPlaceholderText(tr("Search..."));

	QPushButton* button_search = new QPushButton(tr("&Search"), this);
	button_search->setFixedWidth(60);

	QHBoxLayout* hbox_panel = new QHBoxLayout();
	hbox_panel->addWidget(m_addr_line);
	hbox_panel->addWidget(button_search);

	setLayout(hbox_panel);

	connect(button_search, &QAbstractButton::clicked, this, &memory_string_searcher::OnSearch);
};

void memory_string_searcher::OnSearch()
{
	const QString wstr = m_addr_line->text();
	const char *str = wstr.toStdString().c_str();
	const u32 len = wstr.length();

	LOG_NOTICE(GENERAL, "Searching for string %s", str);

	// Search the address space for the string
	u32 strIndex = 0;
	u32 numFound = 0;
	const auto area = vm::get(vm::main);
	for (u32 addr = area->addr; addr < area->addr + area->size; addr++)
	{
		if (!vm::check_addr(addr))
		{
			strIndex = 0;
			continue;
		}

		u8 byte = vm::read8(addr);
		if (byte == str[strIndex])
		{
			if (strIndex == len)
			{
				// Found it
				LOG_NOTICE(GENERAL, "Found @ %04x", addr - len);
				numFound++;
				strIndex = 0;
				continue;
			}

			strIndex++;
		}
		else
		{
			strIndex = 0;
		}

		if (addr % (1024 * 1024 * 64) == 0) // Log every 64mb
		{
			LOG_NOTICE(GENERAL, "Searching %04x ...", addr);
		}
	}

	LOG_NOTICE(GENERAL, "Search completed (found %d matches)", numFound);
}
