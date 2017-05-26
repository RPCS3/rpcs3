
#include "msg_dialog_frame.h"

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

void msg_dialog_frame::Create(const std::string& msg)
{
	if (m_dialog)
	{
		m_dialog->close();
		delete m_dialog;
	}

	m_dialog = new custom_dialog(type.disable_cancel);

	m_text = new QLabel(qstr(msg));

	m_dialog->setWindowTitle(type.se_normal ? "Normal dialog" : "Error dialog");
	m_dialog->setWindowFlags(m_dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);
	m_dialog->setWindowOpacity(type.bg_invisible ? 1.0 : 192.0 / 255.0);

	//Layout
	m_layout = new QFormLayout(m_dialog);
	m_layout->setFormAlignment(Qt::AlignHCenter);
	m_layout->addRow(m_text);

	if (type.progress_bar_count >= 2)
	{
		m_gauge2 = new QProgressBar(m_dialog);
		m_gauge2->setRange(0, m_gauge_max);
		m_gauge2->setFixedWidth(300);
		m_gauge2->setAlignment(Qt::AlignCenter);
		m_hBoxLayoutG2 = new QHBoxLayout;
		m_hBoxLayoutG2->addStretch();
		m_hBoxLayoutG2->addWidget(m_gauge2);
		m_hBoxLayoutG2->addStretch();
		m_text2 = new QLabel("", m_dialog);
	}

	if (type.progress_bar_count >= 1)
	{
		m_gauge1 = new QProgressBar(m_dialog);
		m_gauge1->setRange(0, m_gauge_max);
		m_gauge1->setFixedWidth(300);
		m_gauge1->setAlignment(Qt::AlignCenter);
		m_hBoxLayoutG1 = new QHBoxLayout;
		m_hBoxLayoutG1->addStretch();
		m_hBoxLayoutG1->addWidget(m_gauge1);
		m_hBoxLayoutG1->addStretch();
		m_text1 = new QLabel("", m_dialog);
	}

	if (m_gauge1)
	{
		m_hBoxLayout1 = new QHBoxLayout;
		m_hBoxLayout1->setAlignment(Qt::AlignCenter);
		m_hBoxLayout1->addWidget(m_text1);
		m_layout->addRow(m_hBoxLayout1);
		m_layout->addRow(m_hBoxLayoutG1);

#ifdef _WIN32
		m_tb_button = new QWinTaskbarButton();
		m_tb_button->setWindow(m_taskbarTarget);
		m_tb_progress = m_tb_button->progress();
		m_tb_progress->setRange(0, m_gauge_max);
		m_tb_progress->setVisible(true);
#endif
	}

	if (m_gauge2)
	{
		m_hBoxLayout2 = new QHBoxLayout;
		m_hBoxLayout2->setAlignment(Qt::AlignCenter);
		m_hBoxLayout2->addWidget(m_text2);
		m_layout->addRow(m_hBoxLayout2);
		m_layout->addRow(m_hBoxLayoutG2);
	}

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO)
	{
		m_dialog->setModal(true);

		m_button_yes = new QPushButton("&Yes", m_dialog);
		m_button_no = new QPushButton("&No", m_dialog);

		m_hBoxLayout3 = new QHBoxLayout;
		m_hBoxLayout3->setAlignment(Qt::AlignCenter);
		m_hBoxLayout3->addWidget(m_button_yes);
		m_hBoxLayout3->addWidget(m_button_no);
		m_layout->addRow(m_hBoxLayout3);

		if (type.default_cursor == 1)
		{
			m_button_no->setFocus();
		}
		else
		{
			m_button_yes->setFocus();
		}
		connect(m_button_yes, &QAbstractButton::clicked, [=] {on_close(CELL_MSGDIALOG_BUTTON_YES); m_dialog->accept(); });
		connect(m_button_no, &QAbstractButton::clicked, [=] {on_close(CELL_MSGDIALOG_BUTTON_NO); m_dialog->accept(); });
	}

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK)
	{
		m_dialog->setModal(true);

		m_button_ok = new QPushButton("&OK", m_dialog);
		m_button_ok->setFixedWidth(50);

		m_hBoxLayout3 = new QHBoxLayout;
		m_hBoxLayout3->setAlignment(Qt::AlignCenter);
		m_hBoxLayout3->addWidget(m_button_ok);
		m_layout->addRow(m_hBoxLayout3);

		if (type.default_cursor == 0)
		{
			m_button_ok->setFocus();
		}
		connect(m_button_ok, &QAbstractButton::clicked, [=] { on_close(CELL_MSGDIALOG_BUTTON_OK); m_dialog->accept(); });
	}

	m_dialog->setLayout(m_layout);

	connect(m_dialog, &QDialog::rejected, [=] {if (!type.disable_cancel) { on_close(CELL_MSGDIALOG_BUTTON_ESCAPE); }});

	//Fix size
	m_dialog->setFixedSize(m_dialog->sizeHint());
	m_dialog->show();
}

