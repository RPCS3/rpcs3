#pragma once

#include "emu_settings_type.h"

#include <QString>
#include <QStringList>

class render_creator
{
public:
	render_creator();

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

	bool vulkan_timed_out = false;
	bool supports_vulkan = false;
	QStringList vulkan_adapters;
	render_info Vulkan;
	render_info OpenGL;
	render_info NullRender;
	std::vector<render_info*> renderers;
};
