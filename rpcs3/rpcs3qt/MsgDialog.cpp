
#include "MsgDialog.h"

void MsgDialogFrame::Create(const std::string& msg)
{
	//QWidget* parent = TheApp->m_MainWindow; // TODO

	m_dialog = new QDialog();

	m_text = new QLabel(QString::fromStdString(msg).toUtf8());

	m_dialog->setWindowTitle(type.se_normal ? "Normal dialog" : "Error dialog");
	m_dialog->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	m_dialog->setWindowOpacity(type.bg_invisible ? 1.0 : 192.0 / 255.0);

	//Layout
	m_layout = new QFormLayout(m_dialog);
	m_layout->setFormAlignment(Qt::AlignHCenter);
	m_layout->addRow(m_text);

	if (type.progress_bar_count >= 2)
	{
		m_gauge2 = new QProgressBar(m_dialog);
		m_gauge2->setMaximum(100);
		m_gauge2->setFixedWidth(300);
		m_text2 = new QLabel("", m_dialog);
	}

	if (type.progress_bar_count >= 1)
	{
		m_gauge1 = new QProgressBar(m_dialog);
		m_gauge1->setMaximum(100);
		m_gauge1->setFixedWidth(300);
		m_text1 = new QLabel("", m_dialog);
	}

	if (m_gauge1)
	{
		m_hBoxLayout1 = new QHBoxLayout;
		m_hBoxLayout1->setAlignment(Qt::AlignCenter);
		m_hBoxLayout1->addWidget(m_text1);
		m_hBoxLayout1->addWidget(m_gauge1);
		m_layout->addRow(m_hBoxLayout1);
		m_gauge1->setValue(0);
	}

	if (m_gauge2)
	{
		m_hBoxLayout2 = new QHBoxLayout;
		m_hBoxLayout2->setAlignment(Qt::AlignCenter);
		m_hBoxLayout2->addWidget(m_text2);
		m_hBoxLayout2->addWidget(m_gauge2);
		m_layout->addRow(m_hBoxLayout2);
		m_gauge2->setValue(0);
	}

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO)
	{
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
		connect(m_button_yes, &QAbstractButton::clicked, m_dialog, [this] {on_close(CELL_MSGDIALOG_BUTTON_YES); });
		connect(m_button_no, &QAbstractButton::clicked, m_dialog, [this] {on_close(CELL_MSGDIALOG_BUTTON_NO); });
	}

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK)
	{
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
	}

	m_dialog->setLayout(m_layout);

	//Fix size
	m_dialog->setFixedSize(m_dialog->sizeHint());
	m_dialog->exec();
}

void MsgDialogFrame::CreateOsk(const std::string& msg, char16_t* osk_text)
{
	osk_dialog = new QInputDialog();
	osk_text_return = osk_text;

	//Title
	osk_dialog->setWindowTitle(QString::fromStdString(msg));
	osk_dialog->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	//Text Input
	osk_input = new QLineEdit(osk_dialog);
	osk_input->setFocus();
	connect(osk_input, &QLineEdit::textChanged, this, [this] {std::memcpy(osk_text_return, reinterpret_cast<const char16_t*>(osk_input->text().constData()), osk_input->text().size() * 2); on_osk_input_entered(); });

	//Ok Button
	osk_button_ok = new QPushButton("Ok", osk_dialog);
	osk_button_ok->setFixedWidth(50);
	connect(osk_button_ok, &QAbstractButton::clicked, osk_dialog, [this] {on_close(CELL_MSGDIALOG_BUTTON_OK); });

	//Layout
	osk_hBoxLayout = new QHBoxLayout;
	osk_hBoxLayout->setAlignment(Qt::AlignCenter);
	osk_hBoxLayout->addWidget(osk_button_ok);

	osk_layout = new QFormLayout(osk_dialog);
	osk_layout->setFormAlignment(Qt::AlignHCenter);
	osk_layout->addRow(osk_input);
	osk_layout->addRow(osk_hBoxLayout);
	setLayout(osk_layout);

	//Fix size
	osk_dialog->setFixedSize(osk_dialog->sizeHint());
	osk_dialog->exec();
}

MsgDialogFrame::MsgDialogFrame(){}

void MsgDialogFrame::ProgressBarSetMsg(u32 index, const std::string& msg)
{
	if (m_dialog)
	{
		if (index == 0 && m_text1) m_text1->setText(QString::fromStdString(msg.c_str()).toUtf8());
		if (index == 1 && m_text2) m_text2->setText(QString::fromStdString(msg.c_str()).toUtf8());
	}
}

void MsgDialogFrame::ProgressBarReset(u32 index)
{
	if (m_dialog)
	{
		if (index == 0 && m_gauge1) m_gauge1->setValue(0);
		if (index == 1 && m_gauge2) m_gauge2->setValue(0);
	}
}

void MsgDialogFrame::ProgressBarInc(u32 index, u32 delta)
{
	if (m_dialog)
	{
		if (index == 0 && m_gauge1) m_gauge1->setValue(m_gauge1->value() + delta);
		if (index == 1 && m_gauge2) m_gauge2->setValue(m_gauge2->value() + delta);
	}
}

void MsgDialogFrame::closeEvent(QCloseEvent *event)
{
	if (!type.disable_cancel)
	{
		on_close(CELL_MSGDIALOG_BUTTON_ESCAPE);
	}
}
