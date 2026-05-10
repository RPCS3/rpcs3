#include "msg_dialog_frame.h"
#include "custom_dialog.h"

#include <QCoreApplication>
#include <QPushButton>
#include <QFormLayout>

void msg_dialog_frame::Create(const std::string& msg, const std::string& title)
{
	state = MsgDialogState::Open;

	static const auto& barWidth = [](){return QLabel("This is the very length of the progressbar due to hidpi reasons.").sizeHint().width();};

	Close(true);

	m_dialog = new custom_dialog(type.disable_cancel);
	m_dialog->setWindowTitle(title.empty() ? (type.se_normal ? tr("Normal dialog") : tr("Error dialog")) : QString::fromStdString(title));
	m_dialog->setWindowOpacity(type.bg_invisible ? 1. : 0.75);

	m_text = new QLabel(QString::fromStdString(msg));
	m_text->setAlignment(Qt::AlignCenter);

	// Layout
	QFormLayout* layout = new QFormLayout(m_dialog);
	layout->setFormAlignment(Qt::AlignHCenter);
	layout->addRow(m_text);

	auto l_AddGauge = [this, layout](QProgressBar* &bar, QLabel* &text)
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

		m_progress_indicator = std::make_unique<progress_indicator>(0, 100);
	}

	if (type.progress_bar_count >= 2)
	{
		l_AddGauge(m_gauge2, m_text2);
	}

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO)
	{
		m_dialog->setModal(true);

		QPushButton* m_button_yes = new QPushButton(tr("&Yes"), m_dialog);
		QPushButton* m_button_no = new QPushButton(tr("&No"), m_dialog);

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
			if (on_close) on_close(CELL_MSGDIALOG_BUTTON_YES);
			m_dialog->accept();
		});

		connect(m_button_no, &QAbstractButton::clicked, [this]()
		{
			if (on_close) on_close(CELL_MSGDIALOG_BUTTON_NO);
			m_dialog->accept();
		});
	}

	if (type.button_type.unshifted() == CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK)
	{
		m_dialog->setModal(true);

		QPushButton* m_button_ok = new QPushButton(tr("&OK"), m_dialog);
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
			if (on_close) on_close(CELL_MSGDIALOG_BUTTON_OK);
			m_dialog->accept();
		});
	}

	m_dialog->setLayout(layout);

	connect(m_dialog, &QDialog::rejected, [this]()
	{
		if (!type.disable_cancel)
		{
			if (on_close) on_close(CELL_MSGDIALOG_BUTTON_ESCAPE);
		}
	});

	// Fix size
	m_dialog->layout()->setSizeConstraint(QLayout::SetFixedSize);
	m_dialog->show();
}

void msg_dialog_frame::Close(bool success)
{
	if (m_dialog)
	{
		m_dialog->done(success ? QDialog::Accepted : QDialog::Rejected);
		m_dialog->deleteLater();
	}
}

msg_dialog_frame::~msg_dialog_frame()
{
	if (m_dialog)
	{
		m_dialog->deleteLater();
	}
}

void msg_dialog_frame::SetMsg(const std::string& msg)
{
	if (m_dialog)
	{
		m_text->setText(QString::fromStdString(msg));
	}
}

void msg_dialog_frame::ProgressBarSetMsg(u32 index, const std::string& msg)
{
	if (m_dialog)
	{
		if (index == 0)
		{
			if (m_text1)
			{
				m_text1->setText(QString::fromStdString(msg));
			}
		}
		else if (index == 1)
		{
			if (m_text2)
			{
				m_text2->setText(QString::fromStdString(msg));
			}
		}
	}
}

void msg_dialog_frame::ProgressBarReset(u32 index)
{
	if (!m_dialog)
	{
		return;
	}

	if (index == 0)
	{
		if (m_gauge1)
		{
			m_gauge1->setValue(0);
		}
	}
	else if (index == 1)
	{
		if (m_gauge2)
		{
			m_gauge2->setValue(0);
		}
	}

	if (index == taskbar_index + 0u)
	{
		if (m_progress_indicator)
		{
			m_progress_indicator->reset();
		}
	}
}

void msg_dialog_frame::ProgressBarInc(u32 index, u32 delta)
{
	if (!m_dialog)
	{
		return;
	}

	if (index == 0)
	{
		if (m_gauge1)
		{
			m_gauge1->setValue(std::min(m_gauge1->value() + static_cast<int>(delta), m_gauge1->maximum()));
		}
	}
	else if (index == 1)
	{
		if (m_gauge2)
		{
			m_gauge2->setValue(std::min(m_gauge2->value() + static_cast<int>(delta), m_gauge2->maximum()));
		}
	}

	if (index == taskbar_index + 0u || taskbar_index == -1)
	{
		if (m_progress_indicator)
		{
			m_progress_indicator->set_value(m_progress_indicator->value() + static_cast<int>(delta));
		}
	}
}

void msg_dialog_frame::ProgressBarSetValue(u32 index, u32 value)
{
	if (!m_dialog)
	{
		return;
	}

	if (index == 0)
	{
		if (m_gauge1)
		{
			m_gauge1->setValue(std::min(static_cast<int>(value), m_gauge1->maximum()));
		}
	}
	else if (index == 1)
	{
		if (m_gauge2)
		{
			m_gauge2->setValue(std::min(static_cast<int>(value), m_gauge2->maximum()));
		}
	}

	if (index == taskbar_index + 0u || taskbar_index == -1)
	{
		if (m_progress_indicator)
		{
			m_progress_indicator->set_value(static_cast<int>(value));
		}
	}
}

void msg_dialog_frame::ProgressBarSetLimit(u32 index, u32 limit)
{
	if (!m_dialog)
	{
		return;
	}

	if (index == 0)
	{
		if (m_gauge1)
		{
			m_gauge1->setMaximum(limit);
		}
	}
	else if (index == 1)
	{
		if (m_gauge2)
		{
			m_gauge2->setMaximum(limit);
		}
	}

	[[maybe_unused]] bool set_taskbar_limit = false;

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

	if (set_taskbar_limit && m_progress_indicator)
	{
		m_progress_indicator->set_range(0, m_gauge_max);
	}
}
