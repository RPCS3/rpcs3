#include "msg_dialog_frame.h"
#include "custom_dialog.h"

#include <QPushButton>
#include <QFormLayout>

#ifdef _WIN32
#include <QWinTHumbnailToolbar>
#include <QWinTHumbnailToolbutton>
#elif HAVE_QTDBUS
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#endif

constexpr auto qstr = QString::fromStdString;

void msg_dialog_frame::Create(const std::string& msg, const std::string& title)
{
	state = MsgDialogState::Open;

	static const auto& barWidth = [](){return QLabel("This is the very length of the progressbar due to hidpi reasons.").sizeHint().width();};

	Close(true);

	m_dialog = new custom_dialog(type.disable_cancel);
	m_dialog->setWindowTitle(title.empty() ? (type.se_normal ? "Normal dialog" : "Error dialog") : qstr(title));
	m_dialog->setWindowOpacity(type.bg_invisible ? 1. : 0.75);

	m_text = new QLabel(qstr(msg));
	m_text->setAlignment(Qt::AlignCenter);

	// Layout
	QFormLayout* layout = new QFormLayout(m_dialog);
	layout->setFormAlignment(Qt::AlignHCenter);
	layout->addRow(m_text);

	auto l_AddGauge = [=, this](QProgressBar* &bar, QLabel* &text)
	{
		text = new QLabel("", m_dialog);
		bar = new QProgressBar(m_dialog);
		bar->setRange(0, 100);
		bar->setValue(0);
		bar->setFixedWidth(barWidth());
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
		m_tb_progress->setRange(0, 100);
		m_tb_progress->setVisible(true);
#elif HAVE_QTDBUS
		UpdateProgress(0);
		m_progress_value = 0;
#endif
	}

	if (type.progress_bar_count >= 2)
	{
		l_AddGauge(m_gauge2, m_text2);
	}

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO)
	{
		m_dialog->setModal(true);

		QPushButton* m_button_yes = new QPushButton("&Yes", m_dialog);
		QPushButton* m_button_no = new QPushButton("&No", m_dialog);

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

		connect(m_button_yes, &QAbstractButton::clicked, [this]()
		{
			g_last_user_response = CELL_MSGDIALOG_BUTTON_YES;
			on_close(CELL_MSGDIALOG_BUTTON_YES);
			m_dialog->accept();
		});

		connect(m_button_no, &QAbstractButton::clicked, [this]()
		{
			g_last_user_response = CELL_MSGDIALOG_BUTTON_NO;
			on_close(CELL_MSGDIALOG_BUTTON_NO);
			m_dialog->accept();
		});
	}

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK)
	{
		m_dialog->setModal(true);

		QPushButton* m_button_ok = new QPushButton("&OK", m_dialog);
		m_button_ok->setFixedWidth(50);

		QHBoxLayout* hBoxButtons = new QHBoxLayout;
		hBoxButtons->setAlignment(Qt::AlignCenter);
		hBoxButtons->addWidget(m_button_ok);
		layout->addRow(hBoxButtons);

		if (type.default_cursor == 0)
		{
			m_button_ok->setFocus();
		}

		connect(m_button_ok, &QAbstractButton::clicked, [this]()
		{
			g_last_user_response = CELL_MSGDIALOG_BUTTON_OK;
			on_close(CELL_MSGDIALOG_BUTTON_OK);
			m_dialog->accept();
		});
	}

	m_dialog->setLayout(layout);

	connect(m_dialog, &QDialog::rejected, [this]()
	{
		if (!type.disable_cancel)
		{
			g_last_user_response = CELL_MSGDIALOG_BUTTON_ESCAPE;
			on_close(CELL_MSGDIALOG_BUTTON_ESCAPE);
		}
	});

	// Fix size
	m_dialog->layout()->setSizeConstraint(QLayout::SetFixedSize);
	m_dialog->show();

#ifdef _WIN32
	// if we do this before, the QWinTaskbarProgress won't show
	if (m_tb_button) m_tb_button->setWindow(m_dialog->windowHandle());