void msg_dialog_frame::CreateOsk(const std::string& msg, char16_t* osk_text)
{
	if (osk_dialog)
	{
		osk_dialog->close();
		delete osk_dialog;
	}

	osk_dialog = new custom_dialog(type.disable_cancel);
	osk_dialog->setModal(true);
	osk_text_return = osk_text;

	//Title
	osk_dialog->setWindowTitle(qstr(msg));
	osk_dialog->setWindowFlags(osk_dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);

	//Text Input
	osk_input = new QLineEdit(osk_dialog);
	osk_input->setFixedWidth(200);
	osk_input->setFocus();

	//Ok Button
	osk_button_ok = new QPushButton("Ok", osk_dialog);
	osk_button_ok->setFixedWidth(50);

	//Layout
	osk_hBoxLayout = new QHBoxLayout;
	osk_hBoxLayout->setAlignment(Qt::AlignCenter);
	osk_hBoxLayout->addWidget(osk_button_ok);

	osk_layout = new QFormLayout(osk_dialog);
	osk_layout->setFormAlignment(Qt::AlignHCenter);
	osk_layout->addRow(osk_input);
	osk_layout->addRow(osk_hBoxLayout);
	osk_dialog->setLayout(osk_layout);

	//Events
	connect(osk_input, &QLineEdit::textChanged, [=] {
		std::memcpy(osk_text_return, reinterpret_cast<const char16_t*>(osk_input->text().constData()), osk_input->text().size() * 2);
		on_osk_input_entered();
	});
	connect(osk_input, &QLineEdit::returnPressed, [=] { on_close(CELL_MSGDIALOG_BUTTON_OK); osk_dialog->accept(); });
	connect(osk_button_ok, &QAbstractButton::clicked, [=] { on_close(CELL_MSGDIALOG_BUTTON_OK); osk_dialog->accept(); });
	connect(osk_dialog, &QDialog::rejected, [=] {if (!type.disable_cancel) { on_close(CELL_MSGDIALOG_BUTTON_ESCAPE); }});

	//Fix size
	osk_dialog->setFixedSize(osk_dialog->sizeHint());
	osk_dialog->show();
}

msg_dialog_frame::msg_dialog_frame(QWindow* taskbarTarget) : m_taskbarTarget(taskbarTarget) {}

msg_dialog_frame::~msg_dialog_frame() {
#ifdef _WIN32
	if (m_tb_progress)
	{
		m_tb_progress->hide();
	}
	if (m_tb_button)
	{
		m_tb_button->deleteLater();
	}
#endif
	if (m_dialog)
	{
		m_dialog->deleteLater();
	}
}

void msg_dialog_frame::ProgressBarSetMsg(u32 index, const std::string& msg)
{
	if (m_dialog)
	{
		if (index == 0 && m_text1) m_text1->setText(qstr(msg));
		if (index == 1 && m_text2) m_text2->setText(qstr(msg));
	}
}

void msg_dialog_frame::ProgressBarReset(u32 index)
{
	if (index == 0 && m_gauge1)
	{
		m_gauge1->reset();
#ifdef _WIN32
		m_tb_progress->reset();
#endif
	}

	if (index == 1 && m_gauge2)
	{
		m_gauge2->reset();
	}
}

void msg_dialog_frame::ProgressBarInc(u32 index, u32 delta)
{
	if (m_dialog)
	{
		if (index == 0 && m_gauge1)
		{
			m_gauge1->setValue(m_gauge1->value() + delta);
#ifdef _WIN32
			m_tb_progress->setValue(m_tb_progress->value() + delta);
#endif
		}

		if (index == 1 && m_gauge2)
		{
			m_gauge2->setValue(m_gauge2->value() + delta);
		}
	}
}
