#include "stdafx.h"
#include "basic_mouse_settings_dialog.h"
#include "localized_emu.h"
#include "Input/basic_mouse_handler.h"
#include "Input/keyboard_pad_handler.h"
#include "Emu/Io/mouse_config.h"
#include "util/asm.hpp"

#include <QGroupBox>
#include <QMessageBox>
#include <QVBoxLayout>

LOG_CHANNEL(cfg_log, "CFG");

enum button_role
{
	button_name = Qt::UserRole,
	button_code
};

basic_mouse_settings_dialog::basic_mouse_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	setObjectName("basic_mouse_settings_dialog");
	setWindowTitle(tr("Configure Basic Mouse Handler"));
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);
	setModal(true);

	QVBoxLayout* v_layout = new QVBoxLayout(this);

	m_button_box = new QDialogButtonBox(this);
	m_button_box->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);

	connect(m_button_box, &QDialogButtonBox::clicked, this, [this](QAbstractButton* button)
	{
		if (button == m_button_box->button(QDialogButtonBox::Apply))
		{
			g_cfg_mouse.save();
		}
		else if (button == m_button_box->button(QDialogButtonBox::Save))
		{
			g_cfg_mouse.save();
			accept();
		}
		else if (button == m_button_box->button(QDialogButtonBox::RestoreDefaults))
		{
			if (QMessageBox::question(this, tr("Confirm Reset"), tr("Reset all settings?")) != QMessageBox::Yes)
				return;
			reset_config();
		}
		else if (button == m_button_box->button(QDialogButtonBox::Cancel))
		{
			// Restore config
			if (!g_cfg_mouse.load())
			{
				cfg_log.notice("Could not restore mouse config. Using defaults.");
			}
			reject();
		}
	});

	if (!g_cfg_mouse.load())
	{
		cfg_log.notice("Could not load basic mouse config. Using defaults.");
	}

	m_buttons = new QButtonGroup(this);

	connect(m_buttons, &QButtonGroup::idClicked, this, &basic_mouse_settings_dialog::on_button_click);

	connect(&m_remap_timer, &QTimer::timeout, this, [this]()
	{
		auto button = m_buttons->button(m_button_id);

		if (--m_seconds <= 0)
		{
			if (button)
			{
				if (const int button_id = m_buttons->id(button))
				{
					const std::string name = g_cfg_mouse.get_button(button_id).to_string();
					button->setText(name.empty() ? QStringLiteral("-") : QString::fromStdString(name));
				}
			}
			reactivate_buttons();
			return;
		}
		if (button)
		{
			button->setText(tr("[ Waiting %1 ]").arg(m_seconds));
		}
	});

	const auto insert_button = [this](int id, QPushButton* button)
	{
		m_buttons->addButton(button, id);
		button->installEventFilter(this);
	};

	constexpr u32 button_count = 8;
	constexpr u32 max_items_per_column = 4;
	int rows = button_count;

	for (u32 cols = 1; utils::aligned_div(button_count, cols) > max_items_per_column;)
	{
		rows = utils::aligned_div(button_count, ++cols);
	}

	QWidget* widget = new QWidget(this);
	QGridLayout* grid_layout = new QGridLayout(this);

	for (int i = 0, row = 0, col = 0; i < static_cast<int>(button_count); i++, row++)
	{
		const int cell_code = get_mouse_button_code(i);
		const QString translated_cell_button = localized_emu::translated_mouse_button(cell_code);

		QHBoxLayout* h_layout = new QHBoxLayout(this);
		QGroupBox* gb = new QGroupBox(translated_cell_button, this);
		QPushButton* pb = new QPushButton(this);

		insert_button(cell_code, pb);

		const std::string saved_btn = g_cfg_mouse.get_button(cell_code);

		pb->setText(saved_btn.empty() ? QStringLiteral("-") : QString::fromStdString(saved_btn));

		if (row >= rows)
		{
			row = 0;
			col++;
		}

		m_push_buttons[cell_code] = pb;
		h_layout->addWidget(pb);
		gb->setLayout(h_layout);
		grid_layout->addWidget(gb, row, col);
	}

	widget->setLayout(grid_layout);

	v_layout->addWidget(widget);
	v_layout->addWidget(m_button_box);
	setLayout(v_layout);

	m_palette = m_push_buttons[CELL_MOUSE_BUTTON_1]->palette(); // save normal palette
}

