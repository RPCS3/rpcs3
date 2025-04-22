#include "music_player_dialog.h"
#include "ui_music_player_dialog.h"
#include "qt_music_handler.h"
#include "Emu/System.h"
#include "Emu/VFS.h"

#include <QFileDialog>
#include <QSlider>

music_player_dialog::music_player_dialog(QWidget* parent)
	: QDialog(parent), ui(new Ui::music_player_dialog)
{
	setAttribute(Qt::WA_DeleteOnClose);

	ui->setupUi(this);

	m_handler = std::make_unique<qt_music_handler>();

	connect(ui->fileSelectButton, &QPushButton::clicked, this, [this](){ select_file(); });
	connect(ui->playButton, &QPushButton::clicked, this, [this](){ m_handler->play(m_file_path); });
	connect(ui->pauseButton, &QPushButton::clicked, this, [this](){ m_handler->pause(); });
	connect(ui->stopButton, &QPushButton::clicked, this, [this](){ m_handler->stop(); });
	connect(ui->forwardButton, &QPushButton::clicked, this, [this](){ m_handler->fast_forward(m_file_path); });
	connect(ui->reverseButton, &QPushButton::clicked, this, [this](){ m_handler->fast_reverse(m_file_path); });
	connect(ui->volumeSlider, &QSlider::valueChanged, this, [this](int value){ m_handler->set_volume(std::clamp(value / 100.0f, 0.0f, 1.0f)); });
}

music_player_dialog::~music_player_dialog()
{
}

void music_player_dialog::select_file()
{
	// Initialize Emu if not yet initialized (e.g. Emu.Kill() was previously invoked) before using some of the following vfs:: functions (e.g. vfs::get())
	if (Emu.IsStopped())
	{
		Emu.Init();
	}

	const std::string vfs_dir_path = vfs::get("/dev_hdd0/music");
	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select audio file"), QString::fromStdString(vfs_dir_path), tr("Audio files (*.mp3;*.wav;*.aac;*.ogg;*.flac;*.m4a;*.alac);;All files (*.*)"));

	if (!file_path.isEmpty())
	{
		m_file_path = file_path.toStdString();
	}
}
