#include "stdafx.h"
#include "game_source_dialog.h"

#include "Loader/ISO.h"

#include <QFileInfo>
#include <QMessageBox>

#include <utility>

game_source_dialog::game_source_dialog(QWidget* parent, options options)
	: QFileDialog(parent, options.caption, options.directory)
	, m_options(std::move(options))
{
	setAcceptMode(QFileDialog::AcceptOpen);
	setFileMode(m_options.allow_multiple ? QFileDialog::ExistingFiles : QFileDialog::ExistingFile);
	setNameFilters({
		tr("PS3 disc games (*.iso *.ISO *.zar *.ZAR)"),
		tr("ISO disc images (*.iso *.ISO)"),
		tr("ZArchive disc games (*.zar *.ZAR)"),
		tr("All files (*.*)")
	});
	setOption(QFileDialog::DontUseNativeDialog, true);
	setOption(QFileDialog::DontResolveSymlinks, true);

	if (!m_options.accept_label.isEmpty())
	{
		setLabelText(QFileDialog::Accept, m_options.accept_label);
	}
}

const QStringList& game_source_dialog::selected_sources() const
{
	return m_selected_sources;
}

void game_source_dialog::accept()
{
	const QStringList selected = selectedFiles();
	if (selected.isEmpty())
	{
		return;
	}

	if (!m_options.allow_multiple && selected.size() != 1)
	{
		QMessageBox::warning(this, tr("Invalid Game Selection"), tr("Select exactly one game folder, ISO image, or ZArchive file."));
		return;
	}

	QStringList sources;

	for (const QString& path : selected)
	{
		const QFileInfo info(path);

		if (!info.exists())
		{
			QMessageBox::warning(this, tr("Invalid Game Source"), tr("The selected path does not exist:\n%1").arg(path));
			return;
		}

		if (info.isDir())
		{
			if (!sources.contains(info.absoluteFilePath()))
			{
				sources.append(info.absoluteFilePath());
			}
			continue;
		}

		if (!info.isFile())
		{
			QMessageBox::warning(this, tr("Invalid Game Source"), tr("The selected path is neither a regular file nor a directory:\n%1").arg(path));
			return;
		}

		const QString suffix = info.suffix().toLower();
		if (suffix != QStringLiteral("iso") && suffix != QStringLiteral("zar"))
		{
			QMessageBox::warning(this, tr("Unsupported Game Format"),
				tr("RPCS3 supports disc games from JB folders, ISO images, and ZArchive files.\n\nUnsupported file:\n%1").arg(path));
			return;
		}

		if (m_options.validate_file_sources && !is_iso_file(info.absoluteFilePath().toStdString()))
		{
			QMessageBox::warning(this, tr("Invalid Disc Image"),
				tr("The selected file is not a valid PS3 ISO/ZArchive disc game or the archive is corrupted:\n%1").arg(path));
			return;
		}

		if (!sources.contains(info.absoluteFilePath()))
		{
			sources.append(info.absoluteFilePath());
		}
	}

	if (sources.isEmpty())
	{
		return;
	}

	m_selected_sources = std::move(sources);
	QDialog::accept();
}
