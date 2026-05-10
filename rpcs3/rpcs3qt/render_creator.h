#pragma once

#include "emu_settings_type.h"

#include <QObject>
#include <QString>
#include <QStringList>

class render_creator : public QObject
{
	Q_OBJECT

public:
	render_creator(QObject* parent);

	void update_names(const QStringList& names);

	struct render_info
	{
		QString name;
		QString old_adapter;
		QStringList adapters;
		emu_settings_type type = emu_settings_type::VulkanAdapter;
		bool supported = true;
		bool has_adapters = true;

		render_info()
			: has_adapters(false) {}

		render_info(QStringList adapters, bool supported, emu_settings_type type)
			: adapters(std::move(adapters))
			, type(type)
			, supported(supported) {}
	};

	bool abort_requested = false;
	bool supports_vulkan = false;
	QStringList vulkan_adapters;
	render_info Vulkan;
	render_info OpenGL;
	render_info NullRender;
	std::vector<render_info*> renderers;
};
