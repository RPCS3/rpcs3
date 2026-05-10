#pragma once

#include <string>
#include <QObject>

struct pad_device_info
{
	std::string name;
	QString localized_name;
	bool is_connected{false};
};

Q_DECLARE_METATYPE(pad_device_info)