void basic_mouse_settings_dialog::reset_config()
{
	g_cfg_mouse.from_default();

	for (auto& [cell_code, pb] : m_push_buttons)
	{
		if (!pb)
			continue;

		const QString text = QString::fromStdString(g_cfg_mouse.get_button(cell_code).def);
		pb->setText(text.isEmpty() ? QStringLiteral("-") : text);
	}
}

void basic_mouse_settings_dialog::on_button_click(int id)
{
	if (id < 0)
	{
		return;
	}

	for (auto but : m_buttons->buttons())
	{
		but->setEnabled(false);
		but->setFocusPolicy(Qt::ClickFocus);
	}

	m_button_box->setEnabled(false);
	for (auto but : m_button_box->buttons())
	{
		but->setFocusPolicy(Qt::ClickFocus);
	}

	m_button_id = id;
	if (auto button = m_buttons->button(m_button_id))
	{
		button->setText(tr("[ Waiting %1 ]").arg(MAX_SECONDS));
		button->setPalette(QPalette(Qt::blue));
		button->grabMouse();
	}

	m_remap_timer.start(1000);
}

void basic_mouse_settings_dialog::keyPressEvent(QKeyEvent* event)
{
	if (m_button_id < 0)
	{
		// We are not remapping a button, so pass the event to the base class.
		QDialog::keyPressEvent(event);
		return;
	}

	const std::string name = keyboard_pad_handler::GetKeyName(event, false);
	g_cfg_mouse.get_button(m_button_id).from_string(name);

	if (auto button = m_buttons->button(m_button_id))
	{
		button->setText(QString::fromStdString(name));
	}

	reactivate_buttons();
}

void basic_mouse_settings_dialog::mouseReleaseEvent(QMouseEvent* event)
{
	if (m_button_id < 0)
	{
		// We are not remapping a button, so pass the event to the base class.
		QDialog::mouseReleaseEvent(event);
		return;
	}

	const std::string name = keyboard_pad_handler::GetMouseName(event);
	g_cfg_mouse.get_button(m_button_id).from_string(name);

	if (auto button = m_buttons->button(m_button_id))
	{
		button->setText(QString::fromStdString(name));
	}

	reactivate_buttons();
}

bool basic_mouse_settings_dialog::eventFilter(QObject* object, QEvent* event)
{
	switch (event->type())
	{
	case QEvent::MouseButtonRelease:
	{
		// On right click clear binding if we are not remapping pad button
		if (m_button_id < 0)
		{
			QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
			if (const auto button = qobject_cast<QPushButton*>(object); button && button->isEnabled() && mouse_event->button() == Qt::RightButton)
			{
				if (const int button_id = m_buttons->id(button))
				{
					button->setText(QStringLiteral("-"));
					g_cfg_mouse.get_button(button_id).from_string("");
					return true;
				}
			}
		}

		// Disabled buttons should not absorb mouseclicks
		event->ignore();
		break;
	}
	default:
	{
		break;
	}
	}

	return QDialog::eventFilter(object, event);
}

void basic_mouse_settings_dialog::reactivate_buttons()
{
	m_remap_timer.stop();
	m_seconds = MAX_SECONDS;

	if (m_button_id < 0)
	{
		return;
	}

	if (auto button = m_buttons->button(m_button_id))
	{
		button->setPalette(m_palette);
		button->releaseMouse();
	}

	m_button_id = -1;

	// Enable all buttons
	m_button_box->setEnabled(true);

	for (auto but : m_button_box->buttons())
	{
		but->setFocusPolicy(Qt::StrongFocus);
	}

	for (auto but : m_buttons->buttons())
	{
		but->setEnabled(true);
		but->setFocusPolicy(Qt::StrongFocus);
	}
}
