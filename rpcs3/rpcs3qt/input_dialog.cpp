#include "input_dialog.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>

input_dialog::input_dialog(int max_length, const QString& text, const QString& title, const QString& label, const QString& placeholder, QWidget *parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	setWindowTitle(title);

	m_label = new QLabel(label);

	QLineEdit* input = new QLineEdit();
	input->setPlaceholderText(placeholder);
	input->setText(text);
	input->setMaxLength(max_length);
	input->setClearButtonEnabled(true);
	connect(input, &QLineEdit::textChanged, this, &input_dialog::text_changed);

	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(m_label);
	layout->addWidget(input);
	layout->addWidget(button_box);
	setLayout(layout);

	setFixedHeight(sizeHint().height());
}

input_dialog::~input_dialog()
{
}

void input_dialog::set_label_text(const QString& text)
{
	m_label->setText(text);
}
