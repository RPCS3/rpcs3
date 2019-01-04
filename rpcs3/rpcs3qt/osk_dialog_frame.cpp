
#include "osk_dialog_frame.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>

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

void osk_dialog_frame::Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit)
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

	// Text Input
	QLineEdit* input = new QLineEdit(m_dialog);
	input->setFixedWidth(lineEditWidth());
	input->setMaxLength(charlimit);
	input->setText(QString::fromStdU16String(std::u16string(init_text)));
	input->setFocus();

	// Text Input Counter
	QLabel* inputCount = new QLabel(QString("%1/%2").arg(input->text().length()).arg(charlimit));

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
	inputLayout->addWidget(input);
	inputLayout->addWidget(inputCount);

	QFormLayout* layout = new QFormLayout(m_dialog);
	layout->setFormAlignment(Qt::AlignHCenter);
	layout->addRow(message_label);
	layout->addRow(inputLayout);
	layout->addRow(buttonsLayout);
	m_dialog->setLayout(layout);

	// Events
	connect(input, &QLineEdit::textChanged, [=](const QString& text)
	{
		inputCount->setText(QString("%1/%2").arg(text.length()).arg(charlimit));
		on_osk_input_entered();
	});

	connect(input, &QLineEdit::returnPressed, m_dialog, &QDialog::accept);
	connect(button_ok, &QAbstractButton::clicked, m_dialog, &QDialog::accept);

	connect(m_dialog, &QDialog::accepted, [=]
	{
		std::memcpy(osk_text, reinterpret_cast<const char16_t*>(input->text().constData()), ((input->text()).size() + 1) * sizeof(char16_t));
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
