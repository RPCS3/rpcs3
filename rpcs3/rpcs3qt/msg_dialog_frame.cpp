
#include "msg_dialog_frame.h"

#include <QApplication>
#include <QScreen>
#include <QThread>

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

void msg_dialog_frame::Create(const std::string& msg)
{
	static int dialog_nr;
	static int dialog_count;
	++dialog_count;

	if (m_dialog)
	{
		m_dialog->close();
		delete m_dialog;
	}

	m_dialog = new custom_dialog(type.disable_cancel);
	m_dialog->setWindowTitle(type.se_normal ? "Normal dialog" : "Error dialog");
	m_dialog->setWindowFlags(m_dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);
	m_dialog->setWindowOpacity(type.bg_invisible ? 1.0 : 192.0 / 255.0);

	m_text = new QLabel(qstr(msg));
	m_text->setAlignment(Qt::AlignCenter);

	//Layout
	QFormLayout* layout = new QFormLayout(m_dialog);
	layout->setFormAlignment(Qt::AlignHCenter);
	layout->addRow(m_text);

	auto l_AddGauge = [=] (QProgressBar* &bar, QLabel* &text)
	{
		text = new QLabel("", m_dialog);
		bar = new QProgressBar(m_dialog);
		bar->setRange(0, m_gauge_max);
		bar->setFixedWidth(300);
		bar->setAlignment(Qt::AlignCenter);

		QHBoxLayout* barLayout = new QHBoxLayout;
		barLayout->addStretch();
		barLayout->addWidget(bar);
		barLayout->addStretch();

		QHBoxLayout* textLayout = new QHBoxLayout;
		textLayout->setAlignment(Qt::AlignCenter);
		textLayout->addWidget(text);

		layout->addRow(textLayout);
		layout->addRow(barLayout);
	};

	if (type.progress_bar_count >= 1)
	{
		l_AddGauge(m_gauge1, m_text1);

#ifdef _WIN32
		m_tb_button = new QWinTaskbarButton();
		m_tb_progress = m_tb_button->progress();
		m_tb_progress->setRange(0, m_gauge_max);
		m_tb_progress->setVisible(true);
#endif
	}

	if (type.progress_bar_count >= 2)
	{
		l_AddGauge(m_gauge2, m_text2);
	}

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO)
	{
		m_dialog->setModal(true);

		m_button_yes = new QPushButton("&Yes", m_dialog);
		m_button_no = new QPushButton("&No", m_dialog);

		QHBoxLayout* hBoxButtons = new QHBoxLayout;
		hBoxButtons->setAlignment(Qt::AlignCenter);
		hBoxButtons->addWidget(m_button_yes);
		hBoxButtons->addWidget(m_button_no);
		layout->addRow(hBoxButtons);

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

		QHBoxLayout* hBoxButtons = new QHBoxLayout;
		hBoxButtons->setAlignment(Qt::AlignCenter);
		hBoxButtons->addWidget(m_button_ok);
		layout->addRow(hBoxButtons);

		if (type.default_cursor == 0)
		{
			m_button_ok->setFocus();
		}
		connect(m_button_ok, &QAbstractButton::clicked, [=] { on_close(CELL_MSGDIALOG_BUTTON_OK); m_dialog->accept(); });
	}

	m_dialog->setLayout(layout);

	connect(m_dialog, &QDialog::rejected, [=] {if (!type.disable_cancel) { on_close(CELL_MSGDIALOG_BUTTON_ESCAPE); }});
	connect(m_dialog, &QDialog::destroyed, [=] {if (--dialog_count <= 0) dialog_nr = 0;});

	//Fix size
	m_dialog->setFixedSize(m_dialog->sizeHint());

	// order compile-dialogs on screen
	if (qstr(msg).contains("Compiling PPU module"))
	{
		const QSize screensize = QApplication::primaryScreen()->geometry().size();
		const double ratio = (double)screensize.width() / (double)screensize.height();
		const int thread_count = QThread::idealThreadCount();
		const int min_x = screensize.width() / 10;
		const int max_x = screensize.width() - min_x;
		const int min_y = screensize.height() / 10;
		const int max_y = screensize.height() - min_y;
		const int s_width = 30 + m_dialog->frameSize().width();
		const int s_height = 60 + m_dialog->frameSize().height();
		const int max_dialogs_x = std::max(1, (max_x - min_x) / s_width);
		const int max_dialogs_y = std::max(1, (max_y - min_y) / s_height);
		int dialogs_x = std::min(max_dialogs_x, thread_count);
		int dialogs_y = std::min(max_dialogs_y, (int)((double)thread_count / (double)max_dialogs_x + 0.5));
		const int dialogs = std::min(dialogs_x * dialogs_y, thread_count);

		// resize to better aspect ratio
		while ((double)dialogs_x / (double)dialogs_y > ratio)
		{
			--dialogs_x;
			while (dialogs_x * dialogs_y < dialogs)
			{
				++dialogs_y;
			}
		}

		const int origin_x = (screensize.width() - dialogs_x * s_width) / 2;
		const int origin_y = (screensize.height() - dialogs_y * s_height) / 2;

		// create positions
		QList<QPoint> location;
		for (int y = 0; y < dialogs_y; y++)
		{
			for (int x = 0; x < dialogs_x; x++)
			{
				location.append(QPoint(origin_x + x * s_width, origin_y + y * s_height));
			}
		}

		// create offset positions for moar threads than your window can handle
		if (dialogs_x * dialogs_y < thread_count)
		{
			for (int y = 0; y < dialogs_y; y++)
			{
				for (int x = 0; x < dialogs_x; x++)
				{
					location.append(QPoint(origin_x + s_width / 2 + x * s_width, origin_y + s_height / 2 + y * s_height));
				}
			}
		}

		// move to position
		m_dialog->move(location.at(dialog_nr));

		// reset dialog_nr if needed
		if (++dialog_nr >= location.count()) dialog_nr = 0;
	}
	else
	{
		// we assume compilation is over, so reset dialog_nr
		dialog_nr = 0;
	}

	m_dialog->show();

#ifdef _WIN32
	// if we do this before, the QWinTaskbarProgress won't show
	if (m_tb_button) m_tb_button->setWindow(m_dialog->windowHandle());
#endif
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
	QLineEdit* input = new QLineEdit(osk_dialog);
	input->setFixedWidth(200);
	input->setFocus();

	//Ok Button
	QPushButton* button_ok = new QPushButton("Ok", osk_dialog);
	button_ok->setFixedWidth(50);

	//Layout
	QHBoxLayout* buttonsLayout = new QHBoxLayout;
	buttonsLayout->setAlignment(Qt::AlignCenter);
	buttonsLayout->addWidget(button_ok);

	QFormLayout* layout = new QFormLayout(osk_dialog);
	layout->setFormAlignment(Qt::AlignHCenter);
	layout->addRow(input);
	layout->addRow(buttonsLayout);
	osk_dialog->setLayout(layout);

	//Events
	connect(input, &QLineEdit::textChanged, [=] {
		std::memcpy(osk_text_return, reinterpret_cast<const char16_t*>(input->text().constData()), input->text().size() * 2);
		on_osk_input_entered();
	});
	connect(input, &QLineEdit::returnPressed, [=] { on_close(CELL_MSGDIALOG_BUTTON_OK); osk_dialog->accept(); });
	connect(button_ok, &QAbstractButton::clicked, [=] { on_close(CELL_MSGDIALOG_BUTTON_OK); osk_dialog->accept(); });
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
