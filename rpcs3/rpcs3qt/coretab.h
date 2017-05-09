#ifndef CORETAB_H
#define CORETAB_H

#include "EmuSettings.h"

#include <QLineEdit>
#include <QListWidget>
#include <QWidget>

#include <memory>

class CoreTab : public QWidget
{
	Q_OBJECT

public:
	explicit CoreTab(std::shared_ptr<EmuSettings> xSettings, QWidget *parent = 0);
public slots:
	void SaveSelectedLibraries();
private slots:
	void OnSearchBoxTextChanged();
	void OnHookButtonToggled();
	void OnPPUDecoderToggled();
	void OnSPUDecoderToggled();
	void OnLibButtonClicked(int ind);

private:
	bool shouldSaveLibs = false;
	QListWidget *lleList;
	QLineEdit *searchBox;

	std::shared_ptr<EmuSettings> xEmuSettings;
};

#endif // CORETAB_H
