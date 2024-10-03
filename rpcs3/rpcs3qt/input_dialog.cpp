#include "input_dialog.h"
#include "qt_utils.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QValidator>
#include <QLabel>

input_dialog::input_dialog(int max_length, const QString& text, const QString& title, const QString& label, const QString& placeholder, QWidget *parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	setWindowTitle(title);

	m_label = new QLabel(label);

	m_input = new QLineEdit();
	m_input->setPlaceholderText(placeholder);
	m_input->setText(text);
	m_input->setMaxLength(max_length);
	m_input->setClearButtonEnabled(true);
	connect(m_input, &QLineEdit::textChanged, this, &input_dialog::text_changed);

	m_button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(m_button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(m_label);
	layout->addWidget(m_input);
	layout->addWidget(m_button_box);
	setLayout(layout);

	setFixedHeight(sizeHint().height());
}

input_dialog::~input_dialog()
{
}

void input_dialog::set_clear_button_enabled(bool enabled) const
{
	m_input->setClearButtonEnabled(enabled);
}

void input_dialog::set_input_font(const QFont& font, bool fix_width, char sample) const
{
	if (const int max = m_input->maxLength(); max > 0 && fix_width && std::isprint(static_cast<uchar>(sample)))
	{
		const QString str = QString(max, sample);
		m_input->setFixedWidth(gui::utils::get_label_width(str, &font));
	}

	m_input->setFont(font);
}

void input_dialog::set_validator(const QValidator* validator) const
{
	m_input->setValidator(validator);
}

void input_dialog::set_button_enabled(QDialogButtonBox::StandardButton id, bool enabled) const
{
	if (QPushButton* button = m_button_box->button(id))
	{
		button->setEnabled(enabled);
	}
}

void input_dialog::set_label_text(const QString& text) const
{
	m_label->setText(text);
}

QString input_dialog::get_input_text() const
{
	return m_input->text();
}
