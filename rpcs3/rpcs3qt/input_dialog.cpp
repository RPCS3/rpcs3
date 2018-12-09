#include "input_dialog.h"

#include <QHBoxLayout>
#include <QDialogButtonBox>

input_dialog::input_dialog(int max_length, const QString& text, const QString& title, const QString& label, const QString& placeholder, QWidget *parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	setWindowTitle(title);

	QLabel* m_label = new QLabel(label);

	QLineEdit* m_input = new QLineEdit();
	m_input->setPlaceholderText(placeholder);
	m_input->setText(text);
	m_input->setMaxLength(max_length);
	m_input->setClearButtonEnabled(true);
	connect(m_input, &QLineEdit::textChanged, this, &input_dialog::text_changed);

	QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(m_label);
	layout->addWidget(m_input);
	layout->addWidget(button_box);
	setLayout(layout);

	setFixedHeight(sizeHint().height());
}

input_dialog::~input_dialog(){}
