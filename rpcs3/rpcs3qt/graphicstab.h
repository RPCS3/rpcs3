#ifndef GRAPHICSTAB_H
#define GRAPHICSTAB_H

#include "EmuSettings.h"

#include <QWidget>

#include <memory>
class GraphicsTab : public QWidget
{
	Q_OBJECT

public:
	explicit GraphicsTab(std::shared_ptr<EmuSettings> xEmuSettings, QWidget *parent = 0);

signals:

private:
	std::shared_ptr<EmuSettings> xEmuSettings;
};

#endif // GRAPHICSTAB_H
