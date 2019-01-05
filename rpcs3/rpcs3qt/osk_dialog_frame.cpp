
#include "osk_dialog_frame.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QRegExpValidator>

constexpr auto qstr = QString::fromStdString;

osk_dialog_frame::osk_dialog_frame()
{
}

osk_dialog_frame::~osk_dialog_frame()
{
	if (m_dialog)
	{
		m_dialog->deleteLater();
	}
}

void osk_dialog_frame::Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit, u32 options)
{
	state = OskDialogState::Open;

	static const auto& lineEditWidth = []() {return QLabel("This is the very length of the lineedit due to hidpi reasons.").sizeHint().width(); };

	if (m_dialog)
	{
		m_dialog->close();
		delete m_dialog;
	}

	m_dialog = new custom_dialog(false);
	m_dialog->setModal(true);

	// Title
	m_dialog->setWindowTitle(qstr(title));

	// Message
	QLabel* message_label = new QLabel(QString::fromStdU16String(message));

	// Text Input Counter
	const QString text = QString::fromStdU16String(std::u16string(init_text));
	QLabel* inputCount = new QLabel(QString("%1/%2").arg(text.length()).arg(charlimit));

	// Ok Button
	QPushButton* button_ok = new QPushButton("Ok", m_dialog);

	// Button Layout
	QHBoxLayout* buttonsLayout = new QHBoxLayout;
	buttonsLayout->setAlignment(Qt::AlignCenter);
	buttonsLayout->addStretch();
	buttonsLayout->addWidget(button_ok);
	buttonsLayout->addStretch();

	// Input Layout
	QHBoxLayout* inputLayout = new QHBoxLayout;
	inputLayout->setAlignment(Qt::AlignHCenter);

	// Text Input
	if (options & CELL_OSKDIALOG_NO_RETURN)
	{
		QLineEdit* input = new QLineEdit(m_dialog);
		input->setFixedWidth(lineEditWidth());
		input->setMaxLength(charlimit);
		input->setText(text);
		input->setFocus();

		if (options & CELL_OSKDIALOG_NO_SPACE)
		{
			input->setValidator(new QRegExpValidator(QRegExp("^\S*$"), this));
		}

		connect(input, &QLineEdit::textChanged, [=](const QString& text)
		{
			inputCount->setText(QString("%1/%2").arg(text.length()).arg(charlimit));
			SetOskText(text);
			on_osk_input_entered();
		});
		connect(input, &QLineEdit::returnPressed, m_dialog, &QDialog::accept);

		inputLayout->addWidget(input);
	}
	else
	{
		QTextEdit* input = new QTextEdit(m_dialog);
		input->setFixedWidth(lineEditWidth());
		input->setText(text);
		input->setFocus();

		connect(input, &QTextEdit::textChanged, [=]()
		{
			QString text = input->toPlainText();
			text.chop(text.length() - charlimit);

			if (options & CELL_OSKDIALOG_NO_SPACE)
			{
				text.remove(QRegExp("\\s+"));
			}

			input->setPlainText(text);

			QTextCursor cursor = input->textCursor();
			cursor.setPosition(QTextCursor::End);
			input->setTextCursor(cursor);

			inputCount->setText(QString("%1/%2").arg(text.length()).arg(charlimit));
			SetOskText(text);

			on_osk_input_entered();
		});

		inputLayout->addWidget(input);
	}

	inputLayout->addWidget(inputCount);

	QFormLayout* layout = new QFormLayout(m_dialog);
	layout->setFormAlignment(Qt::AlignHCenter);
	layout->addRow(message_label);
	layout->addRow(inputLayout);
	layout->addRow(buttonsLayout);
	m_dialog->setLayout(layout);

	// Events
	connect(button_ok, &QAbstractButton::clicked, m_dialog, &QDialog::accept);

	connect(m_dialog, &QDialog::accepted, [=]
	{
		on_close(CELL_MSGDIALOG_BUTTON_OK);
	});

	connect(m_dialog, &QDialog::rejected, [=]
	{
		on_close(CELL_MSGDIALOG_BUTTON_ESCAPE);
	});

	// Fix size
	m_dialog->layout()->setSizeConstraint(QLayout::SetFixedSize);
	m_dialog->show();
}

void osk_dialog_frame::SetOskText(const QString& text)
{
	std::memcpy(osk_text, reinterpret_cast<const char16_t*>(text.constData()), size_t(text.size() + 1) * sizeof(char16_t));
}

void osk_dialog_frame::Close(bool accepted)
{
	if (m_dialog)
	{
		m_dialog->done(accepted ? QDialog::Accepted : QDialog::Rejected);
	}
}
