#pragma once

#include <QCoreApplication>
#include <QFileDialog>
#include <QString>
#include <QStringList>

class game_source_dialog final : public QFileDialog
{
	Q_DECLARE_TR_FUNCTIONS(game_source_dialog)

public:
	struct options
	{
		QString caption;
		QString directory;
		QString accept_label;
		bool allow_multiple = false;
		bool validate_file_sources = true;
	};

	explicit game_source_dialog(QWidget* parent, options options);

	const QStringList& selected_sources() const;

protected:
	void accept() override;

private:
	options m_options;
	QStringList m_selected_sources;
};
