#include "stdafx.h"
#include "sound_effect_manager_dialog.h"

#include <QApplication>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QScreen>
#include <QStyle>
#include <QMessageBox>

LOG_CHANNEL(gui_log, "GUI");

sound_effect_manager_dialog::sound_effect_manager_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Sound Effects"));
	setAttribute(Qt::WA_DeleteOnClose);

	QLabel* description = new QLabel(tr("You can import sound effects for the RPCS3 overlays here.\nThe file format is .wav and you should try to make the sounds as short as possible."), this);

	QVBoxLayout* main_layout = new QVBoxLayout(this);
	main_layout->addWidget(description);

	const auto add_sound_widget = [this, main_layout](rsx::overlays::sound_effect sound)
	{
		ensure(!m_widgets.contains(sound));

		QString name;
		switch (sound)
		{
		case rsx::overlays::sound_effect::cursor:       name = tr("Cursor"); break;
		case rsx::overlays::sound_effect::accept:       name = tr("Accept"); break;
		case rsx::overlays::sound_effect::cancel:       name = tr("Cancel"); break;
		case rsx::overlays::sound_effect::osk_accept:   name = tr("Onscreen keyboard accept"); break;
		case rsx::overlays::sound_effect::osk_cancel:   name = tr("Onscreen keyboard cancel"); break;
		case rsx::overlays::sound_effect::dialog_ok:    name = tr("Dialog popup"); break;
		case rsx::overlays::sound_effect::dialog_error: name = tr("Error dialog popup"); break;
		case rsx::overlays::sound_effect::trophy:       name = tr("Trophy popup"); break;
		}

		QPushButton* button = new QPushButton("", this);
		connect(button, &QAbstractButton::clicked, this, [this, button, sound, name]()
		{
			const std::string path = rsx::overlays::get_sound_filepath(sound);
			if (fs::is_file(path))
			{
				if (QMessageBox::question(this, tr("Remove sound effect?"), tr("Do you really want to remove the '%0' sound effect.").arg(name)) == QMessageBox::Yes)
				{
					if (!fs::remove_file(path))
					{
						gui_log.error("Failed to remove sound effect file '%s': %s", path, fs::g_tls_error);
					}

					update_widgets();
				}
			}
			else
			{
				const QString src_path = QFileDialog::getOpenFileName(this, tr("Select Audio File to Import"), "", tr("WAV (*.wav);;"));
				if (!src_path.isEmpty())
				{
					if (!fs::copy_file(src_path.toStdString(), path, true))
					{
						gui_log.error("Failed to import sound effect file '%s' to '%s': %s", src_path, path, fs::g_tls_error);
					}
					update_widgets();
				}
			}
		});

		QPushButton* play_button = new QPushButton(this);
		play_button->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
		play_button->setIconSize(QSize(16, 16));
		play_button->setFixedSize(24, 24);
		connect(play_button, &QAbstractButton::clicked, this, [sound]()
		{
			rsx::overlays::play_sound(sound, 1.0f);
		});

		QHBoxLayout* layout = new QHBoxLayout(this);
		layout->addWidget(button);
		layout->addWidget(play_button);
		layout->addStretch(1);

		QGroupBox* gb = new QGroupBox(name, this);
		gb->setLayout(layout);

		main_layout->addWidget(gb);

		m_widgets[sound] = {
			.button = button,
			.play_button = play_button
		};
	};

	add_sound_widget(rsx::overlays::sound_effect::cursor);
	add_sound_widget(rsx::overlays::sound_effect::accept);
	add_sound_widget(rsx::overlays::sound_effect::cancel);
	add_sound_widget(rsx::overlays::sound_effect::osk_accept);
	add_sound_widget(rsx::overlays::sound_effect::osk_cancel);
	add_sound_widget(rsx::overlays::sound_effect::dialog_ok);
	add_sound_widget(rsx::overlays::sound_effect::dialog_error);
	add_sound_widget(rsx::overlays::sound_effect::trophy);

	setLayout(main_layout);
	update_widgets();
	resize(sizeHint());
}

sound_effect_manager_dialog::~sound_effect_manager_dialog()
{
}

void sound_effect_manager_dialog::update_widgets()
{
	for (auto& [sound, widget] : m_widgets)
	{
		const bool file_exists = fs::is_file(rsx::overlays::get_sound_filepath(sound));

		widget.play_button->setEnabled(file_exists);

		if (file_exists)
		{
			widget.button->setText(tr("Remove"));
			widget.button->setIcon(QApplication::style()->standardIcon(QStyle::SP_TrashIcon));
			widget.button->setIconSize(QSize(16, 16));
		}
		else
		{
			widget.button->setText(tr("Import"));
			widget.button->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
			widget.button->setIconSize(QSize(16, 16));
		}
	}
}