#endif
}

void msg_dialog_frame::Close(bool success)
{
	if (m_dialog)
	{
		m_dialog->done(success ? QDialog::Accepted : QDialog::Rejected);
		m_dialog->deleteLater();
	}
}

msg_dialog_frame::msg_dialog_frame() {}

msg_dialog_frame::~msg_dialog_frame()
{
#ifdef _WIN32
	if (m_tb_progress)
	{
		m_tb_progress->hide();
	}
	if (m_tb_button)
	{
		m_tb_button->deleteLater();
	}
#elif HAVE_QTDBUS
	UpdateProgress(0, false);
#endif

	if (m_dialog)
	{
		m_dialog->deleteLater();
	}
}

void msg_dialog_frame::SetMsg(const std::string& msg)
{
	if (m_dialog)
	{
		m_text->setText(qstr(msg));
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
	if (!m_dialog)
	{
		return;
	}

	if (index == 0 && m_gauge1)
	{
		m_gauge1->setValue(0);
	}

	if (index == 1 && m_gauge2)
	{
		m_gauge2->setValue(0);
	}

	if (index == taskbar_index + 0u)
	{
#ifdef _WIN32
		if (m_tb_progress)
		{
			m_tb_progress->reset();
		}
#elif HAVE_QTDBUS
		UpdateProgress(0);
#endif
	}
}

void msg_dialog_frame::ProgressBarInc(u32 index, u32 delta)
{
	if (!m_dialog)
	{
		return;
	}

	if (index == 0 && m_gauge1)
	{
		m_gauge1->setValue(std::min(m_gauge1->value() + static_cast<int>(delta), m_gauge1->maximum()));
	}

	if (index == 1 && m_gauge2)
	{
		m_gauge2->setValue(std::min(m_gauge2->value() + static_cast<int>(delta), m_gauge2->maximum()));
	}

	if (index == taskbar_index + 0u || taskbar_index == -1)
	{
#ifdef _WIN32
		if (m_tb_progress)
		{
			m_tb_progress->setValue(std::min(m_tb_progress->value() + static_cast<int>(delta), m_tb_progress->maximum()));
		}
#elif HAVE_QTDBUS
		m_progress_value = std::min(m_progress_value + static_cast<int>(delta), m_gauge_max);
		UpdateProgress(m_progress_value);
#endif
	}
}

void msg_dialog_frame::ProgressBarSetLimit(u32 index, u32 limit)
{
	if (!m_dialog)
	{
		return;
	}

	if (index == 0 && m_gauge1)
	{
		m_gauge1->setMaximum(limit);
	}

	if (index == 1 && m_gauge2)
	{
		m_gauge2->setMaximum(limit);
	}

	bool set_taskbar_limit = false;

	if (index == taskbar_index + 0u)
	{
		m_gauge_max = limit;
		set_taskbar_limit = true;
	}
	else if (taskbar_index == -1)
	{
		m_gauge_max += limit;
		set_taskbar_limit = true;
	}

#ifdef _WIN32
	if (set_taskbar_limit && m_tb_progress)
	{
		m_tb_progress->setMaximum(m_gauge_max);
	}
#endif
}

#ifdef HAVE_QTDBUS
void msg_dialog_frame::UpdateProgress(int progress, bool disable)
{
	QDBusMessage message = QDBusMessage::createSignal
	(
		QStringLiteral("/"),
		QStringLiteral("com.canonical.Unity.LauncherEntry"),
		QStringLiteral("Update")
	);
	QVariantMap properties;
	if (disable)
		properties.insert(QStringLiteral("progress-visible"), false);
	else
		properties.insert(QStringLiteral("progress-visible"), true);
	// Progress takes a value from 0.0 to 0.1
	properties.insert(QStringLiteral("progress"), 1.* progress / m_gauge_max);
	message << QStringLiteral("application://rpcs3.desktop") << properties;
	QDBusConnection::sessionBus().send(message);
}
#endif
